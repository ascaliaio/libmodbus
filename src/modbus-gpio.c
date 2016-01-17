/********************************************************************
 * This file is based on:
 * https://developer.ridgerun.com/wiki/index.php/Gpio-int-test.c
 * It is intended to allow use GPIO pins on beaglebone boards instead
 * of RTS for RS485 communication.
 */

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
//#include <poll.h>

#include "modbus-gpio.h"
#include "beaglebone_gpio.h"

/****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	(void)(write(fd, buf, len) + 1);
	close(fd);

	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	(void)(write(fd, buf, len) + 1);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
	if (len < 0)
	{
		return len;
	}

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}

	if (out_flag)
		(void)(write(fd, "out", 4) + 1);
	else
		(void)(write(fd, "in", 3) + 1);

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
volatile void *gpio_addr;
volatile unsigned int *gpio_oe_addr;
volatile unsigned int *gpio_setdataout_addr = NULL;
volatile unsigned int *gpio_cleardataout_addr;
unsigned int reg;
int mem_fd;

int gpio_init() {
    mem_fd = open("/dev/mem", O_RDWR);

    gpio_addr = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO1_START_ADDR);

    gpio_oe_addr = gpio_addr + GPIO_OE;
    gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    if(gpio_addr == MAP_FAILED) {
        perror("Unable to map GPIO\n");
        return -1;
    }

    return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value) {

    if (gpio_setdataout_addr == NULL && gpio_init() != 0) {
        return -1;
    }

    reg = *gpio_oe_addr;

    unsigned int pin = (1<<29);
    reg = reg & (0xFFFFFFFF - pin);
    *gpio_oe_addr = reg;

    if (value)
        *gpio_setdataout_addr = pin;
    else
        *gpio_cleardataout_addr = pin;

    return 0;
}
