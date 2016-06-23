
SRC=main.cpp \
	tongseng.cpp \
	TUIO/TuioServer.cpp \
	TUIO/TuioTime.cpp \
	TUIO/TuioManager.cpp \
	TUIO/TuioObject.cpp \
	TUIO/TuioCursor.cpp \
	TUIO/TuioBlob.cpp \
	TUIO/TuioContainer.cpp \
	TUIO/TuioPoint.cpp \
	TUIO/TuioBlob.cpp \
	TUIO/TuioDispatcher.cpp \
	TUIO/UdpSender.cpp \
	TUIO/OneEuroFilter.cpp \
	oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp \
	oscpack/osc/OscOutboundPacketStream.cpp \
	oscpack/osc/OscTypes.cpp

OBJS=$(SRC:.cpp=.o)
CPPFLAGS=-ITUIO -Ioscpack
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport -framework CoreFoundation
BIN=tongseng

all : $(BIN)

.cpp.o :
	g++ $(CPPFLAGS) -c $< -o $@

$(BIN) : $(OBJS)
	g++ -o $@ $(LIBS) $^

clean :
	rm -f $(BIN) $(OBJS)

