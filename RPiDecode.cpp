#include "RPiDecode.h"
#include <iostream>
#include <udt/udt.h>
#include "udtComm.h"
#include "Timer.hpp"

extern "C" {
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/common.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/samplefmt.h>
}

using std::cout;
using std::cerr;
using std::endl;

/*** Global Variables (local to this file) ***/
static int g_width                        = 0;
static int g_height                       = 0;
static AVCodec *codec;
static AVCodecContext *c                  = NULL;
static AVFrame *frame;
static AVPacket avpkt;
static AVPicture pict;
static struct SwsContext *img_convert_ctx = NULL;
static uint8_t *buffer                    = NULL;

static void print_context(AVCodecContext *c) {
    cout << "===== START: AVCodecContext =====" << endl;
    if (c == NULL) {
        cout << "NULL AVCodecContext" << endl;
    } else {
        cout << "Width            : " << c->width << endl;
        cout << "Height           : " << c->height << endl;
        cout << "Average Bit Rate : " << c->bit_rate << endl;
    }
    cout << "====== END: AVCodecContext ======" << endl;
}

UDTSOCKET RPiDecodeInit(const char* addr, int port) {
    UDTSOCKET sock;
    char sport[10];
    snprintf(sport, 10, "%d", port);
    if ((sock = udtConnect(addr, sport)) == UDT::ERROR) {
        return 1;
    }
    avcodec_register_all();
    
    av_init_packet(&avpkt);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        cerr << "Can't find decoder for H264" << endl;
        return -1;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        cerr << "Failed to allocate codec context" << endl;
        return -1;
    }

    c->err_recognition |= AV_EF_CAREFUL;
    c->workaround_bugs = FF_BUG_AUTODETECT;
    c->error_concealment = 0;
    c->flags2 |= CODEC_FLAG2_CHUNKS;

    if (avcodec_open2(c, codec, NULL) < 0) {
        cerr << "Failed to open codec" << endl;
        return -1;
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
        cerr << "Failed to allocate frame" << endl;
        return -1;
    }

    char *buf;
    int size = 0;
    int len = 0;
    int got_frame;
    uint64_t pkt_size = 0;
    double timestamp;

    for (int i = 0; i < 2; i++) {
        if (udtRecv(sock, (char*)&pkt_size, sizeof(pkt_size)) < 0) {
            cerr << "udtRecv Failed" << endl;
            return -1;
        }

        if (udtRecv(sock, (char*)&timestamp, sizeof(timestamp)) < 0) {
            cerr << "udtRecv Failed" << endl;
            return -1;
        }

        buf = new char[pkt_size];
        if (udtRecv(sock, buf, pkt_size) < 0) {
            cerr << "udtRecv Failed" << endl;
        }

        size = pkt_size;
        avpkt.size = size;
        avpkt.data = (uint8_t*)buf;
        if ((len = avcodec_decode_video2(c, frame, &got_frame, &avpkt)) <= 0) {
            cout << "Failed!" << size << endl;
            return -1;
        }

        delete[] buf;
    }

    // get first image for initilization
    if (udtRecv(sock, (char*)&pkt_size, sizeof(pkt_size)) < 0) {
        cerr << "udtRecv Failed" << endl;
        return -1;
    }

    if (udtRecv(sock, (char*)&timestamp, sizeof(timestamp)) < 0) {
        cerr << "udtRecv Failed" << endl;
        return -1;
    }

    buf = new char[pkt_size];
    if (udtRecv(sock, buf, pkt_size) < 0) {
        cerr << "udtRecv Failed" << endl;
    }
    size = pkt_size;

    avpkt.size = size;
    avpkt.data = (uint8_t*)buf;
    if ((len = avcodec_decode_video2(c, frame, &got_frame, &avpkt)) <= 0) {
        cout << "Failed!" << size << endl;
        return -1;
    }

    // print_context(c);
    g_width = c->width;
    g_height = c->height;

    if (g_width <= 0 || g_height <= 0) {
        cerr << "width or height == 0" << endl;
        return -1;
    }

    img_convert_ctx = sws_getContext(c->width, c->height, c->pix_fmt, c->width, c->height, 
        PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, c->width, c->height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    avpicture_fill(&pict, buffer, PIX_FMT_RGB24, c->width, c->height);
    if (img_convert_ctx == NULL) {
        return -1;
    }

    delete[] buf;

    return sock;
}

int RPiDecodeGetImage(UDTSOCKET sock, Image **im, double &timestamp) {
    char *buf;
    int size = 0;
    int len = 0;
    int got_frame;

    uint64_t pkt_size = 0;
    if (udtRecv(sock, (char*)&pkt_size, sizeof(pkt_size)) < 0) {
        cerr << "udtRecv Failed" << endl;
        return -1;
    }

    if (udtRecv(sock, (char*)&timestamp, sizeof(timestamp)) < 0) {
        cerr << "udtRecv Failed" << endl;
        return -1;
    }

    buf = new char[pkt_size];
    if (udtRecv(sock, buf, pkt_size) < 0) {
        cerr << "udtRecv Failed" << endl;
    }

    size = pkt_size;

    avpkt.size = size;
    avpkt.data = (uint8_t*)buf;
    Timer encode_timer;
    encode_timer.start();
    if ((len = avcodec_decode_video2(c, frame, &got_frame, &avpkt)) <= 0) {
        cout << "Failed!" << size << endl;
    }
     
    if (got_frame) {
        if (img_convert_ctx != NULL) {
            sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, c->height, pict.data, pict.linesize);
            *im = new Image((char*)pict.data[0], c->width, c->height, 3*c->width*c->height);
            // memcpy(image, pict.data[0], 3*sizeof(GLubyte)*c->width*c->height);
        }
    }
    encode_timer.end();

    delete[] buf;
    return 0;
}

void RPiDecodeDestroyAll() {
    avcodec_close(c);
    av_free(c);
    avcodec_free_frame(&frame);
    sws_freeContext(img_convert_ctx);
    free(buffer);
}

int RPiDecodeGetWidth() {
    return g_width;
}

int RPiDecodeGetHeight() {
    return g_height;
}