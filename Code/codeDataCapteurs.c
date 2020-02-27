/**
 * @file   codeLumiereTemperature.cs
 * @author Simon Bradette & Yannick Bergeron-Chartier
 * @date   18-02-2020
 * @brief  CaractÃ©risation d'une serre
 * @version 1.0 : PremiÃ¨re version
 * Environnement de dÃ©veloppement: Visual Studio Code
 */
//Librairie de base pour les capteurs de tempÃ©ratures DS18B20
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

//Librairies ajoutÃ©es pour le capteur de luminositÃ© BH1715
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
//Librairie ajoutÃ©e pour le timestamp
#include <time.h>
//Librairie ajoutÃ©e pour le capteur d'humidité 
#include <stdint.h>
#include "sht21.h"
//Define des PIN pour la lecture du capteur d'humidité
#define SDA_PIN 2
#define SCL_PIN 3


int main (void) 
{
	DIR *dir;
	struct dirent *dirent;
	char buf[256];     // Data from device
	char titleTxt[256];
	char tmpData[5];   // Temp C * 1000 reported by device 
	const char path[] = "/sys/bus/w1/devices"; 
	ssize_t numRead;
	int i = 0;
	int devCnt = 0;
	float luminance = 0;//Pour le luxmÃ¨tre
	int nbEcriture = 0;

	time_t timeStamp;//Initialisation du timestamp pour l'Ã©criture au dÃ©but du fichier.(Simon)
	time(&timeStamp);
	time_t timeStampData;//Initialisation du timeStamp pour l'Ã©criture Ã  chaque Ã©criture dans le fichier.(Simon)
	
  SHT21_Init(SCL_PIN, SDA_PIN);//Initialisation pour le capteur d'humidité
  
  int16_t i2c_temperature;
  uint16_t i2c_humidity;
  uint8_t err;

	//Code pour l'Ã©criture du nom du fichier (Simon)
	//Inspiration pour le bout de code :
	//https://www.geeksforgeeks.org/time-h-localtime-function-in-c-with-examples/
	struct tm* local;
	time_t t = time(NULL);
	local = localtime(&t);
	strftime(titleTxt, sizeof(titleTxt), "/home/pi/Documents/projetSerres/dataCapteurs_%Y-%m-%d_%H:%M:%S.txt", local);//CrÃ©ation du fichier .txt avec la date de crÃ©ation.

	//Ã‰criture de la premiÃ¨re ligne dans le fichier .txt (Simon)
    FILE * fp;
    fp = fopen(titleTxt,"w");
	fprintf(fp,"Date de commencement de capture des donnÃ©es : %s\n",ctime(&timeStamp));//Ajout de la date de crÃ©ation au fichier texte.
	fclose(fp);


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
	char dataHologram[devCnt][256];

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

	//DÃ©but du bloc de code pour l'initialisation du capteur de luminositÃ© I2C.

  	//CrÃ©ation du bus I2C pour le capteur de luminositÃ©.
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
  
	char data[2]={0};//Tableau pour le MSB et le LSB de la luminositÃ©.
  
	//Fin du bloc de code pour l'initialisation du capteur de luminositÃ© I2C.

	// Read temp continuously
	// Opening the device's file triggers new reading
	while(1) 
	{
		//Tant que nous n'avons pas lu tout les capteurs
		time(&timeStampData);//Mise Ã  jour du timeStamp
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
				tabTemp[i] = tempC/1000; //On met la tempÃ©rature du capteur dans le tableau dÃ©signÃ© pour cela.


			}


			close(fd);
			i++;
			

		}

		//Code pour la lecture et l'affichage du capteur de luminositÃ©
		if(read(file, data, 2) != 2)
		{
		printf("ProblÃ¨me avec la lecture du capteur de luminositÃ© \n");
		}
		else
		{
			// Convert the data
			luminance  = (data[0] * 256 + data[1]) / 1.20;
		}


    
		

		//Boucle qui permet d'afficher d'un coup les donnÃ©es des capteurs aprÃ¨s que la boucle d'acquisition des donnÃ©es soit terminÃ©e. (Simon)
		for(int j=0;j<devCnt;j++)
		{
			snprintf(dataHologram[j],sizeof dataHologram, "sudo hologram send \"{ \\\"ID\\\":\\\"%s\\\", \\\"T\\\":\\\"%.1f\\\" }\"", dev[j],tabTemp[j]);				
		}

		//Ã€ mettre avant l'affichage car la commande affiche s'il a rÃ©ussi ou non.
		for(int j=0;j<devCnt;j++)
		{
			system(dataHologram[j]);
			
		}

		system("clear");//Ajout du clear pour effacer tout ce qui a sur l'Ã©cran. (Simon)

		printf("Captures commencees le :  %s\n",ctime(&timeStamp));//Affiche Ã  l'Ã©cran depuis quand le programme roule. (Simon)

		fp = fopen (titleTxt,"a");//Ouverture du .txt
		fprintf(fp,"TimeStamp : %s",ctime(&timeStampData));//Ã‰criture du timeStamp que les donnÃ©es ont Ã©tÃ© prises.

		//Pour chaque capteur prÃ©sent, on Ã©crit dans le fichier texte ainsi qu'Ã  l'Ã©cran son numÃ©ro et la tempÃ©rature.
		for(int j=0;j<devCnt;j++)
		{
						
			printf("Device: %s - ", dev[j]);//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
			printf("Temperature: %.1f C  \n", tabTemp[j]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran

			//Ligne de code permettant d'Ã©crire l'information en format JSON (Simon)
			fprintf (fp, "{ \"ID\":\"%s\", \"T\":\"%.1f\" }\n", dev[j],tabTemp[j]);
		}

		//Ã‰criture de la donnÃ©e du capteur de luminositÃ© dans le fichier(Simon)
		fprintf (fp, "LuminositÃ© : %.2f lux\n\n", luminance);
		// Output data to screen (Ã‰criture de la donnÃ©e Ã  l'Ã©cran)
		printf("Luminosite ambiante : %.2f lux\n", luminance);
   
   // Code pour la lecture des capteurs d'humidité (Yannick)
    //Début
    /* Read temperature and humidity from sensor */
    err = SHT21_Read(&i2c_temperature, &i2c_humidity);
   
    if (SHT21_Cleanup() != 0)
    {
      printf("ERROR during SHT cleanup\n");
      return -1;
    }
   
    if (err == 0 )
    {
      printf("Humidite ambiante = %.1f%%\n",i2c_humidity/10.0); //affichage de la lecture du capteur (Yannick)
    }
    else
    {
      printf("ERROR 0x%X reading sensor\n", err);
    }
    //Fin


		/* close the file*/  
		fclose (fp);
		nbEcriture++;
		printf("Nombre d'ecriture dans le fichier : %d\n",nbEcriture);
		sleep(600);//On arrÃªte pendant 10 minutes. (Simon)
		i = 0;//Reset le compteur qui permet de voir combien de capteurs nous avons lus.
	}
	
	return 0;
}
