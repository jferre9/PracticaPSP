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

#define MAX_CL	2

/*
Ordres possibles:

UP
START
STOP
READ
SEARCH
CLOSE

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
char ruta[64];//Aqui es guarda la ruta del programa
pthread_mutex_t mut_pid = PTHREAD_MUTEX_INITIALIZER; //mutex pel pid

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
    //printf("%s\n",exe.name);

    //Comprovem que no existeixi el fitxer
	if( access( exe.name, F_OK ) != -1 ) {
		enviar_codi(fdClient,ERROR);
		//printf("Existeix un fitxer amb el mateix nom\n");
		return;
	}
	enviar_codi(fdClient,OK);

	FILE *f = fopen(exe.name, "wb");
    if(f==NULL){
        perror("fopen");
    }


    int actSize = 0;
    do {
		//printf("Read\n");
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
		//printf("Error al transferir\n");
	} else {
		enviar_codi(fdClient,OK);
		printf("S'ha transferit el fitxer %s correctament\n",exe.name);
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





//Funcio que inicia el programa definit a la variable global ruta
void start() {
    pid = fork();
	if (pid < 0) {
		perror ("Fork");
		return;
	}
	else if (pid == 0) {
		//implamentar exec
		execl(ruta,ruta,NULL);
		perror("exec");
		_exit(-1);
	}
}

//funcio que para l'execucio del programa que te per pid la variable global pid
int stop() {
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
	return err;
}


/* Codis error possibles:
 * -1: Ja hi ha un programa executant-se
 * -2: No existeix el fitxer
 * -3: S'ha produit un error al executar
 */
void iniciar_executable(int fdClient) {
	int m;

	pthread_mutex_lock(&mut_pid);

	if (pid >= 0) {		// se suposa que el programa nomes es para si se'l hi diu desde aqui
		enviar_codi(fdClient, -1);
		//printf("ja hi ha un programa executant-se\n");
		pthread_mutex_unlock(&mut_pid);
		return;
	}

	//LLegim el nom
	if((m=read(fdClient,&ruta,64))<0){
        perror("read");
    }

    //Comprovem que existeixi el fitxer
	if( access( ruta, F_OK ) == -1 ) {
		enviar_codi(fdClient,-2);
		//printf("El fitxer no existeix\n");
		pthread_mutex_unlock(&mut_pid);
		return;
	}

    //Fem l'exec
    start();

    if (pid < 0) {
        enviar_codi(fdClient,-3);
        pid = -1;
		pthread_mutex_unlock(&mut_pid);
        return;
    }
	sleep(2); // Esperems 2 segons per veure si el proces segueix funcionant
	int res = waitpid(pid,NULL,WNOHANG);//Comprovem que el programa seguixi funcionant amb un waitpid que no bloqueja

	if (res == 0) {
		enviar_codi(fdClient, OK);
	} else {
	    enviar_codi(fdClient,-3);
		pid = -1;
	}
    pthread_mutex_unlock(&mut_pid);


}


/* Codis error possibles:
 * -1: El programa no havia estat iniciat
 * -2: Error al matar el programa
 */
void matar_programa (int fdClient) {
    pthread_mutex_lock(&mut_pid);

	int err = stop();
	enviar_codi(fdClient,err);
	pthread_mutex_unlock(&mut_pid);
}


/* Codis error possibles:
 * -1: No existeix el fitxer
 * -2: La posicio esta fora el fitxer
 */
void llegir_posicio(int fdClient) {
    t_fitxer fitxer;
    int num, m;
    char parat = 0;

    if((m=read(fdClient,&fitxer,sizeof(t_fitxer)))<0){
        perror("read");
    }
    FILE *f = fopen(fitxer.file,"rb");
	if (f == NULL) {
        enviar_codi(fdClient,ERROR);
        return;
	}

	pthread_mutex_lock(&mut_pid);
    if (pid > 0) {
        stop();
        parat = 1;
	}
	char buffer[fitxer.offset+1];

	fseek(f,fitxer.pos,SEEK_SET);
	num = fread(buffer,sizeof(char),fitxer.offset,f);
	if (num <= 0) {
        enviar_codi(fdClient,-2);
	} else {
        buffer[num] = '\0';
        enviar_codi(fdClient,OK);
        if((m=write(fdClient, buffer, fitxer.offset+1))<0) {
            perror("Write client");
        }
	}
    fclose(f);
    if (parat) start();
    pthread_mutex_unlock(&mut_pid);
}


/* Codis error possibles:
 * -1: No existeix el fitxer
 * -2: No s'ha trobat la paraula
 */
void busca_paraula(int fdClient) {
    t_fitxer fitxer;
    int m, num;
    char parat = 0;
    if((m=read(fdClient,&fitxer,sizeof(t_fitxer)))<0){
        perror("read");
    }
    FILE *f = fopen(fitxer.file,"rb");
	if (f == NULL) {
        enviar_codi(fdClient,ERROR);
        return;
	}
    pthread_mutex_unlock(&mut_pid);
	if (pid > 0) {
        stop();
        parat = 1;
	}


	char anterior[fitxer.offset+1];
	char posterior[fitxer.offset+1];
	char buffer[64];
	char *prova = NULL;
	int pos;

	do {
        num = fread(buffer,sizeof(char),64,f);
        prova = strstr(buffer,fitxer.paraula);//Retorna un punter al inici de la paraula
        if (prova != NULL) {
            pos = ftell(f) - 64 + (prova - buffer);//resto els punters per trobar la posicio
            break;
        }
        fseek(f,ftell(f)-16,SEEK_SET);//Moc el punter 16 caracters enrere perque no pugui quedar la paraula tallada

	} while (num == 64);

	if (prova == NULL) {
        enviar_codi(fdClient,-2);
        fclose(f);

        if (parat) start();
		pthread_mutex_unlock(&mut_pid);
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
	enviar_codi(fdClient,OK);
	if((m=write(fdClient, anterior, fitxer.offset+1))<0) {
        perror("Write client");
    }
    if((m=write(fdClient, posterior, fitxer.offset+1))<0) {
        perror("Write client");
    }
    fclose(f);
    if (parat) start();
    pthread_mutex_unlock(&mut_pid);
}

void *atendre_client(void *x) {
	t_client *client = (t_client *)x;
	//printf("fd = %d\n",fdClient);
	char ordre[8];
	int m;

	printf("S'ha establert una nova connexio\n");
	enviar_codi(client->sock,OK); //Envio el codi conforme queden fils lliures per atendre el client

    while (1) {
	//llegim la ordre
	if((m=read(client->sock,ordre,sizeof(ordre)))<0){
        perror("read");
    }
    if (!m) {//Si llegeixo 0 bytes es perque s'ha perdut la connexio
        break;
    }

    //Cridem les diferents funcions depenent de la ordre

    if (!strcmp(ordre,"UP")) pujar_executable(client->sock);
    else if (!strcmp(ordre,"START")) iniciar_executable(client->sock);
    else if (!strcmp(ordre,"STOP")) matar_programa(client->sock);
    else if (!strcmp(ordre,"READ")) llegir_posicio(client->sock);
    else if (!strcmp(ordre,"SEARCH")) busca_paraula(client->sock);
    else if (!strcmp(ordre,"CLOSE")) break;

    }
	pthread_mutex_lock(&mut);
	client->ocupat = 0;
	pthread_mutex_unlock(&mut);
	printf("S'ha tancat la connexio amb el client\n");



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
		   enviar_codi(fdClient,ERROR);
		   printf("No queden connexions lliures\n");
		   close(fdClient);
		   pthread_mutex_unlock(&mut);
		}

	}
	return (0);

}













