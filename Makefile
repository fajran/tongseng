
CPPFLAGS=-ITUIO -Ioscpack
CFLAGS=
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport

all : TuioPad

mt.o : mt.c
	gcc -c $(CFLAGS) $?

TuioPad.o : TuioPad.cpp
	g++ -c $(CPPFLAGS) $?

TuioPad : TuioPad.o mt.o TUIO/*.cpp oscpack/osc/*.cpp oscpack/ip/*.cpp oscpack/ip/posix/*.cpp
	g++ -o $@ $(LIBS) $(CPPFLAGS) $^

clean :
	rm -f TuioPad mt.o TuioPad.o

