//camera.cpp
//Copyright (C) <2011, 2012>  <Yiying Li>
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/> .
//

#include <stdio.h>
#include <cassert>
#include <cstring>
#include <cstdlib>

#ifndef NOOPENCV
#include "highgui.h"
#endif

#include "camera.h"
#include "cameraconstants.h"

#include "Timer.hpp"

using namespace cam1394;

/* defualt constructor */
camera::camera() : guid(0), width(-1), height(-1), cam(NULL) {}

/* destructor */
camera::~camera()
{
    clean_up();
}

int camera::open() {
    return open("NONE");
}

int camera::open(const char* cam_guid) {
    if (initCam(cam_guid) < 0) {
        return -1;
    }
    
    dc1394video_mode_t mode;
    dc1394framerate_t rate;

    if (getBestVideoMode(&mode) < 0) {
        return -1;
    } else if (getBestFrameRate(&rate, mode) < 0) {
        return -1;
    }

    if (initParam(videoModeNames[mode - STARTVIDEOMODE], 
                  videoFrameRates[rate - STARTFRAMERATE], NULL, NULL) < 0) {
        clean_up();
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_capture_setup(cam, 10, DC1394_CAPTURE_FLAGS_DEFAULT)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to setup camera\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_ON)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start camera\n");
        return -1;  
    }

    return 0;
}

void camera::printConnectedCams() {
    dc1394error_t err;
    dc1394camera_list_t *list;
    dc1394_t *d;
    
    d = dc1394_new();
    if (d == NULL) {
        fprintf(stderr, "ERROR: Can't initialize dc1394_content\n");
        dc1394_camera_free_list(list);
        dc1394_free(d);
        return;
    }
    
    err = dc1394_camera_enumerate(d, &list);
    if (err != DC1394_SUCCESS) {
        fprintf(stderr, "ERROR: Could not get camera list\n");
        dc1394_camera_free_list(list);
        dc1394_free(d);
        return;
    }
    
    if (list->num == 0) {
        printf("ERROR: No Cameras Found\n");
        dc1394_camera_free_list(list);
        dc1394_free(d);
        return;
    }

    printf("====== Connected Cameras ======\n");    
    for (unsigned int i = 0; i < list->num; i++) {
        uint64_t guid = list->ids[i].guid;
    
        dc1394camera_t *camera;
        camera = dc1394_camera_new(d, guid);
        if (!camera) {
            fprintf(stderr, "ERROR: Failed to open camera with GUID %016llX\n", guid);
            clean_up();
            return;
        }

        printf("Cam %d: %016llX (%s %s)\n", i, guid, camera->vendor, camera->model);
        printSupportedVideoModes(camera);
        printf("\n\n");
        dc1394_camera_free(camera);
    }
    
    dc1394_camera_free_list(list);
    dc1394_free(d);
}

int camera::initCam(const char* cam_guid) {
    int err;
    dc1394camera_list_t *list;
    d = dc1394_new();
    if (d == NULL)
    {
        fprintf(stderr, "ERROR: Can't initialize dc1394_content\n");
        return -1;
    }

    err = dc1394_camera_enumerate(d, &list);
    if (err != DC1394_SUCCESS)
    {
        fprintf(stderr, "ERROR: Could not get camera list\n");
        return -1;
    }

    if (list->num == 0)
    {
        fprintf(stderr, "ERROR: No Cameras Found\n");
        return -1;
    }
    
    char* temp = (char*)malloc(1024*sizeof(char));

    for(unsigned int i=0; i < list->num; i++)
    {
        sprintf(temp,"%016llX", list->ids[i].guid);

        if (!strcasecmp(cam_guid, "NONE"))
        {
            fprintf(stderr, "WARNING: No guid specified, using first camera. GUID: %s\n", temp);
            guid = list->ids[i].guid;
            break;
        }

        if (!strcasecmp(temp, cam_guid))
        {
#ifdef DEBUGCAMERA
            fprintf(stderr, "Camera %s found\n", cam_guid);
#endif
            guid = list->ids[i].guid;
            break;
        }
    }

    if (guid == 0) {
        fprintf(stderr, "ERROR: Failed to find camera with GUID %s\n", cam_guid);
        clean_up();
        return -1;      
    }

    cam = dc1394_camera_new(d, guid);
    if (!cam) {
        fprintf(stderr, "ERROR: Failed to initliaze camera with GUID %016llX\n", guid);
        clean_up();
        return -1;
    }
        
    free(temp);
    dc1394_camera_free_list(list);
    
    if (!cam)
    {
        if (!strcasecmp(cam_guid, "NONE"))
        {
            fprintf(stderr, "ERROR: Can't find camera\n");
        }
        else
        {
            fprintf(stderr, "ERROR: Can't find camera with guid %s\n", cam_guid);
        }
        return -1;
    }

    return 0;
}

int camera::initParam(const char* video_mode, float fps, const char* method, const char* pattern) {
    if (_setVideoMode(video_mode) < 0) {
        return -1;
    } else if (_setFrameRate(fps) < 0) {
        return -1;
    } else if (setBayer(method, pattern) < 0) {
        return -1;
    }
    
    return 0;
}

int camera::open(const char* cam_guid, const char* video_mode, float fps, const char* method, const char* pattern)
{
    if (initCam(cam_guid) < 0) {
        return -1;
    }

    if (initParam(video_mode, fps, method, pattern) < 0) {
        clean_up();
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_capture_setup(cam, 10, DC1394_CAPTURE_FLAGS_DEFAULT)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to setup camera\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_ON)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start camera\n");
        return -1;  
    }

    return 0;
}

int camera::_setFrameRate(float fps) {
    dc1394framerate_t fr; 
    if (convertFrameRate(fps, &fr) < 0) {
        printSupportedFrameRates(cam, _video_mode);
        return -1;
    }

    if (DC1394_SUCCESS != dc1394_video_set_framerate(cam, fr))
    {
        fprintf(stderr, "ERROR: Failed to set the framerate\n");
        return -1;
    }

    _fps = fr;
    return 0;
}

int camera::getBestVideoMode(dc1394video_mode_t *mode) {
    dc1394error_t err;
    dc1394video_modes_t modes;
    
    /* get all supported video modes for camera*/
    err = dc1394_video_get_supported_modes(cam, &modes);

    if (err != DC1394_SUCCESS) 
        fprintf(stderr, "ERROR getting supported videomodes\n");
    else {
        if (modes.num == 0) {
            fprintf(stderr, "ERROR: no supported videomodes\n");
            return -1;
        }
        int count = modes.num - 1;
        *mode = modes.modes[count];
        while (*mode >= DC1394_VIDEO_MODE_FORMAT7_MIN) {
            count--;
            if (count < 0) {
                fprintf(stderr, "ERROR: no supported non-format7 videomodes\n");
                return -1;
            }   
            *mode = modes.modes[count];
        }
        return 0;
    }
    return -1;
}

int camera::getBestFrameRate(dc1394framerate_t *rate, dc1394video_mode_t mode) {
    dc1394error_t err;
    dc1394framerates_t rates;
    
    /* get all supported video modes for camera*/
    err = dc1394_video_get_supported_framerates(cam, mode, &rates);

    if (err != DC1394_SUCCESS) 
        fprintf(stderr, "ERROR getting supported framerates\n");
    else {
        if (rates.num == 0) {
            fprintf(stderr, "ERROR: no supported framerates\n");
            return -1;
        }
        *rate = rates.framerates[rates.num - 1];
        return 0;
    }
    return -1;
}

/* Sets video_mode based on string input */
int camera::convertVideoMode(const char* mode, dc1394video_mode_t *video_mode)
{
    for (size_t i = 0; i < 32; i++) {
        if (!strcasecmp(mode, videoModeNames[i])) {
            *video_mode = (dc1394video_mode_t)(i + STARTVIDEOMODE);

            /* if a non-Format 7 mode set width and height */
            if (i < 23) {
                //width = videoWidths[i];
                //height = videoHeights[i];
                if (DC1394_SUCCESS != 
                    dc1394_get_image_size_from_video_mode(cam, *video_mode, (uint32_t*)&width, (uint32_t*)&height)) {
                    fprintf(stderr, "ERROR: Failed to convert video mode to image size\n");
                    return -1;
                }
            }

            /* check if it is a valid video */
            if (checkValidVideoMode(video_mode) < 0)
                return -1;
            else
                return 1;
        }
    }

    return -1;
}

/* Checks if the input video mode is a valid video mode */
int camera::checkValidVideoMode(dc1394video_mode_t *mode) {
    if (mode == NULL)
        return -1;
    else if (*mode < DC1394_VIDEO_MODE_MIN || *mode > DC1394_VIDEO_MODE_MAX)
        return -1;

    dc1394error_t err;
    dc1394video_modes_t modes;
    
    /* get all supported video modes for camera*/
    err = dc1394_video_get_supported_modes(cam, &modes);

    if (err != DC1394_SUCCESS) 
        fprintf(stderr, "ERROR getting supported videomodes\n");
    else {
        for (unsigned int i = 0; i < modes.num; i++) {
            if (*mode == modes.modes[i]) 
                return 1;
        }
    }

    return -1;
}

/* Prints the supported video modes */
void camera::printSupportedVideoModes(dc1394camera_t *camera) {
    if (!camera) {
        fprintf(stderr, "ERROR: no cameras initlizalied\n");
        return;
    }
    dc1394error_t err;
    dc1394video_modes_t modes;
    err = dc1394_video_get_supported_modes(camera, &modes);
    
    if (err != DC1394_SUCCESS) 
        fprintf(stderr, "ERROR getting supported videomodes\n");
    else {
        printf("listing possible video modes\n");
        for (unsigned int i = 0; i < modes.num; i++) {
            printf("mode %d: [%d] %s:\n", i, modes.modes[i], videoModeNames[modes.modes[i] - STARTVIDEOMODE]);
            if (modes.modes[i] < DC1394_VIDEO_MODE_FORMAT7_MIN)
                printSupportedFrameRates(camera, modes.modes[i]);
        }
    }
}

int camera::_setVideoMode(const char* video_mode) {
    dc1394video_mode_t mode;
    if (convertVideoMode(video_mode, &mode) < 0) {
        printf("ERROR: invalid video mode: %s\n", video_mode);
        printSupportedVideoModes(cam);
        return -1;
    }

    if (DC1394_SUCCESS != dc1394_video_set_mode(cam, mode))
    {
        fprintf(stderr, "ERROR: Failed to set the video mode\n");
        return -1;
    }

    _video_mode = mode;

    dc1394framerate_t rate;
    if (getBestFrameRate(&rate, mode) < 0)
        return -1;

    if (_setFrameRate(videoFrameRates[rate - STARTFRAMERATE]))
        return -1;

    return 0;
}

int camera::convertFrameRate(float fps, dc1394framerate_t *frame_rate) {
    if (fps < 3.75)
        *frame_rate = DC1394_FRAMERATE_1_875;
    else if (fps < 7.5)
        *frame_rate = DC1394_FRAMERATE_3_75;
    else if (fps < 15)
        *frame_rate = DC1394_FRAMERATE_7_5;
    else if (fps < 30)
        *frame_rate = DC1394_FRAMERATE_15;
    else if (fps < 60)
        *frame_rate = DC1394_FRAMERATE_30;
    else if (fps < 120)
        *frame_rate = DC1394_FRAMERATE_60;
    else if (fps < 240)
        *frame_rate = DC1394_FRAMERATE_120;
    else
        *frame_rate = DC1394_FRAMERATE_240;

    if (checkValidFrameRate(frame_rate) < 0)
        return -1;
    else
        return 1;
}

int camera::checkValidFrameRate(dc1394framerate_t* frame_rate) {
    if (frame_rate == NULL) 
        return -1;
    else if (*frame_rate < DC1394_FRAMERATE_MIN || *frame_rate > DC1394_FRAMERATE_MAX)
        return -1;
    else if (_video_mode < DC1394_VIDEO_MODE_MIN || _video_mode > DC1394_VIDEO_MODE_MAX) {
        fprintf(stderr, "ERROR: Haven't set video mode yet\n");
        return -1;
    }

    dc1394error_t err;
    dc1394framerates_t rates;

    err = dc1394_video_get_supported_framerates(cam, _video_mode, &rates);

    if (err != DC1394_SUCCESS)
        fprintf(stderr, "ERROR getting supported framerates");
    else {
        for (unsigned int i = 0; i < rates.num; i++) {
            if (*frame_rate == rates.framerates[i]) 
                return 1;
        }
    }

    return -1;
}

/* Prints the supported frame rates */
void camera::printSupportedFrameRates(dc1394camera_t *camera, dc1394video_mode_t mode) {
    if (!camera) {
        fprintf(stderr, "ERROR: no cameras initlizalied\n");
        return;
    }
    if (mode < DC1394_VIDEO_MODE_MIN || mode > DC1394_VIDEO_MODE_MAX) {
        fprintf(stderr, "ERROR: Invalid video mode, can't get frame rates");
        return;
    }

    dc1394error_t err;
    dc1394framerates_t rates;
    err = dc1394_video_get_supported_framerates(camera, mode, &rates);
    
    if (err != DC1394_SUCCESS) 
        fprintf(stderr, "ERROR getting supported framerates");
    else {
        printf("Print Supported frame rates for video mode: %s\n", videoModeNames[mode - STARTVIDEOMODE]);
        for (unsigned int i = 0; i < rates.num; i++) {
            printf("    FPS %d: [%d] %f\n", i, rates.framerates[i], videoFrameRates[rates.framerates[i] - STARTFRAMERATE]);
        }
    }
}

int camera::setBayer(const char* method, const char* pattern)
{
    bayer_met = (dc1394bayer_method_t)-1;
    if (method == NULL)
        return 0;

    for (int i = 0; i < DC1394_BAYER_METHOD_NUM; i++) {
        if (!strcasecmp(method, bayerMethods[i])) {
            bayer_met = (dc1394bayer_method_t)(STARTBAYERMETHODS + i);
            break;
        }
    }
    
    if (bayer_met == -1) {
        bayer_pat = (dc1394color_filter_t)-1;
        return 0;
    }

    for (int i = 0; i < DC1394_COLOR_FILTER_NUM; i++) {
        if (!strcasecmp(pattern, bayerPatterns[i])) {
            bayer_pat = (dc1394color_filter_t)(STARTCOLORFILTER + i);
            return 0;
        }
    }

    fprintf(stderr, "ERROR: invalid pattern");
    return -1;
}

void camera::clean_up()
{
    if (cam) {
        dc1394_capture_stop(cam);
        dc1394_camera_free(cam);
    }
    cam = NULL;
}

int camera::setBrightness(unsigned int brightness)
{
    if (DC1394_SUCCESS != dc1394_feature_set_value(cam, DC1394_FEATURE_BRIGHTNESS, brightness))
    {
        fprintf(stderr, "ERROR: Unable to set brightness\n");
        return -1;
    }
    return 0;
}

int camera::setExposure(unsigned int exposure)
{
    if (DC1394_SUCCESS != dc1394_feature_set_value(cam, DC1394_FEATURE_EXPOSURE, exposure))
    {
        fprintf(stderr, "ERROR: Unable to set exposure\n");
        return -1;
    }
    return 0;
}

int camera::setTrigger(int trigger_in)
{
    dc1394switch_t trigger = DC1394_OFF;
    if (trigger_in > 0)
        trigger = DC1394_ON;

    if (DC1394_SUCCESS != dc1394_external_trigger_set_power(cam, trigger))
    {
        fprintf(stderr, "ERROR: Unable to set trigger mode\n");
        return -1;
    }
    return 0;
}

int camera::setShutter(int shutter)
{
    bool autoShutter = shutter < 0;

    if (DC1394_SUCCESS != dc1394_feature_set_mode(cam, DC1394_FEATURE_SHUTTER, (autoShutter ? DC1394_FEATURE_MODE_AUTO:DC1394_FEATURE_MODE_MANUAL)))
    {
        fprintf(stderr, "ERROR: Unable to set shutter mode\n");
        return -1;
    }

    if (!autoShutter)
    {
        if (DC1394_SUCCESS != dc1394_feature_set_value(cam, DC1394_FEATURE_SHUTTER, shutter))
        {
            fprintf(stderr, "ERROR: Unable to set shutter value\n");
            return -1;
        }
    }
    return 0;

}

int camera::setGain(int gain)
{
    bool autoGain = gain <0;

    if (DC1394_SUCCESS != dc1394_feature_set_mode(cam, DC1394_FEATURE_GAIN, (autoGain ? DC1394_FEATURE_MODE_AUTO:DC1394_FEATURE_MODE_MANUAL)))
    {
        fprintf(stderr, "ERROR: Unable to set gain mode\n");
        return -1;
    }

    if (!autoGain)
    {
        if (DC1394_SUCCESS != dc1394_feature_set_value(cam, DC1394_FEATURE_GAIN, gain))
        {
            fprintf(stderr, "ERROR: Unable to set gain value\n");
            return -1;
        }
    }

    return 0;
}

int camera::setWhiteBalance(unsigned int b_u, unsigned int r_v)
{
    if (DC1394_SUCCESS != dc1394_feature_whitebalance_set_value(cam, b_u, r_v))
    {
        fprintf(stderr, "ERROR: Unable to set white balance value\n");
        return -1;
    }

    return 0;
}

int camera::read(cam1394Image* image) {
    if (!cam)
    {
        fprintf(stderr, "ERROR: Camera not initialized\n");
        exit(1);
    }
    
    const int W = width;
    const int H = height;
    dc1394video_frame_t end;
    dc1394video_frame_t * frame;
    dc1394video_frame_t * prev_frame;
    prev_frame = (dc1394video_frame_t *)malloc(sizeof(dc1394video_frame_t));
    prev_frame->id = 255;
    dc1394error_t err;
    end.image = (unsigned char*)malloc(W * H * 3 * sizeof(unsigned char));
    end.color_coding = DC1394_COLOR_CODING_RGB8;
    
    int frames_read = 0;

    Timer waiting;
    Timer polling;
    Timer debayer;
    Timer memory;

    waiting.start();
    err = dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (frame != NULL && err == DC1394_SUCCESS)
    {
        dc1394_capture_enqueue(cam, frame);
        memcpy(prev_frame, frame, sizeof(dc1394video_frame_t));
        frames_read++;
    }
    waiting.end();

    polling.start();
    while (1)
    {
        err = dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_POLL, &frame);
        if (frame == NULL && err == DC1394_SUCCESS && prev_frame->id != 255)
            break;
        else if (frame != NULL && err == DC1394_SUCCESS)
        {
            memcpy(prev_frame, frame, sizeof(dc1394video_frame_t));
            dc1394_capture_enqueue(cam, frame);
            frames_read++;
        }
    }
    droppedframes = frames_read - 1;
    prev_frame->color_filter= bayer_pat;

    polling.end();


    if (bayer_met != -1) {
        debayer.start();
        if (DC1394_SUCCESS != dc1394_debayer_frames(prev_frame, &end, bayer_met))
        {
            fprintf(stderr, "ERROR: Unable to debayer frame\n");
            return -1;
        }

        if (image->data != NULL)
            image->destroy();
        image->width  = end.size[0];
        image->height = end.size[1];
        image->size   = end.image_bytes;
        image->data   = new char[image->size]();
        debayer.end();
        memory.start();
        memcpy(image->data, end.image, image->size);
        memory.end();
    } else {
        if (image->data != NULL)
            image->destroy();
        image->width  = prev_frame->size[0];
        image->height = prev_frame->size[1];
        image->size   = prev_frame->image_bytes;
        image->data   = new char[image->size]();
        memory.start();
        memcpy(image->data, prev_frame->image, image->size);
        memory.end();
    }

    // printf("w: %9.6f p: %9.6f d: %9.6f m: %9.6f \n", 
    //     waiting.elapsed()*1000, polling.elapsed()*1000, 
    //     debayer.elapsed()*1000, memory.elapsed()*1000);

    free(end.image);
    free(prev_frame);
    return 0;
}

#ifndef NOOPENCV
cv::Mat camera::read()
{
    if (!cam)
    {
        fprintf(stderr, "ERROR: Camera not initialized\n");
        exit(1);
    }
    
    const int W = width;
    const int H = height;
    dc1394video_frame_t end;
    dc1394video_frame_t * frame;
    dc1394video_frame_t * prev_frame;
    prev_frame = (dc1394video_frame_t *)malloc(sizeof(dc1394video_frame_t));
    prev_frame->id = 255;
    dc1394error_t err;
    end.image = (unsigned char*)malloc(W * H * 3 * sizeof(unsigned char));
    end.color_coding = DC1394_COLOR_CODING_RGB8;
    
    
    int frames_read = 0;
        
    err = dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (frame != NULL && err == DC1394_SUCCESS)
    {
        memcpy(prev_frame, frame, sizeof(dc1394video_frame_t));
        frames_read++;
    }

    while (1)
    {
        err = dc1394_capture_dequeue(cam, DC1394_CAPTURE_POLICY_POLL, &frame);
        if (frame == NULL && err == DC1394_SUCCESS && prev_frame->id != 255)
            break;
        else if (frame != NULL && err == DC1394_SUCCESS)
        {
            memcpy(prev_frame, frame, sizeof(dc1394video_frame_t));
            frames_read++;
        }
    }
    droppedframes = frames_read - 1;
    prev_frame->color_filter= bayer_pat;

    int bits = prev_frame->image_bytes/(prev_frame->size[0]*prev_frame->size[1]) * 8;

    cv::Mat ret;
    if (bayer_met != -1) {
        if (DC1394_SUCCESS != dc1394_debayer_frames(prev_frame, &end, bayer_met))
        {
            fprintf(stderr, "ERROR: Unable to debayer frame\n");
        }

        cv::Mat final(H, W, getOpenCVbits(bits, 3), end.image);
        final.copyTo(ret);
    } else {
        cv::Mat final(H, W, getOpenCVbits(bits, 1), prev_frame->image);
        final.copyTo(ret);
    }

    timestamp = prev_frame->timestamp;
    free(end.image);

    dc1394_capture_enqueue(cam, prev_frame);
    free(prev_frame);
    return ret;
}
#endif

long camera::getTimestamp()
{
    return timestamp;
}

int camera::getNumDroppedFrames()
{
    return droppedframes;
}

void camera::printGUID()
{
    printf("GUID of attached camera is: %016llX\n", guid);
}

long camera::getGUID() {
    return guid;
}

int camera::setVideoMode(const char* video_mode) {
    if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_OFF)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to stop transmission\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_capture_stop(cam)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to stop capture\n");
        return -1;  
    }

    int ret;
    if ((ret = _setVideoMode(video_mode)) < 0)
        return ret;

    if (DC1394_SUCCESS != dc1394_capture_setup(cam, 30, DC1394_CAPTURE_FLAGS_DEFAULT)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start capture\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_ON)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start transmission\n");
        return -1;  
    }

    return 0;
}

int camera::setFrameRate(float fps) {
    if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_OFF)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to stop transmission\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_capture_stop(cam)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to stop capture\n");
        return -1;  
    }

    int ret;
    if ((ret = _setFrameRate(fps)) < 0)
        return ret;

    if (DC1394_SUCCESS != dc1394_capture_setup(cam, 10, DC1394_CAPTURE_FLAGS_DEFAULT)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start capture\n");
        return -1;  
    } else if (DC1394_SUCCESS != dc1394_video_set_transmission(cam, DC1394_ON)) {
        clean_up();
        fprintf(stderr, "ERROR: Failed to start transmission\n");
        return -1;  
    }

    return 0;
}

void camera::printVideoMode() {
    printf("Video Mode: %s\n", videoModeNames[_video_mode - STARTVIDEOMODE]);
}

void camera::printFrameRate() {
    printf("Frame Rate: %f\n", videoFrameRates[_fps - STARTFRAMERATE]);
}

#ifndef NOOPENCV
int camera::getOpenCVbits(int bits, int stride) {
    if (bits <= 8) {
        return CV_8UC(stride); 
    } else if (bits <= 16) {
        return CV_16UC(stride);
    } else if (bits <= 32) {
        return CV_32FC(stride);
    } else {
        return CV_64FC(stride);
    }
    return 0;
}
#endif
