#include <pthread.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define PORT 1600

#define MAX_CL	1

/*
Ordres possibles:

UP
START
STOP
READ
SEARCH
DELETE

*/



//Codis retorn servidor
#define OK	1
#define ERROR -1

typedef struct t_executable{
	int size;
	char name[32];
}t_executable;


typedef struct {
	int sock;
	pthread_t th;
	int ocupat;
} t_client;

typedef struct {
	t_client client[MAX_CL];
	int num;
} t_clients;

typedef struct {
    char file[32];
    char paraula[16];
    int pos;
    int offset;
} t_fitxer;



int pid = -1;//Aqui es guarda el pid del process que executa el programa

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void enviar_codi(int fdClient, int err) {
	if(write(fdClient, &err, sizeof(int)) <0){
		perror("Write client");
	}
}



void pujar_executable(int fdClient) {
	int m, n, err;
	t_executable exe;
    char buf[1024];
	//LLegim el nom i la mida
	if((m=read(fdClient,&exe,sizeof(t_executable)))<0){
        perror("read");
    }
    printf("%s\n",exe.name);

    //Comprovem que no existeixi el fitxer
	if( access( exe.name, F_OK ) != -1 ) {
		enviar_codi(fdClient,ERROR);
		printf("Existeix un fitxer amb el mateix nom\n");
		return;
	}
	enviar_codi(fdClient,OK);

	FILE *f = fopen(exe.name, "wb");
    if(f==NULL){
        perror("fopen");
    }


    int actSize = 0;
    do {
		printf("Read\n");
        m=read(fdClient,buf,1024);
        if(m>0){
            n = fwrite(buf, sizeof(char), m,f);
            actSize += n;
        }
    } while ((m>0) && (n>0) && (actSize < exe.size));


    if(m<0 || n < 0 || actSize < exe.size){
		err = ERROR;
		if((m=write(fdClient, &err, sizeof(int)))<0){
			perror("Write client");
		}
		printf("Error al transferir\n");
	} else {
		enviar_codi(fdClient,OK);
		printf("S'ha transferit correctament\n");
		if (chmod(exe.name,S_IRWXU | S_IRWXG | S_IRWXO) < 0) perror("chmod");
	}

	fclose(f);

}

void init_taula(t_clients *clients) {
	int i;

	for (i = 0; i < MAX_CL; i++) {
		clients->client[i].ocupat=0;
	}
	clients->num = 0;
}


int busca_posicio(t_clients *clients) {
	int i;

	for (i = 0; i < MAX_CL; i++) {
		if (clients->client[i].ocupat==0) return i;
	}
	return -1;
}

/* Codis error possibles:
 * -1: Ja hi ha un programa executant-se
 * -2: No existeix el fitxer
 * -3: S'ha produit un error al executar
 */
void iniciar_executable(int fdClient) {
	char file[64];
	int m;

	if (pid >= 0) {		// se suposa que el programa nomes es para si se'l hi diu desde aqui
		/*err = ERROR;	// tambe es podria mirar si hi ha un process amb aquest pid
		if((m=write(fdClient, &err, sizeof(int)))<0){
			perror("Write client");
		}*/
		enviar_codi(fdClient, -1);
		printf("ja hi ha un programa executant-se\n");
		return;
	}

	//LLegim el nom
	if((m=read(fdClient,&file,64))<0){
        perror("read");
    }
    printf("Fitxer = %s\n",file);

    //Comprovem que existeixi el fitxer
	if( access( file, F_OK ) == -1 ) {
		/*err = -2;
		if((m=write(fdClient, &err, sizeof(int)))<0){
			perror("Write client");
		}*/
		enviar_codi(fdClient,-2);
		printf("El fitxer no existeix\n");
		return;
	}


	pid = fork();
	if (pid < 0) {
		perror ("Fork"); _exit (-1);
	}
	else if (pid == 0) {
		//implamentar exec
		execl(file,file,NULL);
		perror("exec");
		_exit(-1);
	}
	//Codi del pare
	sleep(2); // Esperems 2 segons per veure si el proces segueix funcionant
	printf("waitpid\n");
	int res = waitpid(pid,NULL,WNOHANG);
	printf("Resultat waitpid = %d\n",res);

	if (res == pid) {//El proces ja ha mort
		enviar_codi(fdClient,-3);
		pid = -1;
	} else if (res == 0) {
		enviar_codi(fdClient, OK);
	}



}


/* Codis error possibles:
 * -1: El programa no havia estat iniciat
 * -2: Error al matar el programa
 */
void matar_programa (int fdClient) {
	int err;
	if (pid <= 0) err = ERROR;
	else if (kill(pid,SIGKILL) >= 0) {
		if (waitpid(pid,NULL,0) < 0) perror("waitpid");
		err = OK;
		pid = -1;
	} else {
		pid = -1;
		err = -2;
	}
	enviar_codi(fdClient,err);
}


/* Codis error possibles:
 * -1: No existeix el fitxer
 * -2: La posicio esta fora el fitxer
 */
void llegir_posicio(int fdClient) {
    t_fitxer fitxer;
    int num, m;

    if((m=read(fdClient,&fitxer,sizeof(t_fitxer)))<0){
        perror("read");
    }
    FILE *f = fopen(fitxer.file,"rb");
	if (f == NULL) {
        enviar_codi(fdClient,ERROR);
        return;
	}
	char buffer[fitxer.offset+1];

	fseek(f,fitxer.pos,SEEK_SET);
	num = fread(buffer,sizeof(char),fitxer.offset,f);
	if (num <= 0) {
        enviar_codi(fdClient,-2);
        fclose(f);
        return;
	}
	buffer[num] = '\0';
	printf("He llegit %d: %s\n",num,buffer);
	enviar_codi(fdClient,OK);
	if((m=write(fdClient, buffer, fitxer.offset+1))<0) {
        perror("Write client");
    }
    fclose(f);

}


/* Codis error possibles:
 * -1: No existeix el fitxer
 * -2: No s'ha trobat la paraula
 */
void busca_paraula(int fdClient) {
    t_fitxer fitxer;
    int m, num;
    if((m=read(fdClient,&fitxer,sizeof(t_fitxer)))<0){
        perror("read");
    }
    FILE *f = fopen(fitxer.file,"rb");
	if (f == NULL) {
        enviar_codi(fdClient,ERROR);
        return;
	}

	char anterior[fitxer.offset+1];
	char posterior[fitxer.offset+1];
	char buffer[64];
	char *prova = NULL;
	int pos;

	do {
        num = fread(buffer,sizeof(char),64,f);
        printf("%s\n",buffer);
        prova = strstr(buffer,fitxer.paraula);
        if (prova != NULL) {
            pos = ftell(f) - 64 + (prova - buffer);
            break;
        }
        fseek(f,ftell(f)-16,SEEK_SET);//Moc el punter 16 caracters enrere perque no pugui quedar la paraula tallada

	} while (num == 64);

	if (prova == NULL) {
        enviar_codi(fdClient,-2);
        printf("No s'ha trobat la paraula\n");
        fclose(f);
        return;
	}
	if ((pos - fitxer.offset) < 0) {
        fseek(f,0,SEEK_SET);
        num = fread(anterior,sizeof(char),pos,f);
	} else {
        fseek(f,pos-fitxer.offset,SEEK_SET);
        num = fread(anterior,sizeof(char),fitxer.offset,f);
    }
    anterior[num] = '\0';

	fseek(f,ftell(f)+strlen(fitxer.paraula),SEEK_SET);
	num  = fread(posterior,sizeof(char),fitxer.offset,f);
	posterior[num] = '\0';
	printf("Anterior = %s\nPosterior = %s\n",anterior,posterior);
	enviar_codi(fdClient,OK);
	if((m=write(fdClient, anterior, fitxer.offset+1))<0) {
        perror("Write client");
    }
    if((m=write(fdClient, posterior, fitxer.offset+1))<0) {
        perror("Write client");
    }
    fclose(f);
}

void *atendre_client(void *x) {
	t_client *client = (t_client *)x;
	//printf("fd = %d\n",fdClient);
	char ordre[8];
	int m;

    while (1) {
	//llegim la ordre
	if((m=read(client->sock,ordre,sizeof(ordre)))<0){
        perror("read");
    }
    if (!m) {
        printf("El client ha tancat el programa\n");
        break;
    }
    printf("m = %d\n\n\n",m);

    //Cridem les diferents funcions depenent de la ordre

    if (!strcmp(ordre,"UP")) pujar_executable(client->sock);
    else if (!strcmp(ordre,"START")) iniciar_executable(client->sock);
    else if (!strcmp(ordre,"STOP")) matar_programa(client->sock);
    else if (!strcmp(ordre,"READ")) llegir_posicio(client->sock);
    else if (!strcmp(ordre,"SEARCH")) busca_paraula(client->sock);
    //else if (!strcmp(ordre,"DELETE")) matar_programa(client->sock);

    }
	pthread_mutex_lock(&mut);
	client->ocupat = 0;
	pthread_mutex_unlock(&mut);



}

int main(int argc, char**argv){

	t_clients clients;

	init_taula(&clients);


    int fd,i,m,longitud, fdClient, err, pos;
    struct sockaddr_in server, client;


    //

    //

    if((fd = socket (AF_INET, SOCK_STREAM,0))<0){
        perror ("socket");return -13;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons (PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    for(i=0;i<8;i++){
        server.sin_zero[i] = 0;
    }

    if((m = bind (fd, (struct sockaddr *)&server, sizeof(server)))<0){
        perror ("bind");return -24;
    }

    if((m = listen(fd,10))<0){
        perror("listen");
    }
    longitud = sizeof(client);
    while(1){
        fdClient = accept (fd,(struct sockaddr *)&client, &longitud);
        while((fdClient < 0) && (errno == EINTR)){
            fdClient = accept (fd,(struct sockaddr *)&client, &longitud);
        }
        if(fdClient < 0){
            perror("accept");return -1;
        }

        pthread_mutex_lock(&mut);
        pos = busca_posicio(&clients);
        if (pos >= 0) {
			clients.client[pos].ocupat = 1;
			(clients.num)++;

			pthread_mutex_unlock(&mut);

			clients.client[pos].sock = fdClient;
			err =  pthread_create(&(clients.client[pos].th),NULL,(void *)&atendre_client,(void *)&(clients.client[pos]));
			if (err < 0) {
				perror("Pthread create");
			}
			pthread_detach((clients.client[pos].th));

		} else {
		   //enviar missatge d'error i tancar connexio
		   printf("No queden connexions lliures\n");
		   close(fdClient);
		   pthread_mutex_unlock(&mut);
		}

	}
	return (0);

}













