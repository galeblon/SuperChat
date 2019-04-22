/*
 * server_tcp.c
 *
 *  Created on: Apr 22, 2019
 *      Author: gales
 */

/*
 * Main thread: listening for keyboard inputs
 * Thread 1: listening for connections
 * Thread 2+: client connection thread
 *
 **/

#include<stdio.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<time.h>


#define ST_PORT 61234
#define MAX_CONNECTIONS 3


struct connection_node{
	struct connection_node* prev, *next;
	int connection;
	pthread_t conn_t;
};

void dll_insert(int conn, pthread_t conn_t, struct connection_node** first, struct connection_node** last){
	if(*first == *last && *first == NULL){
		*first = *last =  malloc(sizeof(struct connection_node));
		(*first)->connection = conn;
		(*first)->next = (*first)->prev = NULL;
		return;
	}
	struct connection_node* iterator = *last;
	//while(iterator->next != NULL)
	//	iterator = iterator->next;
	iterator->next = malloc(sizeof(struct connection_node));
	iterator->next->connection = conn;
	iterator->next->conn_t = conn_t;
	iterator->next->next = NULL;
	iterator->next->prev = iterator;
	*last = iterator->next;
	return;
}

void dll_remove(int conn, struct connection_node** first, struct connection_node** last){
	struct connection_node* iterator = *first;
	if(*first == NULL)
		return;
	while(iterator->connection != conn && iterator != NULL){
		iterator = iterator->next;
	}
	if(iterator == NULL)
		return;
	if(iterator->prev != NULL)
		iterator->prev->next = iterator->next;
	if(iterator->next != NULL)
		iterator->next->prev = iterator->prev;
	free(iterator);
}

void dll_clean(struct connection_node** first){
	if(*first == NULL)
		return;
	struct connection_node* iterator = *first;
	while(iterator->next != NULL){
		iterator = iterator->next;
		free(iterator->prev);
	}
	free(iterator);
}



pthread_mutex_t m;
pthread_mutexattr_t m_att;
int program_running = 1;

int sock_id;
int curr_connections = 0;
struct connection_node* first = NULL;
struct connection_node* last = NULL;


void* connection_listening(void* params);
void* client_loop(void* params);

int main(){
	//Initialize semaphore
	if(pthread_mutex_init(&m, &m_att) == -1){
		fprintf(stderr, "Failed to create mutex\n");
		return 1;
	}

	//Initialize networking
	struct sockaddr_in serv_addr;

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_id == -1){
		fprintf(stderr ,"Could not create a socket!\n");
		return 2;

	}
	memset(&serv_addr, '0', sizeof(struct sockaddr_in));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ST_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock_id, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
		fprintf(stderr, "Could not bind server address!\n");
		return 3;
	}

	if(listen(sock_id, MAX_CONNECTIONS) == -1){
		fprintf(stderr, "Could not begin listening!\n");
		return 4;
	}

	//Server setup done
	//Create thread to listen for connections
	pthread_t connection_listener_t;
	pthread_create(&connection_listener_t, NULL, connection_listening, NULL);

	//Gather keyboard input, await 'q' to shutdown the server
	char key_in = ' ';
	while(key_in != 'q')
		key_in = getchar();

	//Send signal to end all threads;
	program_running = 0;

	//Wait for threads to finish
	pthread_join(connection_listener_t, NULL);

	struct connection_node* iterator = first;
	while(iterator != NULL){
		pthread_join(iterator->conn_t, NULL);
		iterator = iterator->next;
        printf("Threads left: %d\n", curr_connections--);
	}

	printf("All threads stopped\n");

	//Destroy semaphore
	pthread_mutex_destroy(&m);
	//Clear connection dll
	dll_clean(&first);

	printf("Server shutdown\n");
	return 0;
}


void* connection_listening(void *params){
	printf("Listening for connections\n");
	int socket_i;
	struct sockaddr_in sc;
	socklen_t lenc;
	lenc = sizeof(sc);

	fd_set can_read;
	FD_ZERO(&can_read);
	int nready;
	struct timeval timeout = {3, 0};
	while(program_running){
		FD_SET(sock_id, &can_read);
		timeout.tv_sec = 3;
		if((nready = select(sock_id+1, &can_read, NULL, NULL, &timeout)) > 0){ //Select not working correctly should be == 1
			pthread_mutex_lock(&m);
			printf("There is a connection: %d\n", nready);
			if(curr_connections < MAX_CONNECTIONS){
				//Establish connection
				socket_i = accept(sock_id, (struct sockaddr*)&sc, &lenc);
				if(socket_i != -1){
					char buf[20];
					int buf_len = 20;
					inet_ntop(AF_INET, &(sc.sin_addr), buf, buf_len);
					printf("Connected user from: %s\n", buf);

					//Create new thread for client
					pthread_t connection_client_t;
					pthread_create(&connection_client_t, NULL, client_loop, (void*) &socket_i);
					dll_insert(socket_i, connection_client_t, &first, &last);
					curr_connections++;
				} else {
					printf("Could not establish connection\n");
				}
			} else{
				socket_i = accept(sock_id, (struct sockaddr*)&sc, &lenc);
				shutdown(socket_i, 2);
				printf("Connection refused: server full\n");
			}
			pthread_mutex_unlock(&m);
		}
	}


	while(program_running)
		printf("running\n");
	printf("Stopping listening\n");
	return NULL;
}


void* client_loop(void* params){
	int my_sock = *((int*)params);
	printf("Listening for messages from client %d\n", my_sock);
	char buf[80];
	int buf_len = 80;

	fd_set can_read;
	FD_ZERO(&can_read);
	int nready;
	struct timeval timeout = {3, 0};
	while(program_running){
		//Do your magic
		FD_SET(my_sock, &can_read);
		timeout.tv_sec = 3;
		if((nready = select(my_sock+1, &can_read, NULL, NULL, &timeout)) > 0){
			if(recv(my_sock, buf, buf_len, 0) > 0){
				if(strcmp(buf+5, "KONIEC") == 0){
					pthread_mutex_lock(&m);
					printf("Manual connection end for %d\n", my_sock);
					shutdown(my_sock, 2);
					dll_remove(my_sock, &first, &last);
					curr_connections--;
					pthread_mutex_unlock(&m);
					return NULL;
				}
				//Normal message
				pthread_mutex_lock(&m);
				printf("%s\n", buf);
				struct connection_node* iterator = first;
				while(iterator != NULL){
					if(iterator->connection != my_sock)
						send(iterator->connection, buf, buf_len, 0);
					iterator = iterator->next;
				}
				pthread_mutex_unlock(&m);
			} else {
				pthread_mutex_lock(&m);
				printf("Abrupt connection end for %d\n", my_sock);
				shutdown(my_sock, 2);
				dll_remove(my_sock, &first, &last);
				curr_connections--;
				pthread_mutex_unlock(&m);
				return NULL;
			}
		}
	}
	printf("Stopping client %d\n", my_sock);
	int res = shutdown(my_sock, 2);
	printf("\t+--> Stopping result: %d\n", res);
	return NULL;
}
