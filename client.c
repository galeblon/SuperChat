#include<WinSock2.h>
#include<stdio.h>

#define ST_PORT 61234

int main(int argc, char* argv[]) {
	SOCKET s;
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
		return;
	}
	/*
	struct sockaddr my_conn;
	int len;
	getsockname(s, &my_conn, &len);
	struct sockaddr_in *my_conn_in = (struct sockaddr_in*) my_conn;
	printf("%d", my_conn_in->sin_port);
	*/
	
	int dlug;
	char buf[80];
	for (;;) {
		fgets(buf, 80, stdin);
		dlug = strlen(buf);
		buf[dlug - 1] = '\0';
		send(s, buf, dlug, 0);
		if (strcmp(buf, "KONIEC") == 0) break;
	}
	closesocket(s);
	WSACleanup();
	return 0;
}