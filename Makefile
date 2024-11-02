CC=g++
CFLAGS=-MMD -Wall -Wextra -pedantic -std=c++23

SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:%.cpp=%.o)
DEP=$(OBJ:%.o=%.d)

EXE=rtd
LIBS=$(addprefix -l,fmt raylib fontconfig)

TARGET=/usr/local

all: debug

debug: CFLAGS += -g
debug: $(EXE)

release: CFLAGS += -O3 -DNDEBUG
release: $(EXE)

install: all
	cp $(EXE) $(TARGET)/bin

clean:
	rm -f $(OBJ) $(DEP) $(EXE)

-include $(DEP)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
