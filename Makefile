
SRC=main.cpp \
	tongseng.cpp \
	TUIO/TuioServer.cpp \
	TUIO/TuioTime.cpp \
	TUIO/TuioManager.cpp \
	TUIO/TuioObject.cpp \
	TUIO/TuioCursor.cpp \
	TUIO/TuioContainer.cpp \
	TUIO/TuioPoint.cpp \
	TUIO/TuioBlob.cpp \
	TUIO/TuioDispatcher.cpp \
	TUIO/UdpSender.cpp \
	oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp \
	oscpack/osc/OscOutboundPacketStream.cpp \
	oscpack/osc/OscTypes.cpp

OBJS=$(SRC:.cpp=.o)
CPPFLAGS=-ITUIO -Ioscpack
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport -framework Carbon
BIN=tongseng

all : $(BIN)

.cpp.o :
	g++ -c $(CPPFLAGS) $< -o $@

$(BIN) : $(OBJS)
	g++ -o $@ $(LIBS) $^

clean :
	rm -f $(BIN) $(OBJS)

install:
	cp tongseng /usr/bin