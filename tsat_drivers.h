#ifndef TSAT_DRIVERS_H
#define TSAT_DRIVERS_H

// initialize the i2c connections
int tsat_init();

// returns the double from the channel
int tsat_read(double (&values)[8]);

// write the value to the channel
int tsat_write(int channel, double value);

// close the i2c connections
int tsat_deinit();

#endif