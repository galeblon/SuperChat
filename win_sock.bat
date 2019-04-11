gcc -c server_tcp.c -std=c99
gcc -c server_udp.c -std=c99
gcc -c client_tcp.c -std=c99
gcc -c client_udp.c -std=c99
gcc client_tcp.o -lwsock32 -o c_tcp.exe
gcc client_udp.o -lwsock32 -o c_udp.exe
gcc server_tcp.o -lwsock32 -o s_tcp.exe
gcc server_udp.o -lwsock32 -o s_udp.exe
