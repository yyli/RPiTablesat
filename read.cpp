#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

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

static int16_t swap16(int16_t i) {
    return ((i>>8)&0xff)+((i << 8)&0xff00);
}

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

int main(int argc, char *argv[])
{
    char devbusname[] = "/dev/i2c-1";
    int i2caddr = 0x48;

    int file = open(devbusname, O_RDWR);
    if (file < 0) {
        printf("open %s: error = %d\n", devbusname, file);
        exit(1);
    }
    else
        printf("open %s: succeeded.\n", devbusname);

    if (ioctl(file, I2C_SLAVE, i2caddr) < 0) {
        printf("open i2c slave 0x%02x: error = %s\n\n", i2caddr, "dunno");
        exit(1);
    }
    else
        printf("open i2c slave 0x%02x: succeeded.\n\n", i2caddr);

    // set start single conversion
    uint16_t config;
    int32_t result;
    result = i2c_smbus_read_word_data(file, ADS1115_CONFIG_REGISTER);
    if (result >= 0) {
        config = swap16(result);
    }

    decode_config_register(config);
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

    // check set correctly
    decode_config_register(config);

    // write config
    i2c_smbus_write_word_data(file, ADS1115_CONFIG_REGISTER, swap16(config));

    // read to check
    result = i2c_smbus_read_word_data(file, ADS1115_CONFIG_REGISTER);
    if (result >= 0) {
        config = swap16(result);
    }

    decode_config_register(config);

    double a0 = 0, a1 = 0, a2 = 0, a3 = 0;
    while (1) {
        config &= ~ADS1115_MUX_MASK;
        config |= ADS1115_MUX_AIN0;
        result = i2c_smbus_write_word_data(file, ADS1115_CONFIG_REGISTER, swap16(config));
        usleep(10000);

        result = i2c_smbus_read_word_data(file, ADS1115_CONVERSION_REGISTER);
        result = swap16(result);
        a0 = result*0.0001875;
        // usleep(10000);

        config &= ~ADS1115_MUX_MASK;
        config |= ADS1115_MUX_AIN1;
        result = i2c_smbus_write_word_data(file, ADS1115_CONFIG_REGISTER, swap16(config));
        usleep(10000);

        result = i2c_smbus_read_word_data(file, ADS1115_CONVERSION_REGISTER);
        result = swap16(result);
        a1 = result*0.0001875;
        // usleep(10000);

        config &= ~ADS1115_MUX_MASK;
        config |= ADS1115_MUX_AIN2;
        result = i2c_smbus_write_word_data(file, ADS1115_CONFIG_REGISTER, swap16(config));
        usleep(10000);

        result = i2c_smbus_read_word_data(file, ADS1115_CONVERSION_REGISTER);
        result = swap16(result);
        a2 = result*0.0001875;
        // usleep(10000);

        config &= ~ADS1115_MUX_MASK;
        config |= ADS1115_MUX_AIN3;
        result = i2c_smbus_write_word_data(file, ADS1115_CONFIG_REGISTER, swap16(config));
        usleep(10000);

        result = i2c_smbus_read_word_data(file, ADS1115_CONVERSION_REGISTER);
        result = swap16(result);
        a3 = result*0.0001875;
        // usleep(10000);

        printf("%8.6f %8.6f %8.6f %8.6f\n", a0, a1, a2, a3);
    }

    close(file);
    return 0;
}