# Serrebrooke

## testsUnitaires
Dossier contenant les différents codes qui ont servis aux tests des différents capteurs utilisés.
##### BH1715.c
Ce fichier est l'exemple du code utilisé pour la lecture du capteur de luminosité.
##### DS18B20.c
Ce fichier est l'exemple du code utilisé pour la lecture des capteurs de température.

## codeDataCapteurs.c
Ce fichier est le code utilisé par le Pi pour la lecture des différents capteurs. Pour l'instant, il y a que le code pour les capteurs de température ainsi que le capteur de luminosité.


## Code
Dossier contenant le code principal du projet.
##### codeDataCapteurs.c
Ce fichier est le code utilisé par le Pi pour la lecture des différents capteurs. Pour l'instant, il y a que le code pour les capteurs de température ainsi que le capteur de luminosité.

* 11-02-20 : Ajout du code pour le timestamp au début du programme.

* 18-02-20 : -Ajout du code pour le timestamp à chaque capture de données et celui dans le titre du fichier texte à sa création.

* 20-02-20 : -Ajout du code pour l'envoi des données par LTE sur la plateforme Hologram. Cette plateforme envoie ensuite les données vers un bucket S3 sur Amazon.
