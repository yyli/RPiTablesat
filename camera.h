//camera.h
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
//along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

/*!
 * \file camera.h
 *
 * \author Yiying Li
 */
#ifndef CAMERA_H
#define CAMERA_H
#include <dc1394/dc1394.h>

#ifndef NOOPENCV
#include <cv.h>
#include <highgui.h>
#endif

//! Contains the camera class definition and other misc variables
namespace cam1394
{
	struct cam1394Image {
		char *data;
		int width;
		int height;
		int size;
		

		cam1394Image() : data(NULL) {}
		/*!\brief destroys cam1394Image
		 * \return 1 if success, < 0 failure
		 */
		int destroy() {
			if (data != NULL) {
				delete[] data;
				data = NULL;
			}

			return 0;
		}
	};

	/*! 
	 * \class camera
	 * \brief reads and writes from a FireWire 1394 or Point Grey camera
	 */
	class camera
	{
	public:
		//! Makes a camera object without opening an interface
		camera();

		//! Destroys a camera object
		~camera();
				
		/*!\brief Opens an interface to the camera with the 
		 * largest resolution and the fastest possible frame rate 
		 * with no debayering
		 * \return 1 if success, <0 if failure
		 */
		int open();
		
		/*!\brief Opens an interface to the specific camera with the 
		 * largest resolution and the fastest possible frame rate 
		 * with no debayering
		 * \param cam_guid		NONE will pick the first available camera, 
		 * 						else use the GUID printed by #printGUID
		 * \return 1 if success, <0 if failure
		 */
		int open(const char *cam_guid);

		/*!\brief Opens an interface to the camera
		 * \param cam_guid		NONE will pick the first available camera, 
		 * 						else use the GUID printed by #printGUID
		 * \param video_mode	This is a string from one of the following 
		 * 						\link cameraconstants.h::videoModeNames \endlink
		 * \param fps			FPS value, floored to closest possible FPS
		 * \param bayer 		CURRENTLY THIS DOES NOTHING
		 * \param method 		CURRENTLY THIS DOES NOTHING
		 * \return 1 if success, <0 if failure
		 *
		 * <b> Example </b>
		 * \code 
		 * if (open("NONE", "640x480_MONO8", 30, NULL, NULL) > 0) {
		 * 		// display error
		 * 		return -1;
		 * }
		 * \endcode
		 */
		int open(const char* cam_guid, const char* video_mode, float fps, const char* bayer, const char* method);

		/*!\brief closes the interface to the camera
		 * \return 1 if success, < 0 if failure
		 */
		int close();

#ifndef NOOPENCV
		/*!\brief Reads an image from a camera
		 * \return cv::Mat with image if success, an empty cv::Mat if failure
		 */
		cv::Mat read();//cv::Mat&);
#endif

		/*!\brief Reads an image from a camera
		 * \return 1 if success, < 0 failure
		 */
		int read(cam1394Image* image);
		
		/*!\brief Sets the brightness of the camera
		 * \param brightness brightness value
		 * \return 0 if success, <0 if failure
		 */
		int setBrightness(unsigned int brightness);

		/*!\brief Sets the exposure of the camera
		 * \param exposure exposure value
		 * \return 0 if success, <0 if failure
		 */
		int setExposure(unsigned int exposure);

		/*!\brief Sets the shutter speed
		 * \param shutter shutter value in hz, if <0 then auto shutter
		 * \return 0 if success, <0 if failure
		 */
		int setShutter(int shutter);

		/*!\brief Sets the gain value of the camera
		 * \param gain gain value, if <0 then auto gain
		 * \return 0 if success, <0 if failure
		 */
		int setGain(int gain);

		/*!\brief Sets the trigger of the camera
		 * \param trigger_in 0 to disable trigger, 1 to enable trigger
		 * \return 0 if success, <0 if failure
		 */
		int setTrigger(int trigger_in);

		/*!\brief Sets the the white balence
		 * \param b_u blue value (0-255)
		 * \param r_v red value (0-255)
		 * \return 0 if success, <0 if failure
		 */
		int setWhiteBalance(unsigned int b_u, unsigned int r_v);

		/*!\brief gets the timestamp of the last frame
		 * \return timestamp in milliseconds (ms)
		 */
		long getTimestamp();

		/*!\brief gets the number of dropped frames for the last frame
		 * \return number of dropped frames
		 */
		int getNumDroppedFrames();

		/*!\brief prints the GUID of attached camera
		 */
		void printGUID();

		/*!\brief prints the GUIDs of connected cameras
		 */
		void printConnectedCams();

		/*!\brief prints the GUID of attached camera
		 */
		long getGUID();
		
		/*!\brief sets the debayering method and pattern
		 * \param method the string name of the method from 
		 * \link cameraconstant.h::bayerMethods \endlink
		 * \param pattern the string name of the pattern from
		 * \link cameraconstant.h::bayerPatterns \endlink
		 * \return 0 if success, <0 if failure
		 */
		int setBayer(const char* method, const char* pattern);

		int setVideoMode(const char*);
		int setFrameRate(float fps);

		void printFrameRate();
		void printVideoMode();

	private:
		uint64_t guid;
		int width;
		int height;
		
		dc1394_t *d;
		dc1394camera_t* cam;
		dc1394color_filter_t bayer_pat;
		dc1394bayer_method_t bayer_met;
		dc1394video_mode_t _video_mode;
		dc1394framerate_t _fps;
		
		long timestamp;
		int droppedframes;

		int initCam(const char* cam_guid);
		int initParam(const char* video_mode, float fps, const char* method, const char* pattern);

		int getBestVideoMode(dc1394video_mode_t*);
		int getBestFrameRate(dc1394framerate_t*, dc1394video_mode_t);

		int convertVideoMode(const char*, dc1394video_mode_t*);
		int checkValidVideoMode(dc1394video_mode_t*);
		void printSupportedVideoModes(dc1394camera_t*);

		int convertFrameRate(float, dc1394framerate_t*);
		int checkValidFrameRate(dc1394framerate_t* frame_rate);
		void printSupportedFrameRates(dc1394camera_t*, dc1394video_mode_t mode);

		int _setVideoMode(const char*);
		int _setFrameRate(float fps);

#ifndef NOOPENCV
		int getOpenCVbits(int, int); 
#endif

		void clean_up();
	};
};
#endif
