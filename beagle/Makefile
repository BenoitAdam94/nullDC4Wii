SOURCES	:=	../ ../cl/ ../config/ ../dc/aica/ ../dc/asic/ ../dc/ ../dc/gdrom/ ../dc/maple/ \
 ../dc/mem/ ../dc/pvr/ ../dc/sh4/ ../dc/sh4/rec_v2/ ../debug_mem.o ../plugins/ ../profiler/ ../serial_ipc/ \
 ../plugs/nullExtDev/ ../plugs/drkMapleDevices/ ../plugs/EmptyAICA/ ../plugs/ImgReader/ ../plugs/gui/ \
 ../plugs/drkPvr/ ../linux/ ../gui/

EXTRA_CPP = ../rec-ARM/armwreck.cpp ../rec-ARM/ngen_arm.o

CPPFILES := $(foreach dir,$(SOURCES),$(wildcard $(dir)*.cpp))
CPPFILES += $(EXTRA_CPP)
CXXFLAGS := -g -O2 -Wall  -D _BEAGLE -D RELEASE -c -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
CXXFLAGS += -frename-registers -fno-strict-aliasing -fsingle-precision-constant -ffast-math -ftree-vectorize -fomit-frame-pointer
CXXFLAGS += $(CFLAGS) -fno-exceptions -fno-rtti

LDFLAGS	:=	-g -Wl,-Map,$(notdir $@).map
LIBS    :=	-lm -lEGL -lGLESv2 -lglslcompiler -lIMGegl -lpvr2d -lsrv_um
INCDIR  := -I../

#LIBS     += -lX11 -lxcb -lXau -lXdmcp
#CXXFLAGS += -D SUPPORT_X11

#for profiling
EXTRAFLAGS := #-pg

CC=arm-none-linux-gnueabi-g++
OBJECTS=$(CPPFILES:.cpp=.o)
EXECUTABLE=nullDCe_arm.elf

all: $(CPPFILES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(EXTRAFLAGS) $(LDFLAGS) $(LIBS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(EXTRAFLAGS) $(INCDIR) $(CXXFLAGS) $< -o $@
	
.S.o:
	arm-none-linux-gnueabi-as -march=armv7-a -mfpu=neon -mfloat-abi=softfp $(INCDIR) $< -o $@
	
clean:
	rm $(OBJECTS) $(EXECUTABLE) -f