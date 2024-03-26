.PHONY: all clean

RM = rm
RMF = -rf

CC = gcc
CFLAGS = -g
LIBFLAGS =-lpthread -lrt

EXE = kv_server
EXE2 = kv_client

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o,$(notdir ${SRC})) 

all: $(EXE) $(EXE2)

${EXE}:kvdb_server.o kvdb.o kvdb.h
	$(CC) kvdb_server.o kvdb.o -o $@ $(LIBFLAGS)

${EXE2}:kvdb_client.o kvdb.h
	$(CC) kvdb_client.o -o $@ $(LIBFLAGS)
	
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RMF) $(EXE) $(EXE2) *.o
	