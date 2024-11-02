CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c++23

SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:%.cpp=%.o)
HDR=$(wildcard src/*.hpp)

EXE=rtd
LIBS=$(addprefix -l,fmt raylib fontconfig)

TARGET=/usr/local

all: debug

debug: CFLAGS += -g
debug: $(EXE)

remake: clean debug
.NOTPARALLEL: remake

release: CFLAGS += -O3 -DNDEBUG
release: clean $(EXE)
.NOTPARALLEL: release

install: all
	cp $(EXE) $(TARGET)/bin

clean:
	rm -f $(OBJ) $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

$(OBJ): $(HDR)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
