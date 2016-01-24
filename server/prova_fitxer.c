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
        return;
	}

	fseek(f,0,SEEK_END);

	mida = ftell(f);
	//nomes posterior

	if (pos > mida) return; //codi error




	fseek(f,pos,SEEK_SET);


	int num = fread(buffer,sizeof(char),offset,f);
	buffer[num] = '\0';
	printf("He llegit %d: %s\n",num,buffer);



}

void paraula() {
    int offset;
	int mida, pos;
	int mida_real;
	char paraula[16];
	printf("Paraula: ");
	scanf("%s",paraula);
	printf("num caracters: ");
	scanf("%d",&offset);
	char anterior[offset+1];
	char posterior[offset+1];
	char buffer[64];


	FILE *f = fopen("soft.bin","rb");
	if (f == NULL) {
        //codi error
        return;
	}
	int num;
	char *prova = NULL;
	do {
        num = fread(buffer,sizeof(char),64,f);
        printf("%s\n",buffer);
        prova = strstr(buffer,paraula);
        if (prova != NULL) {
            printf("%d\n",ftell(f));
            pos = ftell(f) - 64 + (prova - buffer);
            printf("pos = %d\n",pos);
            break;
        }
        fseek(f,ftell(f)-16,SEEK_SET);//Moc el punter 16 caracters enrere perque no pugui quedar la paraula tallada

	} while (num == 64);

	if (prova == NULL) {
        //codi error
        printf("No s'ha trobat la paraula\n");
        return;
	}
	if ((pos - offset) < 0) {
        fseek(f,0,SEEK_SET);
        num = fread(anterior,sizeof(char),pos,f);
	} else {
        printf("estic al else\n");
        fseek(f,pos-offset,SEEK_SET);
        num = fread(anterior,sizeof(char),offset,f);
    }
    anterior[num] = '\0';

	printf("num = %d, anterior = %s\n",num,anterior);

	fseek(f,ftell(f)+strlen(paraula),SEEK_SET);
	num  = fread(posterior,sizeof(char),offset,f);
	posterior[num] = '\0';
	printf("Posterior = %s\n",posterior);

//    fseek(f,0,SEEK_SET);
//    printf("abans fread\n");
//	int num = fread(buffer,sizeof(char),64,f);
//    printf("num = %d\n",num);
//	prova = strstr(buffer,"CONTRASENYA");
//	printf("Buffer = %s\n",buffer);
//	printf("Prova = %s\n",prova);
//	printf("buffer - prova = %d\n",buffer-prova);
//	printf("prova - buffer = %d\n",prova - buffer);
//	fseek(f,prova - buffer,SEEK_SET);
//	num = fread(buffer,sizeof(char),64,f);
//	printf("Buffer = %s\n",buffer);








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













