#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

Adafruit_seesaw ss;

void main()
{
    	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if ((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}

    // Get I2C device, SoilSensor I2C address is 0x36
	ioctl(file, I2C_SLAVE, 0x36);

    
}