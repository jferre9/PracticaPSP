#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


void posicio() {
	int pos, offset;
	int mida;
	int mida_real;
	printf("Pos: ");
	scanf("%d",&pos);
	printf("num caracters: ");
	scanf("%d",&offset);
	char buffer[offset+1];


	FILE *f = fopen("soft.bin","rb");
	if (f == NULL) {
        //codi error
	}

	fseek(f,0,SEEK_END);

	mida = ftell(f);
	//nomes posterior

	if (pos > mida) return; //codi error

	if (pos + offset > mida) mida_real = mida-pos +1;
	else mida_real = offset+1;


	fseek(f,pos,SEEK_SET);


	int num = fread(buffer,sizeof(char),offset,f);
	buffer[num] = '\0';
	printf("He llegit %d: %s\n",num,buffer);



}

void paraula() {




}



int main(int argc, char **argv) {
	//

	printf("1-Posicio\n0-Paraula\n-> ");
	int opcio;
	scanf("%d",&opcio);
	if (opcio) posicio();
	else paraula();
	return 0;
}













