#include<WinSock2.h>
#include<windows.h>
#include<stdio.h>

#define ST_PORT 61234
#define MAX_CONNECTIONS 8

DWORD WINAPI client_t(void *params);
DWORD WINAPI listener_t(void *params);
SOCKET s;
unsigned int curr_connections;

HANDLE threads[MAX_CONNECTIONS + 1]; // 1 thread per connection + additional one for accepting connections
HANDLE *threads_c;
SOCKET sockets[MAX_CONNECTIONS];

int exit_program;

int main() {
	WSADATA wsas;
	int result;
	WORD wersja;
	wersja = MAKEWORD(1, 1);
	result = WSAStartup(wersja, &wsas);
	s = socket(AF_INET, SOCK_STREAM, 0);
	curr_connections = 0;
	threads_c = threads + 1;		// offset by one thread which is dedicated to listening for connections;
	exit_program = 0;
	
	struct sockaddr_in sa;
	memset((void *)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(ST_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	result = bind(s, (struct sockaddr FAR*)&sa, sizeof(sa));

	result = listen(s, MAX_CONNECTIONS);
	
	// Create thread responsible for listening to connections
	DWORD id;
	threads[0] = CreateThread(
		NULL,
		0,
		listener_t,
		NULL,
		0,
		&id);
	if(threads[0] != INVALID_HANDLE_VALUE){
		// Successfully created a listener thread
		SetThreadPriority(threads[0], THREAD_PRIORITY_NORMAL);
	}
	
	char key = ' ';
	while(key != 'q'){
		key = fgetc(stdin);
	}
	
	// End the server
	exit_program = 1;
	DWORD dwEvent = WaitForMultipleObjects( 
        1+curr_connections,	// number of objects in array
        threads,       		// array of objects
        TRUE,   
        50000); 	 		// five second wait
	WSACleanup(); // tylko 1 raz
	printf("\nServer shutdown..\n");
	return 0;

}

// Listens for new connections 
DWORD WINAPI listener_t(void *params){
	printf("\nListening for connections...");
	SOCKET si;
	DWORD id;
	struct sockaddr_in sc;
	int lenc;
	lenc = sizeof(sc);
	
	fd_set can_read;
	const struct timeval timeout = {10, 0};
	while(!exit_program){
		FD_ZERO(&can_read);
		FD_SET(s, &can_read);
		if(select(1, &can_read, NULL, NULL, &timeout) == 1){
			if(curr_connections < MAX_CONNECTIONS){
				// There is something to read and our server has free slot for connection
				printf("\nThere is a connection!");
				sockets[curr_connections] = accept(s, (struct sockaddr FAR *)&sc, &lenc);
				threads_c[curr_connections] = CreateThread(
					NULL,
					0,
					client_t,
					(void*)&sockets[curr_connections],
					0,
					&id);
					if(threads_c[curr_connections] != INVALID_HANDLE_VALUE){
						// Successfully created a client thread
						SetThreadPriority(threads[0], THREAD_PRIORITY_NORMAL);
						curr_connections += 1;
					}
					
				
			}
		}
		
	}
	printf("\nStopping listening for connections...");
	return 0;
}

DWORD WINAPI client_t(void * params){
	SOCKET* si = (SOCKET*) params;
	printf("\nConnected.");
	char buf[80];
	
	fd_set can_read;
	const struct timeval timeout = {10, 0};
	while(!exit_program){
		FD_ZERO(&can_read);
		FD_SET(*si, &can_read);
		if(select(1, &can_read, NULL, NULL, &timeout) == 1){
			if(recv(*si, buf, 80, 0) > 0){
				if (strcmp(buf, "KONIEC") == 0) {
					closesocket(*si);
					printf("\nConnected closed.");
					return 0;
				}
				printf("\n%s", buf);
			} else {
				printf("\nAbrupt connection end.");
				return 0;
			}
		}
	}
	return 0;
}