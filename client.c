#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#include<conio.h>
#define ST_PORT 61234

DWORD WINAPI read_t(void *params);

SOCKET s;
int dlug;
int curr_len = 0;
char buf[80];

int main(int argc, char* argv[]) {
	
	struct sockaddr_in sa;
	WSADATA wsas;
	WORD wersja;
	wersja = MAKEWORD(2, 0);
	WSAStartup(wersja, &wsas);
	s = socket(AF_INET, SOCK_STREAM, 0);
	memset((void *)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(ST_PORT);
	sa.sin_addr.s_addr = inet_addr(argv[1]);

	int result;
	result = connect(s, (struct sockaddr FAR *)&sa, sizeof(sa));
	if (result == SOCKET_ERROR) {
		printf("\nBlad polaczenia!");
		return 0;
	}
	//Tworzenie wątku odczytującego z socketa
	HANDLE thread;
	DWORD id;
	thread = CreateThread(
		NULL,
		0,
		read_t,
		NULL,
		0,
		&id);
	

	for (;;) {	
		// Get input from keyboard
		int len = 80;
		char key;
		// 32 - enter key
		while((key = getch()) != 13 || curr_len == 0){ // Enter
			if(curr_len > 0 && key == 8){ // Backspace
				curr_len--;
				printf("\b \b");
			}
			if(curr_len < (len-1) && key != 13 && key != 8){
				//printf("KEY:%d|", key);
				buf[curr_len++] = key;
				printf("%c", key);
			}
		}
		printf("\n");
		//clear the rest of line;
		while(curr_len < len)
			buf[curr_len++] = '\0';

		send(s, buf, len, 0);
		if (strcmp(buf, "KONIEC") == 0) break;
		curr_len = 0;
		
	}
	closesocket(s);
	WSACleanup();
	return 0;
}


DWORD WINAPI read_t(void * params){
	char buf_r[80];
	while(1)
	{
		if(recv(s, buf_r, 80, 0) > 0){
			// Return to beginning of line to print new message.
			printf("\r%-80s\n", buf_r);
			// Reprint currently written message.
			for(int i=0; i<curr_len; i++)
				printf("%c", buf[i]);
		}
	}
}