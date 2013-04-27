#ifndef RPIDECODE_H
#define RPIDECODE_H

#include <udt/udt.h>

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

struct Image {
    char *data;
    int width;
    int height;
    int size;

    Image(char* d, int w, int h, int s) : width(w), height(h), size(s) {
        data = new char[s];
        memcpy(data, d, s);
    }

    ~Image() {
        delete[] data;
    }
};

UDTSOCKET RPiDecodeInit(const char* addr, int port);
int RPiDecodeGetImage(UDTSOCKET sock, Image **im, double& timestamp);

void RPiDecodeDestroyAll();

int RPiDecodeGetWidth();
int RPiDecodeGetHeight();

#endif