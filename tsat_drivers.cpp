#include "tsat_drivers.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

// file descriptors
static int ADC1FD = 0;
static int ADC2FD = 0;
static int DAC1FD = 0;
static int DAC2FD = 0;

// slave addresses
static int ADC1ADDR = 0x48;
static int ADC2ADDR = 0x49;
static int DAC1ADDR = 0x62;
static int DAC2ADDR = 0x63;

// DA write address
static int DACWRITEADDR = 0x40;

// bus name
static const char* BUSNAME = "/dev/i2c-1";

// bool to tell if initializeds
static int initbool = 0;

// minimul
static int MINDACVAL = 700;

// ADC defines
#define ADS1115_I2C_ADDRESSES           { 0x48, 0x49, 0x4A, 0x4 };

#define ADS1115_CONVERSION_REGISTER     0x00
#define ADS1115_CONFIG_REGISTER         0x01
#define ADS1115_LO_THRESH_REGISTER      0x02
#define ADS1115_HI_THRESH_REGISTER      0x03

#define ADS1115_START_SINGLE_CONVERSION     (0x01 << 15)
#define ADS1115_CONVERSION_STATUS_MASK      (0x01 << 15)

#define ADS1115_MUX_MASK            (0x07 << 12)
#define ADS1115_MUX_AIN0_AIN1           (0x00 << 12)
#define ADS1115_MUX_AIN0_AIN3           (0x01 << 12)
#define ADS1115_MUX_AIN1_AIN3           (0x02 << 12)
#define ADS1115_MUX_AIN2_AIN3           (0x03 << 12)
#define ADS1115_MUX_AIN0            (0x04 << 12)
#define ADS1115_MUX_AIN1            (0x05 << 12)
#define ADS1115_MUX_AIN2            (0x06 << 12)
#define ADS1115_MUX_AIN3            (0x07 << 12)

#define ADS1115_PGA_MASK            (0x07 << 9)
#define ADS1115_PGA_6144            (0x00 << 9)
#define ADS1115_PGA_4096            (0x01 << 9)
#define ADS1115_PGA_2048            (0x02 << 9)
#define ADS1115_PGA_1024            (0x03 << 9)
#define ADS1115_PGA_0512            (0x04 << 9)
#define ADS1115_PGA_0256            (0x05 << 9)

#define ADS1115_CONVERSION_MODE_MASK        (0x01 << 8)
#define ADS1115_CONVERSION_CONTINUOUS       (0x00 << 8)
#define ADS1115_CONVERSION_SINGLESHOT       (0x01 << 8)

#define ADS1115_RATE_MASK           (0x07 << 5)
#define ADS1115_RATE_8              (0x00 << 5)
#define ADS1115_RATE_16             (0x01 << 5)
#define ADS1115_RATE_32             (0x02 << 5)
#define ADS1115_RATE_64             (0x03 << 5)
#define ADS1115_RATE_128            (0x04 << 5)
#define ADS1115_RATE_250            (0x05 << 5)
#define ADS1115_RATE_475            (0x06 << 5)
#define ADS1115_RATE_860            (0x07 << 5)

#define ADS1115_COMPARATOR_WINDOW_MASK      (0x01 << 4)
#define ADS1115_COMPARATOR_WINDOW       (0x01 << 4)

#define ADS1115_COMPARATOR_ACTIVE_MASK      (0x01 << 3)
#define ADS1115_COMPARATOR_ACTIVE_HIGH      (0x01 << 3)

#define ADS1115_COMPARATOR_LATCHING_MASK    (0x01 << 2)
#define ADS1115_COMPARATOR_LATCHING     (0x01 << 2)

#define ADS1115_COMPARATOR_QUEUE_MASK       (0x03 << 0)
#define ADS1115_COMPARATOR_QUEUE1       (0x00 << 0)
#define ADS1115_COMPARATOR_QUEUE2       (0x01 << 0)
#define ADS1115_COMPARATOR_QUEUE4       (0x02 << 0)
#define ADS1115_COMPARATOR_OFF          (0x03 << 0)

// ADC config
static uint16_t config;

// static function for swaping
static int16_t swap16(int16_t i) {
    return ((i>>8)&0xff)+((i << 8)&0xff00);
}

// static function for shifting to DAC
static uint16_t shift(uint16_t i) {
    // [(voltage >> 4) & 0xFF, (voltage << 4) & 0xFF]
    return (((i >> 4) & 0xFF)) + (((i << 4) & 0xFF) << 8);
}

// decode function for debugging
void decode_config_register(uint16_t config) {
    printf("\nConfig register: 0x%x\n", config);
    if (config & ADS1115_CONVERSION_STATUS_MASK) {
        printf("Device idle\n");
    } else {
        printf("Conversion in progress\n");
    }
    printf("Input MUX: ");
    switch (config & ADS1115_MUX_MASK) {
        case ADS1115_MUX_AIN0: printf("0"); break;
        case ADS1115_MUX_AIN1: printf("1"); break;
        case ADS1115_MUX_AIN2: printf("2"); break;
        case ADS1115_MUX_AIN3: printf("3"); break;
        case ADS1115_MUX_AIN0_AIN1:   printf("0-1"); break;
        case ADS1115_MUX_AIN0_AIN3:   printf("0-3"); break;
        case ADS1115_MUX_AIN1_AIN3:   printf("1-3"); break;
        case ADS1115_MUX_AIN2_AIN3:   printf("2-3"); break;
        default:          printf("unknown (0x%x)", config & ADS1115_MUX_MASK); break;
    }
    printf("\n");
    printf("PGA: ");
    switch (config & ADS1115_PGA_MASK) {
      case ADS1115_PGA_6144:    printf("6144"); break;
      case ADS1115_PGA_4096:    printf("4096"); break;
      case ADS1115_PGA_2048:    printf("2048"); break;
      case ADS1115_PGA_1024:    printf("1024"); break;
      case ADS1115_PGA_0512:    printf("512"); break;
      case ADS1115_PGA_0256:    printf("256"); break;
      default:          printf("unknown (0x%x)", config & ADS1115_PGA_MASK); break;
    }
    printf("\n");
    printf("Conversion mode: ");
    switch (config & ADS1115_CONVERSION_MODE_MASK) {
      case ADS1115_CONVERSION_CONTINUOUS:   printf("continuous"); break;
      case ADS1115_CONVERSION_SINGLESHOT:   printf("singleshot"); break;
      default:              printf("unknown (0x%x)", config & ADS1115_CONVERSION_MODE_MASK); break;
    }
    printf("\n");
    printf("Conversion rate: ");
    switch (config & ADS1115_RATE_MASK) {
      case ADS1115_RATE_8:      printf("8"); break;
      case ADS1115_RATE_16:     printf("16"); break;
      case ADS1115_RATE_32:     printf("32"); break;
      case ADS1115_RATE_64:     printf("64"); break;
      case ADS1115_RATE_128:    printf("128"); break;
      case ADS1115_RATE_250:    printf("250"); break;
      case ADS1115_RATE_475:    printf("475"); break;
      case ADS1115_RATE_860:    printf("860"); break;
      default:          printf("unknown (0x%x)", config & ADS1115_RATE_MASK);
    }
    printf("\n");
}


// initialize the i2c connections
int tsat_init() {
    // open file descriptors
    ADC1FD = open(BUSNAME, O_RDWR);
    if (ADC1FD < 0) {
        fprintf(stderr, "Failed to open i2c bus: %s for ADC1\n", BUSNAME);
        return -1;
    }

    ADC2FD = open(BUSNAME, O_RDWR);
    if (ADC2FD < 0) {
        fprintf(stderr, "Failed to open i2c bus: %s for ADC2\n", BUSNAME);
        return -1;
    }

    DAC1FD = open(BUSNAME, O_RDWR);
    if (DAC1FD < 0) {
        fprintf(stderr, "Failed to open i2c bus: %s for DAC1\n", BUSNAME);
        return -1;
    }

    DAC2FD = open(BUSNAME, O_RDWR);
    if (DAC2FD < 0) {
        fprintf(stderr, "Failed to open i2c bus: %s for DAC2\n", BUSNAME);
        return -1;
    }

    // set slave addrs 
    if (ioctl(ADC1FD, I2C_SLAVE, ADC1ADDR) < 0) {
        fprintf(stderr, "Failed to open i2c slave 0x%02x\n", ADC1ADDR);
        return -2;
    }
    if (ioctl(ADC2FD, I2C_SLAVE, ADC2ADDR) < 0) {
        fprintf(stderr, "Failed to open i2c slave 0x%02x\n", ADC2ADDR);
        return -2;
    }
    if (ioctl(DAC1FD, I2C_SLAVE, DAC1ADDR) < 0) {
        fprintf(stderr, "Failed to open i2c slave 0x%02x\n", DAC1ADDR);
        return -2;
    }
    if (ioctl(DAC2FD, I2C_SLAVE, DAC2ADDR) < 0) {
        fprintf(stderr, "Failed to open i2c slave 0x%02x\n", DAC2ADDR);
        return -2;
    }

    // set ADC settings
    int32_t result;
    result = i2c_smbus_read_word_data(ADC1FD, ADS1115_CONFIG_REGISTER);
    if (result >= 0) {
        config = swap16(result);
    }

    // decode_config_register(config);
    config |= ADS1115_START_SINGLE_CONVERSION;

    // set gain
    config &= ~ADS1115_PGA_MASK;
    config |= ADS1115_PGA_6144;

    // set rate
    config &= ~ADS1115_RATE_MASK;
    config |= ADS1115_RATE_860;

    // set continuous
    config &= ~ADS1115_CONVERSION_SINGLESHOT;
    config |= ADS1115_CONVERSION_CONTINUOUS;

    // write config
    i2c_smbus_write_word_data(ADC1FD, ADS1115_CONFIG_REGISTER, swap16(config));
    i2c_smbus_write_word_data(ADC2FD, ADS1115_CONFIG_REGISTER, swap16(config));

    initbool = 1;
    return 0;
}

// returns the double from the channel
int tsat_read(double (&values)[8]) {
    if (initbool == 0)
        return -1;

    int32_t result;

    // read
    config &= ~ADS1115_MUX_MASK;
    config |= ADS1115_MUX_AIN0;
    i2c_smbus_write_word_data(ADC1FD, ADS1115_CONFIG_REGISTER, swap16(config));
    i2c_smbus_write_word_data(ADC2FD, ADS1115_CONFIG_REGISTER, swap16(config));
    usleep((1/860.0 + 0.001)*1000000);
    result = i2c_smbus_read_word_data(ADC1FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[0] = result*0.0001875;
    result = i2c_smbus_read_word_data(ADC2FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[4] = result*0.0001875;

    config &= ~ADS1115_MUX_MASK;
    config |= ADS1115_MUX_AIN1;
    i2c_smbus_write_word_data(ADC1FD, ADS1115_CONFIG_REGISTER, swap16(config));
    i2c_smbus_write_word_data(ADC2FD, ADS1115_CONFIG_REGISTER, swap16(config));
    usleep((1/860.0 + 0.001)*1000000);
    result = i2c_smbus_read_word_data(ADC1FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[1] = result*0.0001875;
    result = i2c_smbus_read_word_data(ADC2FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[5] = result*0.0001875;

    config &= ~ADS1115_MUX_MASK;
    config |= ADS1115_MUX_AIN2;
    i2c_smbus_write_word_data(ADC1FD, ADS1115_CONFIG_REGISTER, swap16(config));
    i2c_smbus_write_word_data(ADC2FD, ADS1115_CONFIG_REGISTER, swap16(config));
    usleep((1/860.0 + 0.001)*1000000);
    result = i2c_smbus_read_word_data(ADC1FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[2] = result*0.0001875;
    result = i2c_smbus_read_word_data(ADC2FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[6] = result*0.0001875;

    config &= ~ADS1115_MUX_MASK;
    config |= ADS1115_MUX_AIN3;
    i2c_smbus_write_word_data(ADC1FD, ADS1115_CONFIG_REGISTER, swap16(config));
    i2c_smbus_write_word_data(ADC2FD, ADS1115_CONFIG_REGISTER, swap16(config));
    usleep((1/860.0 + 0.001)*1000000);
    result = i2c_smbus_read_word_data(ADC1FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[3] = result*0.0001875;
    result = i2c_smbus_read_word_data(ADC2FD, ADS1115_CONVERSION_REGISTER);
    result = swap16(result);
    values[7] = result*0.0001875;

    return 0;
}

// write the value to the channel
int tsat_write(int channel, double value) {
    int volt = value/10.0 * (4095 - MINDACVAL) + MINDACVAL;
    if (volt > 4095)
        volt = 4095;
    if (volt < MINDACVAL)
        volt = MINDACVAL;
    if (channel == 0)
        i2c_smbus_write_word_data(DAC1FD, DACWRITEADDR, shift(volt));
    else if (channel == 1)
        i2c_smbus_write_word_data(DAC2FD, DACWRITEADDR, shift(volt));
    else {
        fprintf(stderr, "Invalid channel number, must be (0 or 1)\n");
        return -1;
    }
    return 0;   
}

// close the i2c connections
int tsat_deinit() {
    close(ADC1FD);
    close(ADC2FD);
    close(DAC1FD);
    close(DAC2FD);

    return 0;
}