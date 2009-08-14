
CSRC=mt.c
CPPSRC=tongseng.cpp \
	TUIO/TuioClient.cpp \
	TUIO/TuioServer.cpp \
	TUIO/TuioTime.cpp \
	oscpack/ip/IpEndpointName.cpp \
	oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp \
	oscpack/osc/OscOutboundPacketStream.cpp \
	oscpack/osc/OscPrintReceivedElements.cpp \
	oscpack/osc/OscReceivedElements.cpp \
	oscpack/osc/OscTypes.cpp

OBJS=$(CPPSRC:.cpp=.o) $(CSRC:.c=.o)
CPPFLAGS=-ITUIO -Ioscpack
CFLAGS=
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport
BIN=tongseng

all : $(BIN)

.cpp.o :
	g++ -c $(CPPFLAGS) $< -o $@

.c.o :
	gcc -c $(CFLAGS) $< -o $@

$(BIN) : $(OBJS)
	g++ -o $@ $(LIBS) $^

clean :
	rm -f $(BIN) $(OBJS)

