/**
 * @file   codeDataCapteurs.cs
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

//Déclaration des fonctions qui sont utilisées dans le code
void updateDateTime();
void constructTimeStamp();
void effaceEcran();

time_t timeStampData;//Initialisation du timeStamp pour l'écriture de celle-ci dans le fichier.
struct tm *info;
char tabDate[30];
int main (void) 
{
	DIR *dir;
	struct dirent *dirent;
	char buf[256];// Buffer pour l'information reçue des capteurs de température.
	char titleTxt[256];//Buffer pour le nom du fichier .txt qui est créé.
	char tmpData[5];// Temp C * 1000 reported by device 
	const char path[] = "/sys/bus/w1/devices";//Le path pour avoir accès aux capteurs de température 
	ssize_t numRead;
	int i = 0;
	int devCnt = 0;
	float luminance = 0;//Float pour la somme du MSB et LSB de la luminosité.
	char data[2]={0};//Tableau pour le MSB et le LSB de la luminosité.
	int nbEcriture = 0;//Pour le nombre de fois que l'on écrit dans le fichier .txt
	int16_t i2c_temperature;
  	uint16_t i2c_humidity;
  	uint8_t err;
	
	
	updateDateTime();//Fonction qui permet de mettre à jour le temps du Pi si jamais celui-ci est victime d'une panne de courant ou est débranché pendant un certain temps.
	time_t timeStamp;//Initialisation du timestamp pour l'écriture au début du fichier.(Simon)
	time(&timeStamp);
	
	char stringEnvoi[2056];
	
  	SHT21_Init(SCL_PIN, SDA_PIN);//Initialisation pour le capteur d'humidité
  


	//Code pour l'écriture du nom du fichier (Simon)
	//Inspiration pour le bout de code :
	//https://www.geeksforgeeks.org/time-h-localtime-function-in-c-with-examples/
	struct tm* local;
	time_t t = time(NULL);
	local = localtime(&t);
	strftime(titleTxt, sizeof(titleTxt), "/home/pi/SerresCoaticook/dataCapteurs_%Y-%m-%d_%H:%M:%S.txt", local);//CrÃ©ation du fichier .txt avec la date de crÃ©ation.

	//Écriture de la première ligne dans le fichier .txt (Simon)
	FILE * filep;
   	filep = fopen(titleTxt,"w");
	fprintf(filep,"Date de commencement de capture des données : %s\n",ctime(&timeStamp));//Ajout de la date de création au fichier texte.
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

	/*On alloue de l'espace en mémoire pour les capteurs selon le nombre de capteur que nous avons. C'est pourquoi on doit déclarer 
	l'espace mémoire ici et non au tout début du code.*/
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
	//Fin du bloc de code pour l'initialisation du capteur de luminositÃ© I2C.

	// On lis les capteurs en boucle, à une intervalle décidée à la fin de la boucle (sleep)
	// Opening the device's file triggers new reading (Capteurs de température)
	while(1) 
	{
		constructTimeStamp();//Mise à jour d'un marqueur de temps pour savoir quand les données ont été prises.
		
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
				tabTemp[i] = tempC/1000; //On met la température du capteur dans le tableau désigné pour cela.


			}


			close(fd);
			i++;
			

		}

		//Code pour la lecture et l'affichage du capteur de luminosité
		if(read(file, data, 2) != 2)
		{
			printf("Probleme avec la lecture du capteur de luminosite \n");
		}
		else
		{
			//Conversion en lux du MSB et LSB reçus.
			luminance  = (data[0] * 256 + data[1]) / 1.20;
		}
		//Fin
		// Code pour la lecture des capteurs d'humidité (Yannick)
		err = SHT21_Read(&i2c_temperature, &i2c_humidity);
		//Longue ligne qui permet de monter la trame JSON qui sera envoyée sur Hologram.
    	snprintf(dataHologram[0],sizeof dataHologram, "{ \\\"T1\\\":\\\"%.1f\\\", \\\"T2\\\":\\\"%.1f\\\", \\\"H\\\":\\\"%.1f\\\", \\\"L\\\":\\\"%.1f\\\", \\\"Date\\\":\\\"%s\\\" }",tabTemp[0], tabTemp[1], (i2c_humidity/10.0), luminance,tabDate);
		snprintf(stringEnvoi,sizeof stringEnvoi, "sudo hologram send  \"[%s]\"",dataHologram[0]);

		system(stringEnvoi);//Envoi de la commande par le système par ligne de commande.
		effaceEcran();//On efface l'écran pour faire place aux nouvelles données.

		//Section pour l'écriture à l'écran LCD (Simon)
		printf("Captures commencees le :  %s\n",ctime(&timeStamp));//Affiche à  l'écran depuis quand le programme roule. (Simon)
		printf("Capteur: 1 - ");//Affichage du numéro du capteur à l'écran
		printf("Temperature: %.1f C  \n", tabTemp[0]);//Affichage de la température reliée à ce capteur à  l'écran
		printf("Capteur: 2 - ");//Affichage du numéro du capteur à  l'écran
		printf("Temperature: %.1f C  \n", tabTemp[1]);//Affichage de la température reliée à ce capteur à  l'écran
		printf("Luminosite ambiante : %.2f lux\n", luminance);//Affichage de la luminosité à l'écran
		
		
		//Section pour l'écriture dans le fichier .txt (Simon)
		filep = fopen (titleTxt,"a");//Ouverture du .txt pour ajouter du texte
		fprintf(filep,"TimeStamp : %s\n",tabDate);//Écriture dans le fichier .txt du timeStamp auquel les données ont été prises.
		fprintf (filep, "Température capteur 1 : %.1f\n",tabTemp[0]);//Écriture dans le fichier .txt
		fprintf (filep, "Température capteur 2 : %.1f\n",tabTemp[1]);//Écriture dans le fichier .txt
		fprintf (filep, "Luminosité : %.1f lux\n\n", luminance);////Écriture de la donnée du capteur de luminosité dans le fichier.
		
		//Si la lecture du capteur d'humidité c'est bien passée, il écrira à l'écran et dans le fichier. Sinon il nous dira le type d'erreur.
   		if (err == 0 )
		{
	      	printf("Humidite ambiante = %.1f%%\n",i2c_humidity/10.0); //Affichage à l'écran de l'humidité ambiante.
			fprintf(filep,"Humidité ambiante = %.1f%%\n\n",i2c_humidity/10.0);//Écriture de l'humidité dans le fichier .txt
		}
		else
		{
			printf("ERROR 0x%X reading sensor\n", err);
			fprintf(filep,"Problème avec le capteur d'humidité\n\n");//Écriture dans le fichier .txt
		}

		fclose (filep);//Fermeture du fichier dans lequel on écrit
		nbEcriture++;//Incrémentation du nombre de fois que nous avons écris dans le fichier.
		printf("Nombre d'ecriture dans le fichier : %d\n",nbEcriture);//Affichage à l'écran du nombre de fois que nous avons écris dans le fichier.
		sleep(900);//On arrête pendant 15 minutes. (Simon)
		i = 0;//Reset la variable qui permet de savoir combien de capteurs nous avons lus.
	}
	
	return 0;
}
/**
 * @brief  Fonction qui sert à mettre à jour la date et l'heure du Pi à l'aide du module LTE.
 **/
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
	while (fgets(var, sizeof(var), fp) != NULL)//On met l'information reçue par la commande dans un tableau de char.
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
    else //Sinon, on ne fait que soustraire 4 au chiffre pour avoir la bonne heure.
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
/**
 * @brief  Fonction qui sert à utiliser le modèle dd/mm/yyyy hh:mm:ss pour le timestamp de QuickSight
 **/
void constructTimeStamp()
{
	time(&timeStampData);
	info = localtime(&timeStampData);

	strftime(tabDate,sizeof tabDate,"%d/%b/%Y %X\0",info);//Montage de la string par rapport aux informations du temps obtenu.

}
/**
 * @brief  Fonction qui sert à effacer complètement le terminal qui se trouve sur l'écran du Pi.
 **/
void effaceEcran()
{
	system("clear");
}
