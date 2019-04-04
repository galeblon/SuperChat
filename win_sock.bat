gcc -c client.c
gcc -c server.c
gcc client.o -lwsock32 -o c.exe
gcc server.o -lwsock32 -o s.exe
