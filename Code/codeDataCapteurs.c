/**
 * @file   codeLumiereTemperature.cs
 * @author Simon Bradette & Yannick Bergeron-Chartier
 * @date   18-02-2020
 * @brief  Caractérisation d'une serre
 * @version 1.0 : Première version
 * Environnement de développement: Visual Studio Code
 */
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
//Librairie ajoutée pour le timestamp
#include <time.h>
//Librairie ajoutée pour le capteur d'humidité
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
	float luminance = 0;//Pour le luxmètre
	int nbEcriture = 0;

	char dataHologram[devCnt][256];
	char stringEnvoi[2056];

	time_t timeStamp;//Initialisation du timestamp pour l'écriture au début du fichier.(Simon)
	time(&timeStamp);
	time_t timeStampData;//Initialisation du timeStamp pour l'écriture à chaque écriture dans le fichier.(Simon)
	
	
	//Variables pour le capteur d'humidité ambiante
	int16_t i2c_temperature;
	uint16_t i2c_humidity;
	uint8_t err;

	//Code pour l'écriture du nom du fichier (Simon)
	//Inspiration pour le bout de code :
	//https://www.geeksforgeeks.org/time-h-localtime-function-in-c-with-examples/
	struct tm* local;
	time_t t = time(NULL);
	local = localtime(&t);
	strftime(titleTxt, sizeof(titleTxt), "/home/pi/SerresCoaticook/dataCapteurs_%Y-%m-%d_%H_%M_%S.txt", local);//Création du fichier .txt avec la date de création.

	//Écriture de la première ligne dans le fichier .txt (Simon)
    FILE * fp;
    fp = fopen(titleTxt,"w");
	fprintf(fp,"Date de commencement de capture des données : %s\n",ctime(&timeStamp));//Ajout de la date de création au fichier texte.
	fclose(fp);

	//SHT21_Init(SCL_PIN, SDA_PIN);//Initialisation pour le capteur d'humidité

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

	//Début du bloc de code pour l'initialisation du capteur de luminosité I2C.

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
  
	//Fin du bloc de code pour l'initialisation du capteur de luminosité I2C.

	// Read temp continuously
	// Opening the device's file triggers new reading
	while(1) 
	{

		time(&timeStampData);//Mise à jour du timeStamp
		//char dateString[50];
		//snprintf(dateString,sizeof dateString, "%.24s",timeStampData);//Changement de la date pour enlever le \n à la fin.
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

		//Code pour la lecture et l'affichage du capteur de luminosité
		if(read(file, data, 2) != 2)
		{
		printf("Problème avec la lecture du capteur de luminosité \n");
		}
		else
		{
			// Convert the data
			luminance  = (data[0] * 256 + data[1]) / 1.20;
		}

		//Boucle qui permet d'afficher d'un coup les données des capteurs après que la boucle d'acquisition des données soit terminée. (Simon)
		for(int j=0;j<devCnt;j++)
		{

			snprintf(dataHologram[j],sizeof dataHologram, "{ \\\"ID\\\":\\\"%s\\\", \\\"T\\\":\\\"%.1f\\\", \\\"Date\\\":\\\"%s\\\" }", dev[j],tabTemp[j],ctime(&timeStampData));				
		}

		snprintf(dataHologram[devCnt+1],sizeof dataHologram, "{ \\\"ID\\\":\\\"%s\\\", \\\"L\\\":\\\"%.1f\\\", \\\"Date\\\":\\\"%s\\\" }", "Luminosite",luminance,ctime(&timeStampData));
		//Ligne de code qui permet de mettre dans le même tableau de char (string en c) de 5 capteurs.
		snprintf(stringEnvoi,sizeof stringEnvoi, "sudo hologram send  \"[%s, %s, %s, %s, %s]\"",dataHologram[0],dataHologram[1],dataHologram[2],dataHologram[3],dataHologram[4]);

		system(stringEnvoi);//Envoi de la commande par le système (ligne de commande)
		//printf(stringEnvoi);//Ligne pour debug la sortie de la string construite.
		//system("clear");//Ajout du clear pour effacer tout ce qui a sur l'écran. (Simon)

		//Ligne de code qui permet de mettre dans le même tableau de char (string en c) de 5 autres capteurs.
		snprintf(stringEnvoi,sizeof stringEnvoi, "sudo hologram send \"[%s, %s, %s, %s, %s]\"",dataHologram[5],dataHologram[6],dataHologram[7],dataHologram[8],dataHologram[devCnt+1]);
		
		system(stringEnvoi);//Envoi de la commande par le système (ligne de commande)
		//printf(stringEnvoi);//Ligne pour debug la sortie de la string construite.
		system("clear");//Ajout du clear pour effacer tout ce qui a sur l'écran. (Simon)

		printf("Captures commencees le :  %s\n",ctime(&timeStamp));//Affiche à l'écran depuis quand le programme roule. (Simon)

		fp = fopen (titleTxt,"a");//Ouverture du .txt
		fprintf(fp,"TimeStamp : %s",ctime(&timeStampData));//Écriture du timeStamp que les données ont été prises.

		//Pour chaque capteur présent, on écrit dans le fichier texte ainsi qu'à l'écran son numéro et la température.
		for(int j=0;j<devCnt;j++)
		{				
			printf("Device: %s - ", dev[j]);//Affichage du numéro du capteur à l'écran
			printf("Temperature: %.1f C  \n", tabTemp[j]);//Affichage de la température reliée à ce capteur à l'écran

			//Ligne de code permettant d'écrire l'information en format JSON (Simon)
			fprintf (fp, "{ \"ID\":\"%s\", \"T\":\"%.1f\" }\n", dev[j],tabTemp[j]);
		}

		//Écriture de la donnée du capteur de luminosité dans le fichier (Simon)
		fprintf (fp, "Luminosité : %.2f lux\n\n", luminance);

		// Output data to screen (Écriture de la donnée à l'écran)
		printf("Luminosite ambiante : %.2f lux\n", luminance);
   
		// Code pour la lecture du capteur d'humidité ambiante (Yannick)
		//Début du code pour le capteur d'humidité ambiante
		/* Read temperature and humidity from sensor */

		//Ce bout de code fait une segmentation fault, il va falloir règler ça Jeudi. (10-03-20)
		/*err = SHT21_Read(&i2c_temperature, &i2c_humidity);
	
		if (SHT21_Cleanup() != 0)
		{
		printf("ERROR during SHT cleanup\n");
		return -1;
		}
	
		if (err == 0 )
		{
			printf("Humidite ambiante = %.1f%%\n",i2c_humidity/10.0); //Affichage à l'écran de la donnée du capteur d'humidité
			fprintf(fp, "Humidite ambiante = %.1f%%\n",i2c_humidity/10.0); //Écriture de la donnée du capteur d'humidité ambiante dans le fichier

		}
		else
		{
			printf("ERROR 0x%X reading sensor\n", err);
		}*/
		//Fin du bloc de code pour le capteur d'humidité ambiante

		/* close the file*/  
		fclose (fp);
		nbEcriture++;
		printf("Nombre d'ecriture dans le fichier : %d\n",nbEcriture);
		sleep(10);//On arrête pendant 45 minutes. (Simon)
		i = 0;//Reset le compteur qui permet de voir combien de capteurs nous avons lus.
	}
	
	return 0;
}
