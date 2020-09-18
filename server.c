#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <netdb.h> 
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <stdbool.h>

#define BUFFER_SIZE 4096 
#define PORT 8000 
#define SOCKET_ERROR (-1)
#define SERVER_BACKLOG 100


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void handler_connection(int client_socket);
int check(int exp, const char *msg);

int main(int argc, char **argv) 
{ 
	int server_scoket, client_socket, addr_size;
     	SA_IN server_addr, client_addr;

		
	// socket create and verification 
	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), 
		"socket creation failed...\n"); 
	
	// initialze the address struct
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(PORT); 


	// Binding newly created socket to given IP and verification 
	check((bind(server_socket, (SA*)&server_addr, sizeof(server_addr)), 
		"socket bind failed...\n"); 

	// Now server is ready to listen and verification 
	check(listen(server_socket, SERVER_BACKLOG), 
		"Listen failed...\n"); 
	
	while(true) {
		printf("Waiting for connection...\n");
		addr_size = sizeof(SA_IN);
		check(client_socket = 
			accept(server_socket, (SA*)&clieny_addr, (socketlen_t*)&addr_size), "accept failed");
		printf("connected!!!\n");
		// do whatever we do with connections.
		handle_connection(client_socket);
	}

	return 0;
}

int check (int exp, const char *msg)
{
}

void handle_connection(int client_scoket)
{
}
