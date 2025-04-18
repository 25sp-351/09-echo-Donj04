MAIN = echo

OBJS = $(MAIN).o

CC = gcc
CFLAGS = -g -Wall -Wextra


# compile commands

all: build

build: $(MAIN)


# dependencies

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(MAIN).o: $(MAIN).c


# remove generated files

clean:
	rm -f $(MAIN) $(OBJS)
