INCDIR :=$(shell pwd)/include

CC := g++
CFLAGS := -ggdb 
#CFLAGS += -std=gnu++0x
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
EXE += drop_simulator
EXE += find_low
EXE += eval_bp
EXE_OBJ := $(addsuffix .oo, $(EXE))

NON_EXE_OBJ := $(filter-out $(EXE_OBJ), $(OBJ))

t0_OBJ := $(NON_EXE_OBJ) t0_simulator.oo
drop_OBJ := $(NON_EXE_OBJ) drop_simulator.oo
find_low_OBJ := $(NON_EXE_OBJ) find_low.oo
eval_bp_OBJ := $(NON_EXE_OBJ) eval_bp.oo

.PHONY: all install clean test

all: $(EXE)

t0_simulator: $(t0_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $^ -o $@

drop_simulator: $(drop_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $^ -o $@

find_low: $(find_low_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $^ -o $@

eval_bp: $(eval_bp_OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) $^ -o $@

%.oo: %.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@ 

clean:
	-rm -f $(OBJ) $(EXE)
