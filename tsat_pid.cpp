#include <cstdio>
#include <unistd.h>
#include "tsat_drivers.h"

int main() {
    tsat_init();
    double input, output, error;
    double kp = 1000;
    double setpoint = 1;
    while(1) {
        double values[8] = {0};
        tsat_read(values);
        input = values[0];
        error = setpoint - input;
        output = kp*error;
        if (output > 0) {
            tsat_write(0, output);
            tsat_write(1, 0);
        } else {
            tsat_write(0, 0);
            tsat_write(1, -1*output);
        }
        printf("%+8.6f %+8.6f %+8.6f %+8.6f\n", setpoint, input, output, error);
        usleep(1/100.0*(1000000));
    }

    tsat_deinit();
}