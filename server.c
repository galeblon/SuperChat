#include<WinSock2.h>
#include<windows.h>
#include<stdio.h>

#define ST_PORT 61234

DWORD WINAPI client_t(void* nibba);
SOCKET s;

int main() {
	WSADATA wsas;
	int result;
	WORD wersja;
	wersja = MAKEWORD(1, 1);
	result = WSAStartup(wersja, &wsas);
	//SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sa;
	memset((void *)(&sa), 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(ST_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	result = bind(s, (struct sockaddr FAR*)&sa, sizeof(sa));

	result = listen(s, 5);

	HANDLE watki[2];
	DWORD id;
	
	watki[0] = CreateThread(
			NULL,
			0,
			client_t,
			NULL, // tutej adres i port klienta
			0,
			&id
	);
	if(watki[0] == INVALID_HANDLE_VALUE){
	
	}
	
	watki[1] = CreateThread(
			NULL,
			0,
			client_t,
			NULL, // tutej adres i port klienta
			0,
			&id
	);
	
	Sleep(1000000);
	/*
	SOCKET si;
	struct sockaddr_in sc;
	int lenc;
	lenc = sizeof(sc);
	si = accept(s, (struct sockaddr FAR *)&sc, &lenc);
	char buf[80];
	while (recv(si, buf, 80, 0) > 0) {
		if (strcmp(buf, "KONIEC") == 0) {
			closesocket(si);
			WSACleanup();
			return 0;
		}
		printf("\n%s", buf);
	}
	return 0;
	*/
}


DWORD WINAPI client_t(void* nibba){
	printf("wontek pontek");
	
	SOCKET si;
	struct sockaddr_in sc;
	int lenc;
	lenc = sizeof(sc);
	si = accept(s, (struct sockaddr FAR *)&sc, &lenc);
	printf("Connected.");
	char buf[80];
	while (recv(si, buf, 80, 0) > 0) {
		if (strcmp(buf, "KONIEC") == 0) {
			closesocket(si);
			WSACleanup(); // tylko 1 raz
			return 0;
		}
		printf("\n%s", buf);
	}
	return 0;

}