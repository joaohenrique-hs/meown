CC=gcc

meown: meown.c debug
	$(CC) meown.c -o meown -Wall -Wextra -pedantic -std=c99

debug: meown.c
	$(CC) -g meown.c -o dmeown
