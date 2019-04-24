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
 * Thread 3+: client handling
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

struct client{
	char* msg;
	struct sockaddr_in* addr;
};

struct connection_node{
	struct connection_node* prev, *next;
	struct sockaddr_in* connection;
	int ttl;
};

void dll_insert(struct sockaddr_in* conn, struct connection_node** first, struct connection_node** last){
	if(*first == *last && *first == NULL){
		*first = *last =  malloc(sizeof(struct connection_node));
		(*first)->connection = conn;
		(*first)->ttl = 5; // Time to live
		(*first)->next = (*first)->prev = NULL;
		return;
	}
	struct connection_node* iterator = *last;
	//while(iterator->next != NULL)
	//	iterator = iterator->next;
	iterator->next = malloc(sizeof(struct connection_node));
	iterator->next->connection = conn;
	iterator->next->ttl = 5;
	iterator->next->next = NULL;
	iterator->next->prev = iterator;
	*last = iterator->next;
	return;
}

void dll_remove(struct sockaddr_in* conn, struct connection_node** first, struct connection_node** last){
	struct connection_node* iterator = *first;
	if(*first == NULL)
		return;
	while(iterator != NULL && memcmp(iterator->connection, conn, sizeof(struct sockaddr_in)) != 0){
		iterator = iterator->next;
	}
	if(iterator == NULL)
		return;
	if(iterator->prev != NULL)
		iterator->prev->next = iterator->next;
	if(iterator->next != NULL)
		iterator->next->prev = iterator->prev;
	if(iterator == *first)
		*first = iterator->next;
	if(iterator == *last)
		*last = iterator->prev;
	free(iterator->connection);
	free(iterator);
}

struct connection_node* dll_contains(struct sockaddr_in* conn, struct connection_node** first, struct connection_node** last){
	struct connection_node* iterator = *first;
	if(*first == NULL)
		return NULL;
	while(iterator != NULL && memcmp(iterator->connection, conn, sizeof(struct sockaddr_in)) != 0 ){
		iterator = iterator->next;
	}
	return iterator;
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
void* handle_client(void* params);

void parse_system_message(char* msg, struct sockaddr_in sc);

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

	//Initial packet;
	sendto(sock_id, "SYSTEM_INITIAL", 15, 0, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));

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

	//Send shutdown signal to all clients;
	struct connection_node* iterator = first;
	while(iterator != NULL){
		sendto(sock_id, "SYSTEM_SHUTDOWN", 15, 0, (struct sockaddr*)iterator->connection, sizeof(struct sockaddr_in));
		iterator = iterator->next;
	}

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
 * SYSTEM_DISCONNECT -> client manually unsubscribes
 * SYSTEM_PING -> client refreshes his subscribtion
 * SYSTEM_SHUTDOWN -> server informs clients it's shutting down
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
				//Create new thread to handle client;
				struct client* c_handle = malloc(sizeof(struct client));
				c_handle->addr = malloc(sizeof(struct sockaddr_in));
				c_handle->msg = malloc(80);
				memcpy(c_handle->addr, &sc, sizeof(struct sockaddr_in));
				memcpy(c_handle->msg, buf, buf_len);
				pthread_t c_handler_t;
				pthread_create(&c_handler_t, NULL, &handle_client, (void*)c_handle);
			} else {
				printf("Could not receive packet!\n");
			}
			memset(buf, '\0', buf_len);
		}
	}

	return NULL;
}

void parse_system_message(char* msg, struct sockaddr_in sc){
	if(strncmp(msg, "SYSTEM_JOIN", 11) == 0){
		if(curr_connections < MAX_CONNECTIONS){
			pthread_mutex_lock(&m);
			if(dll_contains(&sc, &first, &last) == NULL){
				struct sockaddr_in* conn = malloc(sizeof(struct sockaddr_in));
				memcpy(conn, &sc, sizeof(struct sockaddr_in));
				dll_insert(conn, &first, &last);
				curr_connections++;
				printf("Added new subscriber\n");
			} else{
				printf("Cannot add subscriber: already subscribed!\n");
			}
			pthread_mutex_unlock(&m);
		} else{
			printf("Connection refused: Server full\n");
		}
	} else if(strncmp(msg, "SYSTEM_DISCONNECT", 17) == 0){
		pthread_mutex_lock(&m);
		if(dll_contains(&sc, &first, &last) != NULL){
			dll_remove(&sc, &first, &last);
			curr_connections--;
			printf("Removed subscriber\n");
		} else{
			printf("Cannot remove subscriber: not subscribed!\n");
		}
		pthread_mutex_unlock(&m);
	} else if(strncmp(msg, "SYSTEM_PING", 11) == 0){
		struct connection_node* client;
		pthread_mutex_lock(&m);
		if((client = dll_contains(&sc, &first, &last)) != NULL){
			client->ttl = 5;
		} else{
			printf("Cannot ping subscriber: not subscribed!\n");
		}
		pthread_mutex_unlock(&m);
	} else{
		printf("Unrecognized system message:\n%s\n", msg);
	}
}

void* ttl_update(void* params){
	struct connection_node* iterator;
	while(program_running){
		iterator = first;
		pthread_mutex_lock(&m);
		while(iterator != NULL){
			if(--iterator->ttl < 0){
				//Remove user: timeout
				struct connection_node* local = iterator;
				iterator = iterator->next;
				dll_remove(local->connection, &first, &last);
				curr_connections--;
				printf("User timeout: disconnected\n");
			} else
				iterator = iterator->next;
		}
		pthread_mutex_unlock(&m);
		sleep(2);
	}
	return NULL;
}

void* handle_client(void* params){
	struct client* c_handle = (struct client*) params;
	struct sockaddr_in sc = *c_handle->addr;
	char* buf = c_handle->msg;

	char conn_addr[20];
	inet_ntop(AF_INET, &(sc.sin_addr), conn_addr, 20);
	//printf("Received a packet from %s\n: %s\n", conn_addr, buf);
	if(strncmp(buf, "SYSTEM", 6)== 0){
		printf("Received system packet from %s:\n\t%s\n", conn_addr, buf);
		parse_system_message(buf, sc);
	} else{
		pthread_mutex_lock(&m);
		if (dll_contains(&sc, &first, &last) != NULL){
			printf("%s\n", buf);
			//Resend to every subscribed cllient
			struct connection_node* iterator = first;
			while(iterator != NULL){
				sendto(sock_id, buf, 80, 0, (struct sockaddr*)iterator->connection, sizeof(struct sockaddr_in));
				iterator = iterator->next;
			}
		} else
			printf("Non-subscribed client tried to send message: %s\n", conn_addr);
		pthread_mutex_unlock(&m);
	}
	free(buf);
	free(c_handle->addr);
	free((struct client*) params);
	return NULL;
}
