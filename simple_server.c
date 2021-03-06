#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define MAX_GROUP 10
#define GROUP_NAME 10

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct
{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
    char groups[MAX_GROUP][GROUP_NAME];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
    { // trim \n
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i])
        {
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void print_groups(client_t *client)
{
    printf("\n");
    for (int i = 0; i < MAX_GROUP; i++) {
        if (strcmp(client->groups[i], "") !=0){
            printf("%s\n",client->groups[i]);
        }   
    }
}

void join(client_t *client, char *gp)
{
    for (int i = 0; i < MAX_GROUP; i++) {
        if (strlen(client->groups[i]) == 0){
            strcpy(client->groups[i], gp);
            return;
        }   
    }
    
}

void leave(client_t *client, char *gp)
{  
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            for (int g = 0; g<MAX_GROUP; g++)
            {
                if (strlen(client->groups[g]) != 0 && strcmp(client->groups[g], gp)==0 ){
                    strcpy(client->groups[g], "");
                    return;
                }
            }
        }    
    }
}

/* Send message to all clients except sender */
void send_message(char *s, char* gp)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            printf("clients %d", i);
            for (int g_index; g_index<MAX_GROUP; g_index++)
            {
                char* group_temp = clients[i]->groups[g_index];
                // printf("group is $s\n" , *group_temp);
                // printf("strcmp %d", strcmp(group_temp , gp));
                if (strcmp(group_temp , gp) == 0)
                    if (write(clients[i]->sockfd, s, strlen(s)) < 0)
                    {
                        perror("ERROR: write to descriptor failed");
                        break;
                    }
            }
        }

    }
    pthread_mutex_unlock(&clients_mutex);
}

int handle_message(client_t *cl, char *s)
{
    // str_trim_lf(s, strlen(s));      
    // s = s[strlen(s) - 1];
    // s/trtok(s, "\n");
    s[strcspn(s, "\n")] = 0;
    // client_t *cl = (client_t *)client;
    char *command = strtok(s, " ");
    printf("command is: %s", command);
    if (strcmp(command, "join") == 0)
    {
        char *gp = strtok(NULL, " ");
        printf("user want to join to %s group", gp);
        join(cl, gp);
        print_groups(cl);
    }
    else if (strcmp(command, "leave") == 0)
    {
        char *gp = strtok(NULL, " ");
        printf("user want to join to %s group", gp);
        leave(cl, gp);
        print_groups(cl);
    }
    else if (strcmp(command, "quit") == 0)
    {
        printf("user %s quit", cl->name);
        return 1;
    }
    else if (strcmp(command, "send") == 0)
    {
        char *gp = strtok(NULL, " ");
        char *message = strtok(NULL, " ");
        printf("user want to send message {%s} to %s group ", message, gp);
        send_message(message, gp);
    }
    else
    {
        printf("user entered an invalid command: %s", command);
    }
    return 0;
}

/* Handle all communication with the client */
void *handle_client(void *arg)
{
    char buff_out[BUFFER_SZ];
    char name[32];
    int leave_flag = 0;

    cli_count++;
    client_t *cli = (client_t *)arg;

    // Name
    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
    {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    }
    else
    {
        strcpy(cli->name, name);
        sprintf(buff_out, "%s has joined\n", cli->name);
        printf("%s", buff_out);
        // send_message(buff_out, cli->uid);
    }

    bzero(buff_out, BUFFER_SZ);

    while (1)
    {
        if (leave_flag)
        {
            break;
        }

        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        if (receive > 0)
        {
            if (strlen(buff_out) > 0)
            {
                leave_flag = handle_message(cli, buff_out);
                // send_message(buff_out, cli->uid);

                str_trim_lf(buff_out, strlen(buff_out));
                printf("%s -> %s\n", buff_out, cli->name);
            }
        }
        else
        {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);
    }

    /* Delete client from queue and yield thread */
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
    {
        perror("ERROR: setsockopt failed");
        return EXIT_FAILURE;
    }

    /* Bind */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    /* Listen */
    if (listen(listenfd, 10) < 0)
    {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=== WELCOME TO THE CHATROOM ===\n");

    while (1)
    {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

        /* Check if max clients is reached */
        if ((cli_count + 1) == MAX_CLIENTS)
        {
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", cli_addr.sin_port);
            close(connfd);
            continue;
        }

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        /* Add client to the queue and fork thread */
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);

        /* Reduce CPU usage */
        sleep(1);
    }

    return EXIT_SUCCESS;
}
