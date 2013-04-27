#include <iostream>
#include <unistd.h>
#include "RPiDecode.h"
#include "cv.h"
#include "highgui.h"

using namespace std;

int main(int argc, char *argv[]) {
    UDTSOCKET sock;

    if ((RPiDecodeInit(argv[1], atol(argv[2]))) < 0) {
        RPiDecodeDestroyAll();
        return 1;
    }

    int height = RPiDecodeGetHeight();
    int width = RPiDecodeGetWidth();

    double timestamp = 0;

    cv::Mat previmage;
    cv::Mat curimage;
    cv::Mat flow;
    std::vector<cv::Point2f> corners;
    std::vector<unsigned char> status;
    cv::Mat err;
    std::vector<cv::Point2f> output;
    cv::Mat converted;

    int start = 0;

    while (1) {
        cv::Mat temp;
        if (RPiDecodeGetImage(temp, timestamp) < 0) {
            return 1;
        }

        // assert(im != NULL);

        // cv::Mat temp(height, width, CV_8UC3, im->data);
        // cv::cvtColor(temp, converted, CV_RGB2GRAY);
        // curimage.copyTo(previmage);
        // converted.copyTo(curimage);

        // if (start > 0) {
        //     cv::goodFeaturesToTrack(previmage, corners, 400, 0.1, 0.1);

        //     cv::calcOpticalFlowPyrLK(previmage, curimage, corners, output, status, err);

        //     double xdiff = 0;
        //     double ydiff = 0;
        //     int num = 0;
        //     for (int i = 0; i < 400; i++) {
        //         if (status[i] == 1) {
        //             num++;
        //             xdiff += output[i].x - corners[i].x;
        //             ydiff += output[i].y - corners[i].y;
        //             //printf("(%6.4f %6.4f) -> (%6.4f %6.4f)\n", corners[i].x, corners[i].y, output[i].x, output[i].y);
        //         }
        //     }
        //     if (num > 100) {
        //         printf("%15.6f %09.6f %09.6f\n", timestamp, xdiff/num, ydiff/num);
        //         fprintf(stderr, "%15.6f %09.6f %09.6f\n", timestamp, xdiff/num, ydiff/num);
        //     }
        // }

        // start++;

        cv::imshow("image", temp);
        cv::waitKey(5);
    }

    RPiDecodeDestroyAll();
}