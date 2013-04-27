#include <iostream>
#include <unistd.h>
#include "RPiDecode.h"
#include "cv.h"
#include "highgui.h"

using namespace std;

int main(int argc, char *argv[]) {

    if ((RPiDecodeInit(argv[1], atol(argv[2]))) < 0) {
        RPiDecodeDestroyAll();
        return 1;
    }

    double timestamp = 0;

    while (1) {
        cv::Mat temp;
        if (RPiDecodeGetImage(temp, timestamp) < 0) {
            return 1;
        }

        cv::imshow("image", temp);
        cv::waitKey(5);
    }

    RPiDecodeDestroyAll();
}