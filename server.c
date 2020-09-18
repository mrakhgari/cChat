#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <netdb.h> 
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <stdbool.h>
#include <pthread.h>

#define BUFFER_SIZE 4096 
#define PORT 8000 
#define SOCKET_ERROR (-1)
#define SERVER_BACKLOG 100


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void * handle_connection(void* p_client_socket);
int check(int exp, const char *msg);

int main(int argc, char **argv) 
{ 
	int server_socket, client_socket, addr_size;
     	SA_IN server_addr, client_addr;

		
	// socket create and verification 
	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), 
		"socket creation failed...\n"); 
	
	// initialze the address struct
	// assign IP, PORT 
	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = INADDR_ANY; 
	server_addr.sin_port = htons(PORT); 


	// Binding newly created socket to given IP and verification 
	check(bind(server_socket, (SA*)&server_addr, sizeof(server_addr)), 
		"socket bind failed...\n"); 

	// Now server is ready to listen and verification 
	check(listen(server_socket, SERVER_BACKLOG), 
		"Listen failed...\n"); 
	
	while(true) {
		printf("Waiting for connection...\n");
		addr_size = sizeof(SA_IN);
		check(client_socket = 
			accept(server_socket, (SA*)&client_addr, (socklen_t*)&addr_size), "accept failed");
		printf("connected!!!\n");
		// do whatever we do with connections.
		pthread_t t;
		int *pclient = malloc(sizeof(int));
		*pclient = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient);
//		handle_connection(client_socket);
	}

	return 0;
}

int check (int exp, const char *msg)
{
	if ( exp == SOCKET_ERROR) {
		perror(msg);
		exit(1);
	}
	return exp;
}


void * handle_connection(void* p_client_socket)
{
	int client_socket = *((int*)p_client_socket);
	free(p_client_socket);
	char buffer[BUFFER_SIZE];
	size_t bytes_read;
	int msg_size = 0;
	
	// read the client message
	while((bytes_read = read(client_socket, buffer+msg_size, sizeof(buffer)-msg_size-1))>0){
		msg_size += bytes_read;
		if (msg_size> BUFFER_SIZE-1 || buffer[msg_size-1] == '\n') break;
	}
	check(bytes_read, "recv error");
	buffer[msg_size -1]=0;
	printf("REQUEST: %s", buffer);
	fflush(stdout);
	close(client_socket);
	printf("cloasing connection\n");
	return NULL;
}
