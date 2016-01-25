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
CLOSE

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

typedef struct {
    char file[32];
    char paraula[16];
    int pos;
    int offset;
} t_fitxer;

//client envia mida i nom servidor torna error si existeix i ok si no existeix
void pujar_executable(int fd){
	t_executable exe;
	int m, n, resposta;
    char buf[1024];
    FILE *f = NULL;
    do {
        printf("Fitxer a pujar: ");
        scanf("%s", exe.name);
        f = fopen(exe.name,"rb");
        if (f == NULL) printf("El fitxer no existeix\n");
    } while (f == NULL);



	//Enviem la operacio al servidor:
	char ordre[8] = "UP";
	if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }


    if(f==NULL){
        printf("El fitxer no existeix");
        return;
    }


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
void iniciar_executable(int fd){
	int m;
	char file[64];

    printf("Fitxer a executar: ");
    scanf("%s", file);
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



void llegir_posicio(int fd) {
    t_fitxer fitxer;
    int m;

    printf("Introdueix el nom del fitxer: ");
    scanf("%s",fitxer.file);

    do {
		printf("Introdueix la posicio: ");
		scanf("%d",&fitxer.pos);
		if (fitxer.pos < 0) printf("La posició no pot ser negativa\n");
	} while (fitxer.pos < 0);
	do {
		printf("Introdueix el nombre de caracters a llegir: ");
		scanf("%d",&fitxer.offset);
		if (fitxer.offset < 1) printf("Ha de ser superior a 1\n");
	} while (fitxer.offset < 1);

    char ordre[8]= "READ";
    if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }
    if((m=write(fd, &fitxer, sizeof(t_fitxer)))<0){
        perror("Write client");
        return;
    }

    int resposta = rebre_codi(fd);
    if (resposta == OK) {
        char buffer[fitxer.offset+1];
        if((m=read(fd,buffer,fitxer.offset+1))<0){
            perror("read");
        }
        printf("Caracters llegits = %s\n",buffer);

    } else {
        if (resposta == -1) printf("No existeix el fitxer\n");
        else if (resposta == -2) printf("La posicio es troba fora del fitxer\n");
        else printf("Error desconegut\n");
    }


}

void busca_paraula(int fd) {
    t_fitxer fitxer;
    int m;


    printf("Introdueix el nom del fitxer: ");
    scanf("%s",fitxer.file);
    printf("Introdueix la paraula: ");
    scanf("%s",fitxer.paraula);
	do {
		printf("Introdueix el nombre de caracters a llegir: ");
		scanf("%d",&fitxer.offset);
		if (fitxer.offset < 1) printf("Ha de ser superior a 1\n");
	} while (fitxer.offset < 1);

    char ordre[8]= "SEARCH";
    if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
        return;
    }
    if((m=write(fd, &fitxer, sizeof(t_fitxer)))<0){
        perror("Write client");
        return;
    }

    int resposta = rebre_codi(fd);
    if (resposta == OK) {
        char anterior[fitxer.offset+1];
        char posterior[fitxer.offset+1];
        if((m=read(fd,anterior,fitxer.offset+1))<0){
            perror("read");
        }
        if((m=read(fd,posterior,fitxer.offset+1))<0){
            perror("read");
        }
        printf("Caracters anteriors = %s\n",anterior);
        printf("Caracters posteriors = %s\n",posterior);
    } else {
        if (resposta == -1) printf("No existeix el fitxer\n");
        else if (resposta == -2) printf("No s'ha trobat la paraula\n");
        else printf("Error desconegut\n");
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
    if (rebre_codi(fd) != OK) {
		printf("No queden conexions lliures al servidor\n");
		close(fd);
		return (-1);
	}
    int opcio = 0;
    while (opcio != 6) {
        printf("Que vols fer? \n1)Passar executable\n2)Iniciar Executable\n");
        printf("3)Parar execució\n4)Llegir caracters\n5)Buscar paraula\n");
        printf("6)Tancar programa\n");
        printf("Introdueix la opcio: ");
        scanf("%d",&opcio);
        switch(opcio) {
            printf("op = %d",opcio);
            case 1:
                pujar_executable(fd);
                break;
            case 2:
                iniciar_executable(fd);
                break;
            case 3:
                matar_programa(fd);
                break;
            case 4:
                llegir_posicio(fd);
                break;
            case 5:
                busca_paraula(fd);
                break;
            case 6:
                break;
            default:
                printf("Opcio incorrecte\n");
                break;
        }
    }
    char ordre[8] = "CLOSE";
    if((m=write(fd, ordre, sizeof(ordre)))<0){
        perror("Write client");
    }
    close(fd);
	return (0);
}































