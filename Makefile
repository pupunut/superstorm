INCDIR :=$(shell pwd)/include

CC := g++
CFLAGS := -ggdb -std=gnu++0x
CFLAGS += -I$(INCDIR)
LDFLAGS := -luuid -lsqlite3

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   CFLAGS += -D LONG_BIT=64
else
   CFLAGS += -D LONG_BIT=32
endif

SRC := $(shell ls *.cpp)
OBJ := $(patsubst %.cpp, %.oo, $(SRC)) 
HEADERS := $(wildcard $(INCDIR)/*.h)
EXE := t0_simulator

.PHONY: all install clean test

all: $(EXE)

t0_simulator: $(OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(OBJ) -o t0_simulator

%.oo: %.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	-rm -f $(OBJ) $(EXE)
