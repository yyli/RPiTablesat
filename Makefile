CC = g++
CROSSCC = /opt/cross/x-tools/arm-unknown-linux-gnueabi/bin/arm-unknown-linux-gnueabi-g++

BIN = command_fan read_gyro tsat_drivers.a RPiDecode.a
CFLAGS = -Wall
SERVERLDFLAGS = -lilclient -lpthread -ldc1394 -L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -L/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/src/hello_pi/libs/vgfont -ludt
SERVERCFLAGS = -O3 -DNOOPENCV -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
SERVERINCLUDES = -I$(SDKSTAGE)/opt/vc/include/ -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I./ -I/opt/vc/src/hello_pi/libs/ilclient -I/opt/vc/src/hello_pi/libs/vgfont

all: $(BIN)

command_fan: command_fan.cpp tsat_drivers.a
	$(CROSSCC) $(CFLAGS) $^ -o $@

read_gyro: read_gyro.cpp tsat_drivers.a
	$(CROSSCC) $(CFLAGS) $^ -o $@

tsat_drivers.o: tsat_drivers.cpp
	$(CROSSCC) $(CFLAGS) $^ -c -o $@

tsat_drivers.a: tsat_drivers.o
	ar crv $@ $^

udtComm.o: udtComm.cpp 
	$(CC) -O3 -Wall -D__STDC_CONSTANT_MACROS -c $^ -o $@ `pkg-config libavcodec libavutil libswscale opencv --cflags`

RPiDecode.o: RPiDecode.cpp
	$(CC) -O3 -Wall -D__STDC_CONSTANT_MACROS -c $^ -o $@ `pkg-config libavcodec libavutil libswscale opencv --cflags`

RPiDecode.a: udtComm.o RPiDecode.o
	ar crv $@ $^

camclient: camclient.cpp RPiDecode.a
	$(CC) -O3 -D__STDC_CONSTANT_MACROS -Wall $^ -o $@ `pkg-config libavcodec libavutil libswscale opencv --cflags --libs` -ludt

camserver: camserver.cpp camera.cpp udtComm.o
	$(CROSSCC) $(SERVERCFLAGS) $(SERVERINCLUDES) -o $@ -Wl,--whole-archive $^ $(SERVERLDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	rm -rf $(BIN)