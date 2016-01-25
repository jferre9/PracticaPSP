#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TEMPS		10
#define CARACTERS	20


//Programa que cada TEMPS segons escriu CARACTERS caracters abans i despres d'una paraula en un fitxer
int main (int argc, char **argv) {

	char *paraules[] = {"CONTRASENYA", "USUARI", "WEB"};



	srand(time(NULL));

	while (1) {
		FILE *f = fopen("soft.bin", "ab");
		if (f == NULL) {
            return (-1);
		}
		int i;
		char s[CARACTERS];
		for (i = 0; i < CARACTERS;i++) {
			s[i] = 'A' + (rand() % 26);
			if (rand()%2) s[i] += 'a'-'A' ;

		}
		fwrite(s, sizeof(char),CARACTERS,f);

		int p = rand()%3;

		fwrite(paraules[p],sizeof(char),strlen(paraules[p]),f);


		for (i = 0; i < CARACTERS;i++) {
			s[i] = 'A' + (rand() % 26);
			if (rand()%2) s[i] += 'a'-'A' ;
		}
		fwrite(s, sizeof(char),CARACTERS,f);


		fclose(f);
		sleep(TEMPS);

	}
	return 0;
}


