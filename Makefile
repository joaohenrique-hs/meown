CC=gcc

meown: meown.c
	$(CC) meown.c -o meown -Wall -Wextra -pedantic -std=c99
