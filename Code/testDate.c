#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main()
{
    FILE *fp,*outputfile;
    char var[300];
    char subvarDate[20];
    char subvarTime[20];



    fp = popen("sudo hologram modem location", "r");
    while (fgets(var, sizeof(var), fp) != NULL) 
        {
        //printf("%s", var);
        }
    pclose(fp);
    printf("%s\n",var);

    memcpy(subvarDate, &var[20],10);
    memcpy(subvarTime, &var[42],8);
    subvarDate[10] = '\0';
    subvarTime[8] = '\0';
    printf("%s\n",subvarDate);
    printf("%s\n",subvarTime);
    outputfile = fopen("text.txt", "a");
    fprintf(outputfile,"%s\n",var);
    fclose(outputfile);

    return 0;
}