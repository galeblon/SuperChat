#include<WinSock2.h>
#include<stdio.h>
#include<windows.h>
#include<conio.h>
#define ST_PORT 61234

void set_username();
void strjoi(char* to, char* f1, char* f2, int l1, int l2);
void fill_buf(char* buf, int len);
DWORD WINAPI read_t(void *params);

SOCKET s;
int dlug;
int curr_len = 0;
char buf[75];
char namebuf[5];
char fullbuf[80];
int exit_program = 0;

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
	
	set_username();

	for (;;) {	
		// Get input from keyboard
		int len = 75;

		fill_buf(buf, len);

		strjoi(fullbuf, namebuf, buf, 5, 75);
		
		send(s, fullbuf, 80, 0);
		if (strcmp(buf, "KONIEC") == 0) break;
		
	}
	exit_program = 1;
	DWORD dwEvent = WaitForSingleObject( 
        thread,       		// object
        5000); 	 			// five second wait
	closesocket(s);
	WSACleanup();
	return 0;
}


DWORD WINAPI read_t(void * params){
	char buf_r[80];
	while(!exit_program)
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

void fill_buf(char* buf, int len) {
	char key;
	// 32 - enter key
	while ((key = getch()) != 13 || curr_len == 0) { // Enter
		if (curr_len > 0 && key == 8) { // Backspace
			curr_len--;
			printf("\b \b");
		}
		if (curr_len < (len - 1) && key != 13 && key != 8) {
			//printf("KEY:%d|", key);
			buf[curr_len++] = key;
			printf("%c", key);
		}
	}
	printf("\n");
	//clear the rest of line;
	while (curr_len < len)
		buf[curr_len++] = '\0';

	curr_len = 0;
}

void strjoi(char* to, char* f1, char* f2, int l1, int l2) {
	int toLoc = 0, fromLoc;
	for (fromLoc = 0; fromLoc < l1; fromLoc++) {
		to[toLoc] = f1[fromLoc];
		toLoc++;
	}
	for (fromLoc = 0; fromLoc < l2; fromLoc++) {
		to[toLoc] = f2[fromLoc];
		toLoc++;
	}
}

void set_username() {
	printf("First, please choose your username, up to 3 letters in length.\n");
	fill_buf(namebuf, 4);
	namebuf[3] = '>';
	namebuf[4] = ' ';
}