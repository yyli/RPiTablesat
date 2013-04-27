#include <iostream>
#include <unistd.h>
#include "RPiDecode.h"
#include "cv.h"
#include "highgui.h"

using namespace std;

int main(int argc, char *argv[]) {
    UDTSOCKET sock;

    if ((sock = RPiDecodeInit(argv[1], atol(argv[2]))) < 0) {
        RPiDecodeDestroyAll();
        return 1;
    }

    int height = RPiDecodeGetHeight();
    int width = RPiDecodeGetWidth();

    Image *im = NULL;
    double timestamp = 0;

    cout << width << " " << height <<  endl;

    while (1) {
        if (RPiDecodeGetImage(sock, &im, timestamp) < 0) {
            return 1;
        }

        assert(im != NULL);

        cout << timestamp << endl;

        cv::Mat img(height, width, CV_8UC3, im->data);
        cv::imshow("image", img);
        cv::waitKey(5);
    }

    RPiDecodeDestroyAll();
}