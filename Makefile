
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
	TUIO/TcpSender.cpp \
	TUIO/WebSockSender.cpp \
	TUIO/OneEuroFilter.cpp \
	TUIO/TuioClient.cpp \
	TUIO/OscReceiver.cpp \
	TUIO/UdpReceiver.cpp \
	TUIO/TcpReceiver.cpp \
	TouchInjector.cpp \
	oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp \
	oscpack/osc/OscOutboundPacketStream.cpp \
	oscpack/osc/OscReceivedElements.cpp \
	oscpack/osc/OscTypes.cpp

OBJS=$(SRC:.cpp=.o)
CPPFLAGS=-ITUIO -Ioscpack
# keep the binary runnable on pre-macOS 27 systems; without an explicit
# floor ld stamps the build OS version and the binary refuses to launch
# on anything older
CXXFLAGS=-mmacosx-version-min=14.6
LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport -framework CoreFoundation -framework ApplicationServices -framework IOKit
BIN=tongseng

all : $(BIN)

.cpp.o :
	g++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BIN) : $(OBJS)
	g++ $(CXXFLAGS) -o $@ $(LIBS) $^

clean :
	rm -f $(BIN) $(OBJS)

