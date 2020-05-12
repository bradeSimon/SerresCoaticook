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
void updateDateTime();

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
	char data[2]={0};//Tableau pour le MSB et le LSB de la luminositÃ©.
	int nbEcriture = 0;//Pour le nombre de fois que l'on ecrit dans le fichier .txt
	
	updateDateTime();//Fonction qui permet de mettre à jour le temps du Pi si jamais celui-ci est victime d'une panne de courant ou dbrancher pendant un certain temps.
	time_t timeStamp;//Initialisation du timestamp pour l'Ã©criture au dÃ©but du fichier.(Simon)
	time(&timeStamp);
	time_t timeStampData;//Initialisation du timeStamp pour l'Ã©criture Ã  chaque Ã©criture dans le fichier.(Simon)
	char stringEnvoi[2056];
	
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
	strftime(titleTxt, sizeof(titleTxt), "/home/pi/SerresCoaticook/dataCapteurs_%Y-%m-%d_%H:%M:%S.txt", local);//CrÃ©ation du fichier .txt avec la date de crÃ©ation.

	//Ã‰criture de la premiÃ¨re ligne dans le fichier .txt (Simon)
	FILE * filep;
    filep = fopen(titleTxt,"w");
	fprintf(filep,"Date de commencement de capture des données : %s\n",ctime(&timeStamp));//Ajout de la date de crÃ©ation au fichier texte.
	fclose(filep);

	
	//On compte le nombre de capteurs qu'il y a sur notre 1-wire
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

	//On alloue de l'espace en mémoire pour les capteurs selon le nombre de capteur que nous avons. C'est pourquoi on doit déclarer 
	//l'espace mémoire ici et non au tout début du code.
	// 2nd pass allocates space for data based on device count
	char dev[devCnt][16];
	char devPath[devCnt + 2][128];
	float tabTemp[devCnt];
	char dataHologram[devCnt + 2][256];
	
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
	
	//Fin du bloc de code pour l'initialisation du capteur de luminositÃ© I2C.

	// On lis les capteurs en boucle, à une intervalle décidée à la fin de la boucle (sleep)
	// Opening the device's file triggers new reading
	while(1) 
	{
		
		time(&timeStampData);//Mise Ã  jour du timeStamp
		//Tant que nous n'avons pas lu tout les capteurs, on prends la température de chacun d'eux.
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
		/*if(read(file, data, 2) != 2)
		{
		printf("ProblÃ¨me avec la lecture du capteur de luminositÃ© \n");
		}
		else
		{
			// Convert the data
			luminance  = (data[0] * 256 + data[1]) / 1.20;
		}*/

	   	// Code pour la lecture des capteurs d'humidité (Yannick)
		//Début
		/* Read temperature and humidity from sensor */
		err = SHT21_Read(&i2c_temperature, &i2c_humidity);


		//Fin
		//snprintf(dataHologram[devCnt+1],sizeof dataHologram, "{ \\\"ID\\\":\\\"%s\\\", \\\"L\\\":\\\"%.1f\\\", \\\"Date\\\":\\\"%s\\\" }", "Luminosite",luminance,ctime(&timeStampData));
		//Ligne de code qui permet de mettre dans le mÃªme tableau de char (string en c) toutes les donnÃ©es accumulÃ©es.
		
    	snprintf(dataHologram[0],sizeof dataHologram, "{ \\\"T1\\\":\\\"%.1f\\\", \\\"T2\\\":\\\"%.1f\\\", \\\"H\\\":\\\"%.1f\\\", \\\"L\\\":\\\"%.1f\\\", \\\"Date\\\":\\\"%s\\\" }",tabTemp[0], tabTemp[1], (i2c_humidity/10.0), luminance,ctime(&timeStampData));
		snprintf(stringEnvoi,sizeof stringEnvoi, "sudo hologram send  \"[%s]\"",dataHologram[0]);

		//system(stringEnvoi);//Envoi de la commande par le système (ligne de commande)
		printf(stringEnvoi);//Ligne pour debug la sortie de la string construite.
		//system("clear");//Ajout du clear pour effacer tout ce qui a sur l'écran. (Simon)

		printf("Captures commencees le :  %s\n",ctime(&timeStamp));//Affiche Ã  l'Ã©cran depuis quand le programme roule. (Simon)

		filep = fopen (titleTxt,"a");//Ouverture du .txt
		fprintf(filep,"TimeStamp : %s",ctime(&timeStampData));//Ã‰criture du timeStamp que les donnÃ©es ont Ã©tÃ© prises.

		//Pour chaque capteur prÃ©sent, on Ã©crit dans le fichier texte ainsi qu'Ã  l'Ã©cran son numÃ©ro et la tempÃ©rature.
		/*for(int j=0;j<devCnt;j++)
		{
						
			printf("Device: %d - ", j + 1);//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
			printf("Temperature: %.1f C  \n", tabTemp[j]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran

			//Ligne de code permettant d'Ã©crire l'information en format JSON (Simon)
			fprintf (filep, "{ \"ID\":\"%s\", \"T\":\"%.1f\" }\n", dev[j],tabTemp[j]);
      
      
		}*/
		printf("Device: 1 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[0]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[0]);

		printf("Device: 2 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[1]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"2\", \"T\":\"%.1f\" }\n",tabTemp[1]);/*

		printf("Device: 3 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[0]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[0]);

		printf("Device: 4 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[2]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[2]);

		printf("Device: 5 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[6]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[6]);

		printf("Device: 6 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[8]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[8]);

		printf("Device: 7 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[4]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[4]);

		printf("Device: 8 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[7]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[7]);

		printf("Device: 9 - ");//Affichage du numÃ©ro du capteur Ã  l'Ã©cran
		printf("Temperature: %.1f C  \n", tabTemp[5]);//Affichage de la tempÃ©rature reliÃ©e Ã  ce capteur Ã  l'Ã©cran
		fprintf (filep, "{ \"ID\":\"1\", \"T\":\"%.1f\" }\n",tabTemp[5]);*/


		/*//Ã‰criture de la donnÃ©e du capteur de luminositÃ© dans le fichier(Simon)
		fprintf (filep, "LuminositÃ© : %.2f lux\n\n", luminance);
		// Output data to screen (Ã‰criture de la donnÃ©e Ã  l'Ã©cran)
		printf("Luminosite ambiante : %.2f lux\n", luminance);*/

   		if (err == 0 )
		{
	      		printf("Humidite ambiante = %.1f%%\n",i2c_humidity/10.0); //affichage de la lecture du capteur (Yannick)
				fprintf(filep,"Humidite ambiante = %.1f%%\n",i2c_humidity/10.0);
		}
		else
		{
			printf("ERROR 0x%X reading sensor\n", err);
		}

		fclose (filep);//Fermeture du fichier dans lequel on écrit
		nbEcriture++;
		printf("Nombre d'ecriture dans le fichier : %d\n",nbEcriture);
		sleep(9);//On arrÃªte pendant 15 minutes. (Simon)
		i = 0;//Reset la variable qui permet de savoir combien de capteurs nous avons lus.
	}
	
	return 0;
}
/**
 * @brief  Fonction qui sert à mettre à jour la date et l'heure du Pi à l'aide du module LTE.
 */
void updateDateTime()
{
	FILE *fp;
    char var[155];
    char subvarDate[10];
    char subvarTime[10];
    char dateTime[50];
    char datebuff1[20];
    int heure;
    fp = popen("sudo hologram modem location", "r");//Permet d'effectuer la commande hologram modem location qui donne l'heure et la date exacte
    while (fgets(var, sizeof(var), fp) != NULL) 
    {
    }
    pclose(fp);

    //Montage du char* de la date, on doit inverser le format pour avoir le bon (dd-mm-yy doit être yy-mm-dd)
    memcpy(subvarDate, &var[20],10);
    datebuff1[0]=subvarDate[6];
    datebuff1[1]=subvarDate[7];
    datebuff1[2] = '-';
    datebuff1[3]=subvarDate[3];
    datebuff1[4]=subvarDate[4];
    datebuff1[5]='-';
    datebuff1[6]=subvarDate[0];
    datebuff1[7]=subvarDate[1];
    datebuff1[8] = '\0';

    //Montage du char* du temps de la journée.
    memcpy(subvarTime, &var[42],8);
    subvarTime[8] = '\0';

    //Si l'heure se trouve entre 00:00 et 03:00, on change l'heure différament.
    if(subvarTime[0] == '0' && subvarTime[1] == '0')
    {
        subvarTime[0] = '2';
        subvarTime[1] = '0';
    }
    else if(subvarTime[0] == '0' && subvarTime[1] == '1')
    {
        subvarTime[0] = '2';
    }
    else if(subvarTime[0] == '0' && subvarTime[1] == '2')
    {
        subvarTime[0] = '2';  
    }
    else if(subvarTime[0] == '0' && subvarTime[1] == '3')
    {
        subvarTime[0] = '2';        
    }
    else // Sinon, on ne fait que soustraire 4 au chiffre pour avoir la bonne heure.
    {
        heure = (subvarTime[0] - '0')*10;
        heure += (subvarTime[1]- '0');
        heure -= 4;

        subvarTime[1] = heure%10 + '0';
        subvarTime[0] = heure/10 + '0';
    }   
    //Montage du char* final pour avoir la commande complète à envoyer au système.
    snprintf(dateTime,sizeof dateTime, "sudo timedatectl set-time \"%s %s\"",datebuff1,subvarTime);
    
    //Envoi en ligne de commande du char*
    system(dateTime);
}
