#include<WinSock2.h>
#include<windows.h>
#include<stdio.h>

#define ST_PORT 61234
#define MAX_CONNECTIONS 3
#define BUF_LEN 80

DWORD WINAPI listener_t(void *params);
SOCKET s;
unsigned int curr_connections = 0;

HANDLE *threads_c;
struct sockaddr_in users[MAX_CONNECTIONS];
int socket_free[MAX_CONNECTIONS];

int exit_program;

int main() {
	WSADATA wsas;
	int result;
	WORD wersja;
	wersja = MAKEWORD(1, 1);
	result = WSAStartup(wersja, &wsas);
	s = socket(AF_INET, SOCK_DGRAM, 17);
	curr_connections = 0;
	exit_program = 0;
	for(int i=0; i<MAX_CONNECTIONS; i++)
		socket_free[i] = 1;
	
	struct sockaddr_in sa;
	memset((void *)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(ST_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	result = bind(s, (struct sockaddr FAR*)&sa, sizeof(sa));
	if (result == SOCKET_ERROR) {
		printf("\nBlad polaczenia!");
		return 0;
	}

	//Create a thread to listen and send UDP datagrams
	HANDLE thread;
	DWORD id;
	thread = CreateThread(
		NULL,
		0,
		listener_t,
		NULL,
		0,
		&id);


	char key = ' ';
	while(key != 'q'){
		key = fgetc(stdin);
	}

	exit_program = 1;
	DWORD dwEvent = WaitForSingleObject(
		thread,       		// object
		2000); 	 			// 2 second wait

	// Tell everyone to go home
	for (int i = 0; i < curr_connections; i++) {
		sendto(s, "KONIEC", 6, 0, (struct sockaddr*)&(users[i]),
			sizeof(struct sockaddr_in));
	}
	closesocket(s);
	WSACleanup(); // tylko 1 raz
	printf("\nServer shutdown..\n");
	return 0;

}


DWORD WINAPI listener_t(void *params) {
	char buf[BUF_LEN];
	struct sockaddr from;
	int fromlen = sizeof(from);
	while (!exit_program) {
		memset(buf, '\0', BUF_LEN);
		if (recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr *) &from, &fromlen) > 0) {
			if (strcmp("JOIN", buf) == 0 && curr_connections < MAX_CONNECTIONS) {
				for(int i=0; i<MAX_CONNECTIONS; i++){
					if(socket_free[i]){
						// Remember user
						memcpy(&users[i], &from, sizeof(struct sockaddr_in));
						curr_connections++;
						socket_free[i] = 0;
						printf("New user connected\n");
						break;
					}
				}
			}
			else if (strcmp("KONIEC", buf+5) == 0) {
				// Search for user in user array and remove him.
				curr_connections -= 1;
				for(int index=0; index<MAX_CONNECTIONS; index++){
					if(memcmp(&users[index], &from, sizeof(struct sockaddr_in)) == 0){
						socket_free[index] = 1;
						printf("User disconnected\n");
						break;
					}
				}
			}
			int is_user = 0;
			// Resend received message to everyone
			for(int i=0; i<MAX_CONNECTIONS; i++){
				if(memcmp(&users[i], &from, sizeof(struct sockaddr_in)) == 0){
				is_user = 1;
				break;
				}
			}
			if(!is_user)
				continue;
			printf("%s\n", buf);
			for (int i = 0; i < MAX_CONNECTIONS; i++) {
				if(socket_free[i] == 0)
					sendto(s, buf, 80, 0, (struct sockaddr*)&(users[i]), sizeof(struct sockaddr_in));
			}
		}
	}
}