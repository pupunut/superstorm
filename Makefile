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

t0_OBJ := $(filter-out drop_simulator.oo, $(OBJ))
drop_OBJ := $(filter-out t0_simulator.oo, $(OBJ))
EXE := t0_simulator
EXE += drop_simulator

.PHONY: all install clean test

all: $(EXE)

t0_simulator: $(t0_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(t0_OBJ) -o t0_simulator

drop_simulator: $(drop_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $(drop_OBJ) -o drop_simulator

%.oo: %.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	-rm -f $(OBJ) $(EXE)
