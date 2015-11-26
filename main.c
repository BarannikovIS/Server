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