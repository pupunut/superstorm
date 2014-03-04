INCDIR :=$(shell pwd)

CC := g++
CFLAGS := -ggdb -std=gnu++0x
LDFLAGS := -luuid -lsqlite3

SRC := t0.cpp
OBJ := $(patsubst %.cpp, %.oo, $(SRC)) 
HEADERS := $(wildcard $(INCDIR)/*.h)
EXE := t0

.PHONY: all install clean test

all: t0

t0: $(OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OBJ) -o t0

%.oo: %.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	-rm -f $(OBJ) $(EXE)
