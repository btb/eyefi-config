#CC=$(CROSS_COMPILE)gcc -march=armv5te -mtune=xscale
#-march=armv5te  
#-marm
CFLAGS=-g -Wall
CXXFLAGS=`wx-config --cxxflags`

OBJS = eyefi-config.o sha1.o md5.o
cli_OBJS = eyefi-unix.o
gui_OBJS = eyefi-wxgui.o

PLATFORM := $(shell uname -s)

ifeq ($(PLATFORM),Linux)
	OBJS += eyefi-linux.o
endif
ifeq ($(PLATFORM),Darwin)
	OBJS += eyefi-osx.o
endif
ifeq ($(PLATFORM),FreeBSD)
	OBJS += eyefi-freebsd.o
endif

all: eyefi-config eyefi-config-gui

eyefi-config: $(OBJS) $(cli_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(cli_OBJS) -o $@

eyefi-config-gui: $(OBJS) $(gui_OBJS)
	$(CXX) $(CFLAGS) `wx-config --libs std,propgrid` $(OBJS) $(gui_OBJS) -o $@

clean:
	rm -f eyefi-config eyefi-config-gui core  $(OBJS) $(cli_OBJS) $(gui_OBJS) cscope*

eyefi-chdk.o: eyefi-config.h 
eyefi-config.o: eyefi-config.h
eyefi-linux.o: eyefi-config.h 
eyefi-unix.o: eyefi-config.h
eyefi-wxgui.o: eyefi-config.h
md5.o: eyefi-config.h
sha1.o: eyefi-config.h
