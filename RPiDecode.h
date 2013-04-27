#ifndef RPIDECODE_H
#define RPIDECODE_H

#include <udt/udt.h>
#include <cv.h>

// required libaries to decode images from the RPi
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


// inits the decoder, opens the connection to the RPi on addr and port
// RETURN: 0 if success, <0 on failure
int RPiDecodeInit(const char* addr, int port);

// gets a mat from the stream and decodes it
// also gets a timestamp of that image from the system clock on RPi
// takes in both the returned mat and the timestamp by reference
// RETURN: 0 if success, <0 on failure
int RPiDecodeGetImage(cv::Mat &ret, double &timestamp);

// destroys all used memory from this driver
void RPiDecodeDestroyAll();

// get the width of the frames
int RPiDecodeGetWidth();

// get the height of the frames
int RPiDecodeGetHeight();

#endif