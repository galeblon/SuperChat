gcc -c client.c -std=c99
gcc -c server.c -std=c99
gcc client.o -lwsock32 -o c.exe
gcc server.o -lwsock32 -o s.exe
