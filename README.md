# Serrebrooke

## testsUnitaires
Dossier contenant les différents codes qui ont servis aux tests des différents capteurs utilisés.
##### BH1715.c
Ce fichier est l'exemple du code utilisé pour la lecture du capteur de luminosité.
##### DS18B20.c
Ce fichier est l'exemple du code utilisé pour la lecture des capteurs de température.


## Code
Dossier contenant le code principal du projet.
##### codeDataCapteurs.c
Ce fichier est le code utilisé par le Pi pour la lecture des différents capteurs.
Le code commence par mettre à jour la date et le temps du Pi au démarrage, puis passe en revue son bus 1-wire.

Après avoir passé sur son bus, nous avons exactement le nombre de capteurs que celui-ci détecte.Par la suite, on alloue de l'espace en mémoire pour garder l'information que nous allons lire sur nos capteurs. Viens ensuite l'initialisation des autres capteurs, soit le capteur d'humidité ambiante ainsi que le capteur de luminosité.

Ensuite, on lis continuellement les capteurs par intervalle choisie. Dans notre cas, on utilise des intervalles de 15 minutes. Donc, à chaque 15 minutes, on effectue la lecture des différents capteurs puis on assemble cette information dans un tableau pour ensuite le faire afficher à l'écran du Pi. En plus d'envoyer l'information à l'écran du Pi, on l'envoie aussi sur le serveur d'AWS (Amazon Web Services), à l'aide d'une clé LTE de la compagnie Hologram.

Une fois rendu sur le site d'hologram, nous avons une route qui envoie l'information dans un bucket S3 sur le serveur AWS. Puis finalement, l'information enregistrée dans ce bucket est mise à notre disposition via un graphique monté via l'application QuickSight.
##### demo
Ceci est le code compilé du projet sous forme d'exécutable.
