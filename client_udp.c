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
struct sockaddr_in sa;
int sa_len;

int main(int argc, char* argv[]) {
	
	if (argc < 2)
		return 1;
	WSADATA wsas;
	WORD wersja;
	wersja = MAKEWORD(2, 0);
	WSAStartup(wersja, &wsas);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset((void *)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(ST_PORT);
	sa.sin_addr.s_addr = inet_addr(argv[1]);
	sa_len = sizeof(sa);

	// Subscribe to chat server

	set_username();
	sendto(s, "JOIN", 4, 0, (struct sockaddr*)&sa, sizeof sa);
	//Tworzenie wątku odczytującego datagramy UDP
	HANDLE thread;
	DWORD id;
	thread = CreateThread(
		NULL,
		0,
		read_t,
		NULL,
		0,
		&id);
	

	while(!exit_program) {	
		// Get input from keyboard
		int len = 75;

		fill_buf(buf, len);

		strjoi(fullbuf, namebuf, buf, 5, 75);

		sendto(s, fullbuf, 80, 0, (struct sockaddr*)&sa, sizeof sa);
		if (strcmp(buf, "KONIEC") == 0) break;
		curr_len = 0;
		
	}
	exit_program = 1;
	DWORD dwEvent = WaitForSingleObject( 
        thread,       		// object
        2000); 	 			// 2 second wait
	closesocket(s);
	WSACleanup();
	printf("Client shutdown..\n");
	return 0;
}


DWORD WINAPI read_t(void * params){
	char buf_r[80];
	while(!exit_program)
	{
		memset(buf_r, '\0', 80);
		recvfrom(s, buf_r, 80, 0, (struct sockaddr *) &sa, &sa_len);
		if (strcmp("KONIEC", buf_r) == 0)
			break;
		// Return to beginning of line to print new message.
		printf("\r%-80s\n", buf_r);
		// Reprint currently written message.
		for(int i=0; i<curr_len; i++)
			printf("%c", buf[i]);
	}
	exit_program = 1;
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
	//Enter was pressed, clear current line
	printf("\r%-80s", "");
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