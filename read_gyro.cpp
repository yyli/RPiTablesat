#include <cstdio>
#include <unistd.h>
#include "tsat_drivers.h"

int main() {
    tsat_init();
    while(1) {
        // initialize a array of 8 (0 = gyro, 1-3 (xyz magnetometer), 4-7 (sun sensors))
        double values[8] = {0};
        // read the values from ADC
        tsat_read(values);

        for (int i = 0; i < 8; i++) {
            printf("%09.6f ", values[i]);
        }
        printf("\n");
        usleep(1/100.0*(1000000));
    }
    tsat_deinit();
}