#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


/*
Ordres possibles:

UP
START
STOP
READ
SEARCH
DELETE

*/



#define PORT		1600
#define IP_SERVER	"127.0.0.1"

//Codis retorn servidor

#define OK	1
#define ERROR -1


typedef struct t_executable{
	int size;
	char name[32];
}t_executable;

typedef struct t_resultat {
	int err;
} t_resultat;


int rebre_codi(int fd) {
	int resposta = 0;
	if(read(fd,&resposta,sizeof(int))<0){
        perror("read");
        return -100;
	}
	return resposta;
}

//client envia mida i nom servidor torna error si existeix i ok si no existeix
void pujar_executable(int fd, char* file){
	t_executable exe;
	int m, n, resposta;
    char buf[1024];
	
	//Enviem la operacio al servidor:
	char ordre[8] = "UP";
	if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }
    
	FILE *f = fopen(file,"rb");
    if(f==NULL){
        printf("El fitxer no existeix");
        return;
    }
    
    strcpy(exe.name, file);
    fseek(f, 0L, SEEK_END);
    exe.size = ftell(f);
    
    
    
	if((m=write(fd, &exe, sizeof(t_executable)))<0){
        perror("Write client");
        return;
    }
    
    //Resposta del servidor
    if((m=read(fd,&resposta,sizeof(int)))<0){
        perror("read");
    }
    if (resposta != OK) {
		printf("Ja existeix un fitxer amb el mateix nom al servidor");
		return;
	}
    resposta = ERROR;
    
    fseek(f, 0L, SEEK_SET);
    
    int x;
	int i;
	x=fread(buf,sizeof(char),1024,f);
	while(x>0){
		if ((i=write(fd,buf,x)) != x) {
			perror("write");return;
		}
		x=fread(buf,sizeof(char),1024,f);
	}
	if (x < 0) {
		perror("fread");
	}
	//Resposta del servidor
    if((m=read(fd,&resposta,sizeof(int)))<0){
        perror("read");
    }
    if (resposta != OK) {
		printf("S'ha produit un error al transferir el fitxer\n");
	} else {
		printf("El fitxer s'ha transferit correctament\n");
	}
	
	fclose(f);
	
	
}


/* Codis error possibles:
 * -1: Ja hi ha un programa executant-se
 * -2: No existeix el fitxer
 * -3: S'ha produit un error al executar
 */
void iniciar_executable(int fd, char* file){
	int m;
	//Enviem la operacio al servidor:
	char ordre[8] = "START";
	if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }
    if((m=write(fd, file, 64))<0){
        perror("Write client");
        return;
    }
    int resposta = rebre_codi(fd);
    switch (resposta) {
		case 1:
			printf("El programa s'ha executat correctament\n");
			break;
		case -1:
			printf("Ja hi ha un programa executant-se primer l'has d'aturar\n");
			break;
		case -2:
			printf("No existeix cap fitxer amb aquest nom\n");
			break;
		case -3:
			printf("S'ha produït un error al executar el programa\n");
			break;
		default:
			printf("S'ha produït un error no identificat");
			break;
	}
    
}

/* Codis error possibles:
 * -1: El programa no havia estat iniciat
 * -2: Error al matar el programa
 */
void matar_programa (int fd) {
	int m;
	//Enviem la operacio al servidor:
	char ordre[8] = "STOP";
	if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }
    int resposta = rebre_codi(fd);
    switch (resposta) {
		case 1:
			printf("El programa s'ha aturat correctament\n");
			break;
		case -1:
			printf("El programa no havia estat iniciat\n");
			break;
		case -2:
			printf("Error al matar el programa\n");
			break;
		default:
			printf("S'ha produït un error no identificat");
			break;
	}
    
}

int main(int argc, char **argv) {
	int fd,i,m,longitud,res;
    struct sockaddr_in server, client;
    char buff[64];

    if((fd = socket (AF_INET, SOCK_STREAM,0))<0){
        perror ("socket");return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons (PORT);
    server.sin_addr.s_addr = inet_addr (IP_SERVER);
    for(i=0;i<8;i++){
        server.sin_zero[i] = 0;
    }
    if((res = connect (fd, (struct sockaddr *)&server, sizeof(server)))<0){
        perror("connect");
        return -1;
    }
    int opcio;
    printf("Que vols fer? \n1)Passar executable\n2)Iniciar Executable\n");
    printf("3)Parar execució\n4)Llegir caracters\n5)Buscar paraula\n");
    printf("Introdueix la opcio: ");
    scanf("%d",&opcio);
    switch(opcio) {
		printf("op = %d",opcio);
		case 1:
            printf("Fitxer a pujar: ");
            scanf("%s", buff);
            pujar_executable(fd, buff);
            break;
		case 2:
			printf("Fitxer a executar: ");
            scanf("%s", buff);
            iniciar_executable(fd, buff);
            break;
		case 3:
			matar_programa(fd);
			break;
        default:
			printf("Opcio incorrecte\n");
			break;
	}
	
	
	return (0);
}
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
