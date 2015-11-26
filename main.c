#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#define THREAD_COUNT 5
#define CONNMAX 1000

pthread_t ntid[THREAD_COUNT];
pthread_mutex_t lock[THREAD_COUNT];
int clients[CONNMAX];

//оправляет ответ клиену
void headers(int client, int size, int httpcode, char *contentType) {
	char buf[1024];
	char strsize[20];
	printf(strsize, "%d", size);
	if (httpcode == 200) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	}
	else if (httpcode == 404) {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
	}
	else {
		strcpy(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	}
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Connection: keep-alive\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Content-length: ");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, strsize);
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "simple-server\r\n");
	send(client, buf, strlen(buf), 0);
	printf(buf, contentType);
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}

//отправляет поток на выполнение
void createThread(int cd) {
	int *k = (int*) malloc(sizeof(int));
	*k = cd; 
	//i++; 
	int err = pthread_create(&ntid[cd], NULL, &handleClient, k); 
	if (err != 0) {
		printf("it's impossible to create a thread %s\n", strerror(err));
	}
}

void parseFileName(char *line, char **filepath, size_t *len) {
	char *start = NULL;
	while ((*line) != '/') line++;
	start = line + 1;
	while ((*line) != ' ') line++;
	(*len) = line - start;
	*filepath = (char*)malloc(*len + 1);
	*filepath = strncpy(*filepath, start, *len);
	(*filepath)[*len] = '\0';
	printf("%s \n", *filepath);
}

int setContentType(char *filepath, char **contentType) {
    if(strstr(filepath, ".jpg")!=NULL) {
        *contentType = "Content-Type: image/jpeg\r\n";
        return 1;
    }
    else if(strstr(filepath, ".txt")!=NULL) {
        *contentType = "Content-Type: text/plain\r\n";
        return 2;
    }
    else if(strstr(filepath, ".html")!=NULL) {
        *contentType = "Content-Type: text/html\r\n";
        return 3;
    }
    else return NULL;
}
//обрабатывает запрос клиента и отправляет ответ
void *handleClient(void *arg) {   
	
	int filesize = 0;
    char *line = NULL;
    size_t len = 0;
    char *filepath = NULL;
	size_t filepath_len = 0;
	int res = 0;
    FILE *fd;
	FILE *file;
	char *contentType = (char*) malloc(255*sizeof(char));
	int cd = *((int *)arg);
	
	
	pthread_mutex_lock(&lock[cd%5]);
	pthread_mutex_unlock(&lock[cd%5]);
	
	int empty_str_count = 0;
    printf("In thread #%d cd =%d\n", cd,clients[cd]);
    fd = fdopen(clients[cd], "r");
    if (fd == NULL) {
        printf("error open client descriptor as file \n");
    }
	printf("In thread #%d: after fdopen\n", clients[cd]);
    while ((res = getline(&line, &len, fd)) != -1) {
        if (strstr(line, "GET")) {
        	printf("1");
            parseFileName(line, &filepath, &filepath_len);
        }
        if (strcmp(line, "\r\n") == 0) {
        	printf("2");
            empty_str_count++;
        }
        else {
        	printf("3");
            empty_str_count = 0;
        }
        if (empty_str_count == 1) {
        	printf("4");
            break;
        }
        printf("%s", line);
    }
    printf("In thread #%d: open %s \n", clients[cd],filepath);

    file = fopen(filepath, "rb");
	printf("In thread #%d: after fopen\n", clients[cd]);
    if (file == NULL) {
        printf("404 File Not Found \n");
        headers(clients[cd], 0, 404, contentType);
    } 
    else if (file!=NULL && setContentType(filepath, &contentType)==NULL)
    {
        printf("500 Internal Server Error \n");
        headers(clients[cd], 0, 500, contentType);
    }
    else if (file!=NULL && setContentType(filepath, &contentType)!=NULL){
        fseek(file, 0L, SEEK_END);
        filesize = ftell(file);
        fseek(file, 0L, SEEK_SET);
        printf("%s", contentType);
        headers(clients[cd], filesize, 200, contentType);
        unsigned char buf[1024];
		int bytes = 0;
		while((bytes=fread(buf,1,1024,file))>0) {
		    //int n = fread(buf,filesize,1,file);
		    //if(n==0)
		    //    printf("Read file Error");
		    res = send(clients[cd], buf, 1024, 0);
		    if (res == -1) {
		        printf("send error \n");
		    }
		}		
    }
	//pthread_mutex_unlock(&lock[cd%5]);
	free(arg);
    close(clients[cd]);
	clients[cd]=-1;
    return (void*)0;
}
int main() {
	int ld = 0;
	int res = 0;
	//int cd = 0; 
	//int lastcd = 0;
	const int backlog = 10;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;


	socklen_t size_saddr;
	socklen_t size_caddr;
	
	int tmp =0;
	while(tmp<5){
		pthread_mutex_init(&lock[tmp],NULL);
		pthread_mutex_lock(&lock[tmp]);
		createThread(tmp);
		tmp++;
	}
	
	int slot=0;
	//установка всех элементов в -1
    int i;
    for (i=0; i<CONNMAX; i++)
        clients[i]=-1;

	ld = socket(AF_INET, SOCK_STREAM, 0);
	if (ld == -1) {
		printf("listener create error \n");
	}
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8080);
	saddr.sin_addr.s_addr = INADDR_ANY;
	res = bind(ld, (struct sockaddr *)&saddr, sizeof(saddr));
	if (res == -1) {
		printf("bind error \n");
	}
	res = listen(ld, backlog);
	if (res == -1) {
		printf("listen error \n");
	}
	
	//int slot=-1;
	int j=0;
	while (1) {
		clients[slot] = accept(ld, (struct sockaddr *)&caddr, &size_caddr);
		if(j>4){//если все потоки отработали, то их заново надо создать
			j=0;
			int k=0;
			int coeff=slot/5;
			while(k<5){
				if(pthread_mutex_trylock(&lock[k])==0)
					createThread(k+5*coeff);
				k++;
			}
		}
		if (clients[slot] < -1) {
			printf("accept error \n");
		}
		else{
			//printf("client in %d descriptor. Client addr is %d \n", cd, caddr.sin_addr.s_addr);
			pthread_mutex_unlock(&lock[j]);
		}
		j++;
		while(clients[slot]!=-1) slot =(slot+1)%CONNMAX;
	}
	return 0;
}
