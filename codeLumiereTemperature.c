//Librairie de base pour les capteurs de températures DS18B20
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

//Librairies ajoutées pour le capteur de luminosité BH1715
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>



int main (void) 
{
	DIR *dir;
	struct dirent *dirent;
	char buf[256];     // Data from device
	char tmpData[5];   // Temp C * 1000 reported by device 
	const char path[] = "/sys/bus/w1/devices"; 
	ssize_t numRead;
	int i = 0;
	int devCnt = 0;
  
 

	// 1st pass counts devices
	dir = opendir (path);
	if (dir != NULL)
	{
		while ((dirent = readdir (dir))) 
		{
			// 1-wire devices are links beginning with 28-
			if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL) 
			{
				i++;
			}
		}
		(void) closedir (dir);
	}
	else
	{
			perror ("Couldn't open the w1 devices directory");
			return 1;
	}
	devCnt = i;
	i = 0;

	// 2nd pass allocates space for data based on device count
	char dev[devCnt][16];
	char devPath[devCnt][128];
	float tabTemp[devCnt];

	dir = opendir (path);
	if (dir != NULL)
	{
		while ((dirent = readdir (dir))) 
		{
		// 1-wire devices are links beginning with 28-
		if (dirent->d_type == DT_LNK && 
			strstr(dirent->d_name, "28-") != NULL) 
		{ 
			strcpy(dev[i], dirent->d_name);
					// Assemble path to OneWire device
			sprintf(devPath[i], "%s/%s/w1_slave", path, dev[i]);
			i++;
			}
		}
	(void) closedir (dir);
	}
	else
	{
		perror ("Couldn't open the w1 devices directory");
		return 1;
	}
	i = 0;

  	//Création du bus I2C pour le capteur de luminosité.
	int file;
	char *bus = "/dev/i2c-1";
	if ((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, BH1715 I2C address is 0x23(35)
	ioctl(file, I2C_SLAVE, 0x23);

	// Send power on command(0x01)
	char config[1] = {0x01};
	write(file, config, 1);
	// Send continuous measurement command(0x10)
	config[0] = 0x10;
	write(file, config, 1); 
	sleep(1);
  
	char data[2]={0};//Tableau pour le MSB et le LSB de la luminosité.
  
  // Read temp continuously
 // Opening the device's file triggers new reading
 while(1) 
 {
	 //Tant que nous n'avons pas lu tout les capteurs
	while(i != devCnt)
	{
		int fd = open(devPath[i], O_RDONLY);
		if(fd == -1)
		{
		   perror ("Couldn't open the w1 device.");
		   return 1;
		}
		while((numRead = read(fd, buf, 256)) > 0) 
		{
		   strncpy(tmpData, strstr(buf, "t=") + 2, 5);
		   float tempC = strtof(tmpData, NULL);
		   tabTemp[i] = tempC/1000; //On met la température du capteur dans le tableau désigné pour cela.
		}
		close(fd);
		i++;
		

	}
	system("clear");//Ajout du clear de l'écran pour effacer tout ce qui a sur celui-ci.

	//Boucle qui permet d'afficher d'un coup les données des capteurs après que la boucle d'acquisition des données soit terminée.
	for(int j=0;j<i;j++)
	{
		printf("Device: %s - ", dev[j]);//Affichage du numéro du capteur
		printf("Temperature: %.1f C  \n", tabTemp[j]);//Affichage de la température reliée à ce capteur.
	}
	//Code pour la lecture du capteur de luminosité
	if(read(file, data, 2) != 2)
	{
	  printf("Error : Input/Output error \n");
	}
	else
	{
		// Convert the data
		float luminance  = (data[0] * 256 + data[1]) / 1.20;

		// Output data to screen
		printf("Ambient Light Luminance : %.2f lux\n", luminance);
	}
	
	sleep(5);//On arrête pendant 5 secondes pour laisser le temps au client de prendre les données en note.
    i = 0;//Reset le compteur qui permet de voir combien de capteurs nous avons lus.
  }
    return 0;
}
