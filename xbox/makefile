#
# update this variable to wherever you installed the OpenXDK libraries
#
PREFIX = c:/MinGW

CC = gcc
CCAS = gcc
CPP  = g++
CXBE = $(PREFIX)/bin/cxbe.bat

SDLFLAGS = -DENABLE_XBOX -DDISABLE_CDROM 
CC_FLAGS = -c -g -std=gnu99 -ffreestanding -nostdlib -fno-builtin -fno-exceptions -mno-cygwin -march=i386 $(SDLFLAGS) -D _LINUX -D _X86 -I../
CCAS_FLAGS = -g -O2
INCLUDE  = -I$(PREFIX)/include -I$(PREFIX)/include -I$(PREFIX)/include/SDL 

CLINK = -nostdlib
ALIGN = -Wl,--file-alignment,0x20 -Wl,--section-alignment,0x20 
SHARED = -shared
ENTRYPOINT = -Wl,--entry,_WinMainCRTStartup 
STRIP = -Wl,--strip-all
LD_FLAGS = $(CLINK) $(ALIGN) $(SHARED) $(ENTRYPOINT) $(STRIP)
LD_DIRS = -L$(PREFIX)/i386-pc-xbox/lib -L$(PREFIX)/lib 
LD_LIBS  = $(LD_DIRS) -lSDL -lm -lopenxdk -lhal -lc -lusb -lc -lxboxkrnl -lc -lhal -lxboxkrnl -lhal -lopenxdk -lc

#LD_LIBS = -lSDL_ttf -lfreetype2 -lSDL_image -lSDL -ljpeg -lpng -lz -lxml2 -lm -lhal -lusb -lopenxdk -lhal -lc -lxboxkrnl -lstdc++ -lgcc

OBJS = \
 ../cl/cl.o \
 ../config/config.o \
 ../dc/aica/aica_if.o \
 ../dc/asic/asic.o \
 ../dc/dc.o \
 ../dc/gdrom/gdromv3.o \
 ../dc/gdrom/gdrom_response.o \
 ../dc/maple/maple_if.o \
 ../dc/mem/memutil.o \
 ../dc/mem/mmu.o \
 ../dc/mem/sb.o \
 ../dc/mem/sh4_area0.o \
 ../dc/mem/sh4_internal_reg.o \
 ../dc/mem/sh4_mem.o \
 ../dc/mem/_vmem.o \
 ../dc/pvr/pvrLock.o \
 ../dc/pvr/pvr_if.o \
 ../dc/pvr/pvr_sb_regs.o \
 ../dc/sh4/bsc.o \
 ../dc/sh4/ccn.o \
 ../dc/sh4/cpg.o \
 ../dc/sh4/dmac.o \
 ../dc/sh4/intc.o \
 ../dc/sh4/rtc.o \
 ../dc/sh4/sci.o \
 ../dc/sh4/scif.o \
 ../dc/sh4/sh4_cpu.o \
 ../dc/sh4/sh4_fpu.o \
 ../dc/sh4/sh4_if.o \
 ../dc/sh4/sh4_interpreter.o \
 ../dc/sh4/sh4_opcode_list.o \
 ../dc/sh4/sh4_registers.o \
 ../dc/sh4/tmu.o \
 ../dc/sh4/ubc.o \
 ../dc/sh4/rec_v2/blockmanager.o \
 ../dc/sh4/rec_v2/driver.o \
 ../dc/sh4/rec_v2/decoder.o \
 ../debug_mem.o \
 ../gui/base.o \
 ../nullDC.o \
 ../plugins/plugin_manager.o \
 ../plugins/plugin_types.o \
 ../profiler/profiler.o \
 ../serial_ipc/serial_ipc_client.o \
 ../stdclass.o \
 ../plugs/nullExtDev/nullExtDev.o \
 ../plugs/drkMapleDevices/drkMapleDevices.o \
 ../plugs/EmptyAICA/aica_hax.o \
 ../plugs/EmptyAICA/aica_hle.o \
 ../plugs/EmptyAICA/EmptyAICA.o \
 ../plugs/ImgReader/ImgReader.o \
 ../plugs/ImgReader/cdi.o \
 ../plugs/ImgReader/common.o \
 ../plugs/ImgReader/iso9660.o \
 ../plugs/ImgReader/mds.o \
 ../plugs/ImgReader/mds_reader.o \
 ../plugs/gui/libgui.o \
 ../plugs/drkPvr/drkPvr.o \
 ../plugs/drkPvr/regs.o \
 ../plugs/drkPvr/nullRend.o \
 ../plugs/drkPvr/Renderer_if.o \
 ../plugs/drkPvr/SPG.o \
 ../plugs/drkPvr/ta.o \
 
  OBJS+=main.o

all: default.xbe

.cpp.o:
	$(CPP) -c $< $(CC_FLAGS) $(INCLUDE)
	
.c.o:
	$(CC) -c $< $(CC_FLAGS) $(INCLUDE)


.s.o:
	$(CCAS) -c $< $(CCAS_FLAGS)

default.exe: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LD_LIBS) $(LD_FLAGS)

default.xbe: default.exe
	cmd /c $(CXBE) >> cxbe.log

clean: 
	rm -f *.o *.exe *.dll *.xbe *.cxbe
