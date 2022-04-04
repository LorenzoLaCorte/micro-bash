OBJS = microbash.o
SOURCE	= microbash.c
HEADER	= stdio.h string.h unistd.h stdlib.h sys/wait.h string.h readline/readline.h readline/history.h fcntl.h errno.h
OUT	= microbash
CC	 = gcc
FLAGS	 = -Wall -pedantic -Werror
LFLAGS	 = -lreadline

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

microbash: microbash.c
	$(CC) $(FLAGS) $(LFLAGS) microbash.c 

clean:
	rm -f $(OBJS) $(OUT)