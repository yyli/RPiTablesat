#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <unistd.h>

extern "C" {
    #include "bcm_host.h"
    #include "ilclient.h"
}

#include "udtComm.h"
#include "Timer.hpp"
#include "camera.h"

using namespace cam1394;
Timer timestamp;

struct partial_frame_t {
	char *data;
	int size;
	bool del;

	partial_frame_t(char *data, int size, bool del) : data(data), size(size), del(del) {}
};

void camera_loop(UDTSOCKET sock, int bitrate) {
	int camHeight = 0;
	int camWidth = 0;
    camera a;
    cam1394Image img;

    if (a.open("NONE", "640x480_MONO8", 30, "DOWNSAMPLE", "RGGB") < 0)
        return;

	if (a.read(&img) < 0)
		return;

	camHeight = img.height;
	camWidth = img.width;

    OMX_PARAM_PORTDEFINITIONTYPE def;
    OMX_PARAM_PORTDEFINITIONTYPE defout;

    COMPONENT_T *video_encode = NULL;
    COMPONENT_T *list[5];
    OMX_BUFFERHEADERTYPE *buf;
    OMX_BUFFERHEADERTYPE *out;
    OMX_ERRORTYPE r;
    ILCLIENT_T *client;

    memset(list, 0, sizeof(list));

    if ((client = ilclient_init()) == NULL) {
        return;
    }

    if (OMX_Init() != OMX_ErrorNone) {
        ilclient_destroy(client);
        return;
    }

	// required for ilclient_create_component, for some reason its expecting a writable char array here
	char type[20];
	strcpy(type, "video_encode");

    r = (OMX_ERRORTYPE)ilclient_create_component(client, &video_encode, type,
        (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS |
                                  ILCLIENT_ENABLE_OUTPUT_BUFFERS));

    if (r != 0) {
        printf("ilclient_create_component() for video_encode failed with %x!\n", r);
        return;
    }
    list[0] = video_encode;

    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    def.nSize             = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex        = 200;

    if (OMX_GetParameter(ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition,&def) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n", __FUNCTION__, __LINE__);
        return;
    }

    def.format.video.nFrameWidth  = camWidth;
    def.format.video.nFrameHeight = camHeight;
    def.format.video.xFramerate   = 30 << 16;
    def.format.video.nSliceHeight = camHeight;
    def.format.video.nStride      = camWidth;
    def.format.video.eColorFormat = OMX_COLOR_Format24bitBGR888;

    r = OMX_SetParameter(ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition, &def);
    if (r != OMX_ErrorNone) {
        printf("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n", __FUNCTION__, __LINE__, r);
        return;
    }

    /* set bitrate */
    memset(&defout, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    defout.nSize             = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    defout.nVersion.nVersion = OMX_VERSION;
    defout.nPortIndex        = 201;

    if (OMX_GetParameter(ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition,&defout) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n", __FUNCTION__, __LINE__);
        return;
    }

    defout.format.video.nFrameWidth  = camWidth;
    defout.format.video.nFrameHeight = camHeight;
    defout.format.video.xFramerate   = 30 << 16;
    defout.format.video.nSliceHeight = camHeight;
    defout.format.video.nStride      = camWidth;
    defout.format.video.nBitrate     = bitrate;
    defout.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    r = OMX_SetParameter(ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition, &defout);
    if (r != OMX_ErrorNone) {
        printf("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n", __FUNCTION__, __LINE__, r);
        return;
    }

    printf("encode to idle...\n");
    if (ilclient_change_component_state(video_encode, OMX_StateIdle) == -1) {
        printf("%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed", __FUNCTION__, __LINE__);
        return;
    }

    printf("enabling port buffers for 200...\n");
    if (ilclient_enable_port_buffers(video_encode, 200, NULL, NULL, NULL) != 0) {
        printf("enabling port buffers for 200 failed!\n");
        return;
    }

    printf("enabling port buffers for 201...\n");
    if (ilclient_enable_port_buffers(video_encode, 201, NULL, NULL, NULL) != 0) {
        printf("enabling port buffers for 201 failed!\n");
        return;
    }

    printf("encode to executing...\n");
    ilclient_change_component_state(video_encode, OMX_StateExecuting);

    double img_time;

    while (1) {
    	Timer b;
    	b.start();
        buf = ilclient_get_input_buffer(video_encode, 200, 1);
        if (buf == NULL) {
            printf("Doh, no buffers for me!\n");
        } else {
            if (a.read(&img) < 0)
                return;

            img_time = getSystemTime();

            memcpy(buf->pBuffer, (OMX_U8*)img.data, img.size);
            buf->nFilledLen = img.size;
            if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_encode), buf) != OMX_ErrorNone) {
                printf("Error emptying buffer!\n");
                return;
            }

            bool stop = false;

            std::vector<partial_frame_t> partial_frames;
            int64_t total_len = 0;
            while (1) {
	            out = ilclient_get_output_buffer(video_encode, 201, 1);
	            r = OMX_FillThisBuffer(ILC_GET_HANDLE(video_encode), out);
	            if (r != OMX_ErrorNone) {
	                printf("Error filling buffer: %x\n", r);
	                return;
	            }

				// Debug print the buffer flags
				if (out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) {
					// printf("Got buffer flag: OMX_BUFFERFLAG_ENDOFFRAME\n");
					stop = true;
				}
				// if (out->nFlags & OMX_BUFFERFLAG_SYNCFRAME ) {
				// 	printf("Got buffer flag: OMX_BUFFERFLAG_SYNCFRAME\n");
				// }
				// if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
				// 	printf("Got buffer flag: OMX_BUFFERFLAG_CODECCONFIG\n");
				// }

				if (out == NULL) {
					printf("null buffer");
					return;
				}

				total_len += out->nFilledLen;
				if (stop) {
					partial_frames.push_back(partial_frame_t((char*)out->pBuffer, out->nFilledLen, false));
				} else {
					char *temp = new char[out->nFilledLen];
					memcpy(temp, out->pBuffer, out->nFilledLen);
					partial_frames.push_back(partial_frame_t(temp, out->nFilledLen, true));
				}

	            out->nFilledLen = 0;
	            out->nFlags = 0;

	            if (stop) {
					if (udtSend(sock, (char*)&total_len, sizeof(total_len)) < 0) {
						printf("Error writing header 1\n");
		                return;
					}

                    if (udtSend(sock, (char*)&img_time, sizeof(img_time)) < 0) {
                        printf("Error writing header 2\n");
                        return;
                    }

					for (size_t i = 0; i < partial_frames.size(); i++) {
		                if (udtSend(sock, partial_frames[i].data, partial_frames[i].size) < 0) {
		            		printf("Error writing data\n");
		                    return;
		                }

		                if (partial_frames[i].del)
		                	delete[] partial_frames[i].data;
		        	}

					printf("sent: %7lld timestamp %15.6f time: %9.6f framesdropped: %d\n", total_len, img_time, b.timePassed()*1000, a.getNumDroppedFrames());

					break;
	            }

                puts("looped");
	        }
        }
    }

    ilclient_disable_port_buffers(video_encode, 200, NULL, NULL, NULL);
    ilclient_disable_port_buffers(video_encode, 201, NULL, NULL, NULL);

    ilclient_state_transition(list, OMX_StateIdle);
    ilclient_state_transition(list, OMX_StateLoaded);

    ilclient_cleanup_components(list);

    OMX_Deinit();

    ilclient_destroy(client);
}

int main(int argc, char* argv[]) {
    timestamp.start();
    int bitrate = 0;
    if (argc > 1) {
        if (sscanf(argv[1], "%d", &bitrate) != 1) {
            puts("Failed to read bitrate");
            return 0;
        }
    }

    if (bitrate > 20000000)
        bitrate = 20000000;
    else if (bitrate < 0)
        bitrate = 0;

    bcm_host_init();
    UDTSOCKET sock = udtOpenServer("12345");

    if (UDT::ERROR == UDT::listen(sock, 10)) {
        std::cerr << "listen: " << UDT::getlasterror().getErrorMessage() << std::endl;
        return 1;
    }

    sockaddr_storage peer_addr;
    int addrlen = sizeof(peer_addr);
    UDTSOCKET peer;

    if (UDT::INVALID_SOCK == (peer = UDT::accept(sock, (sockaddr*)&peer_addr, &addrlen))) {
        std::cerr << "accept: " << UDT::getlasterror().getErrorMessage() << std::endl;
        return 0;
    }

    camera_loop(peer, bitrate);
}
