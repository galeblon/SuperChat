#!/bin/bash

gcc -c server_tcp_unix.c
gcc server_tcp_unix.o -lpthread -o s_tcp_unix.out
