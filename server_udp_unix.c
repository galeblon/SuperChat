/*
 * server_udp.c
 *
 * 	Created on: Apr 24, 2019
 * 		Author: gales
 */


/**
 * Main thread: listening for keyboard inputs
 * Thread 1: listening for UDP packets
 * Thread 2: updating TTL
 */

/*
 * 	For testing purposes use:
 * 	ncat -vv localhost 61234 -u
 */
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
	struct sockaddr_in* connection;
	int ttl;
};

void dll_insert(struct sockaddr_in* conn, struct connection_node** first, struct connection_node** last){
	if(*first == *last && *first == NULL){
		*first = *last =  malloc(sizeof(struct connection_node));
		(*first)->connection = conn;
		(*first)->ttl = 15; // Time to live
		(*first)->next = (*first)->prev = NULL;
		return;
	}
	struct connection_node* iterator = *last;
	//while(iterator->next != NULL)
	//	iterator = iterator->next;
	iterator->next = malloc(sizeof(struct connection_node));
	iterator->next->connection = conn;
	iterator->next->ttl = 15;
	iterator->next->next = NULL;
	iterator->next->prev = iterator;
	*last = iterator->next;
	return;
}

void dll_remove(struct sockaddr_in* conn, struct connection_node** first, struct connection_node** last){
	struct connection_node* iterator = *first;
	if(*first == NULL)
		return;
	while(memcmp(iterator->connection, conn, sizeof(struct sockaddr_in)) != 0 && iterator != NULL){
		iterator = iterator->next;
	}
	if(iterator == NULL)
		return;
	if(iterator->prev != NULL)
		iterator->prev->next = iterator->next;
	if(iterator->next != NULL)
		iterator->next->prev = iterator->prev;
	free(iterator->connection);
	free(iterator);
}

void dll_clean(struct connection_node** first){
	if(*first == NULL)
		return;
	struct connection_node* iterator = *first;
	while(iterator->next != NULL){
		iterator = iterator->next;
		free(iterator->connection);
		free(iterator->prev);
	}
	free(iterator->connection);
	free(iterator);
}


pthread_mutex_t m;
pthread_mutexattr_t m_att;
int program_running = 1;

int sock_id;
int curr_connections = 0;
struct connection_node* first = NULL;
struct connection_node* last = NULL;


void* packet_listening(void* params);
void* ttl_update(void* params);

int main(){
	//Initialize semaphore
	if(pthread_mutex_init(&m, &m_att) == -1){
		fprintf(stderr, "Failed to create mutex\n");
		return 1;
	}

	//Initialize networking
	struct sockaddr_in serv_addr;

	sock_id = socket(AF_INET, SOCK_DGRAM, 17);
	if(sock_id == -1){
		fprintf(stderr, "Could not create a socket!\n");
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

	//Server setup done
	pthread_mutex_lock(&m);
	//Create thread to listen for packets
	pthread_t packet_listener_t;
	pthread_create(&packet_listener_t, NULL, packet_listening, NULL);

	//Create thread to update ttl
	pthread_t ttl_updater_t;
	pthread_create(&ttl_updater_t, NULL, ttl_update, NULL);

	pthread_mutex_unlock(&m);

	//Gather keyboard input, await 'q' to shutdown the server
	char key_in = ' ';
	while(key_in != 'q')
		key_in = getchar();

	//Send signal to end all threads;
	program_running = 0;

	//Wait for threads to finish;
	pthread_join(packet_listener_t, NULL);
	pthread_join(ttl_updater_t, NULL);

	printf("All threads stopped\n");

	//Destroy semaphore
	pthread_mutex_destroy(&m);
	//Clear connection dll
	dll_clean(&first);

	printf("Server shutdown\n");
	return 0;
}

/*
 * System messages:
 * SYSTEM_JOIN -> client want to subscribe to server
 * SYSTEM_DISCONNECT -> client manually
 */
void* packet_listening(void* params){
	char buf[80];
	int buf_len = 80;
	printf("Listening for packets\n");
	//int socket_i;
	struct sockaddr_in sc;
	socklen_t lenc;
	struct timeval timeout = {3, 0};

	fd_set can_read;
	FD_ZERO(&can_read);
	int nready;
	while(program_running){
		FD_SET(sock_id, &can_read);
		timeout.tv_sec = 3;
		if((nready = select(sock_id+1, &can_read, NULL, NULL, &timeout)) > 0){
			if (recvfrom(sock_id, buf, buf_len, 0, (struct sockaddr *) &sc, &lenc) > 0) {
				char conn_addr[20];
				inet_ntop(AF_INET, &(sc.sin_addr), conn_addr, 20);
				//printf("Received a packet from %s\n: %s\n", conn_addr, buf);
				if(strncmp(buf, "SYSTEM", 6)== 0){
					printf("Received system packet from %s:\n\t%s", conn_addr, buf);
				} else{
					//CHeck if player is subscribed;
					printf("%s", buf);
				}
			} else {
				printf("Could not receive packet!\n");
			}
			memset(buf, '\0', buf_len);
		}
	}

	return NULL;
}

void* ttl_update(void* params){
	while(program_running);
	return NULL;
}
