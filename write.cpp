#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

using namespace std;

static uint16_t shift(uint16_t i) {
    // [(voltage >> 4) & 0xFF, (voltage << 4) & 0xFF]
    return (((i >> 4) & 0xFF)) + (((i << 4) & 0xFF) << 8);
}

int main(int argc, char *argv[])
{
    char devbusname[] = "/dev/i2c-1";
    int i2caddr = 0x62;

    int fan0 = open(devbusname, O_RDWR);
    if (fan0 < 0) {
        printf("open %s: error = %d\n", devbusname, fan0);
        exit(1);
    }
    else
        printf("open %s: succeeded.\n", devbusname);

    if (ioctl(fan0, I2C_SLAVE, i2caddr) < 0) {
        printf("open i2c slave 0x%02x: error = %s\n\n", i2caddr, "dunno");
        exit(1);
    }
    else
        printf("open i2c slave 0x%02x: succeeded.\n\n", i2caddr);

    int i2caddr1 = 0x63;

    int fan1 = open(devbusname, O_RDWR);
    if (fan1 < 0) {
        printf("open %s: error = %d\n", devbusname, fan1);
        exit(1);
    }
    else
        printf("open %s: succeeded.\n", devbusname);

    if (ioctl(fan1, I2C_SLAVE, i2caddr1) < 0) {
        printf("open i2c slave 0x%02x: error = %s\n\n", i2caddr1, "dunno");
        exit(1);
    }
    else
        printf("open i2c slave 0x%02x: succeeded.\n\n", i2caddr1);

    int16_t volt = 0;
    int fan;
    while (1) {
        cout << "Enter (Fan Voltage): ";
        cin >> fan;
        cin >> volt;
        if (fan != 1 && fan != 0) {
            cout << "Invalid Fan Number (0 or 1 only)" << endl;
            continue;
        }
        if (volt > 4095)
            volt = 4095;
        if (volt < 0)
            volt = 0;
        printf("%3X, %3X, %6.4f\n", shift(volt), volt, volt/4095.0 * 5);
        if (fan == 0)
            i2c_smbus_write_word_data(fan0, 0x40, shift(volt));
        else if (fan == 1)
            i2c_smbus_write_word_data(fan1, 0x40, shift(volt));
        usleep(10000);
    }

    close(fan0);
    close(fan1);
    return 0;
}