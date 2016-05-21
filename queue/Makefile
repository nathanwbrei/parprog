NAME = nathansqueue 
CC = gcc
CFLAGS = -std=gnu99 -fstrict-overflow -Werror -Wall -Wshadow -pedantic -I$(CURDIR)
LDFLAGS = -lm -lpthread -ldl
DEST = bin

#Compiler flags
#if mode variable is empty, setting release build mode
ifeq ($(mode),debug)
	CFLAGS += -O0 -g
else
	mode = release
	CFLAGS += -O2
endif

SRC = $(wildcard *.c) $(wildcard */*.c)

all:
	$(CC) $(CFLAGS) $(SRC) -o $(NAME) $(LDFLAGS)

clean:
	rm -f *.o
	

