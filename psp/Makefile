PSP_EBOOT_TITLE = nullDC/PSP
TARGET = nullDCpsp
OBJS = \
 ../cl/cl.o \
 ../config/config.o \
 ../dc/aica/aica_if.o \
 ../dc/asic/asic.o \
 ../dc/dc.o \
 ../dc/gdrom/gdromv3.o \
 ../dc/gdrom/gdrom_response.o \
 ../dc/maple/maple_if.o \
 ../dc/maple/maple_devs.o \
 ../dc/maple/maple_cfg.o \
 ../dc/maple/maple_helper.o \
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
 ../dc/sh4/rec_v2/shil.o \
 ../nullDC.o \
 ../plugins/plugin_manager.o \
 ../plugins/plugin_types.o \
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
 ../plugs/drkPvr/drkPvr.o \
 ../plugs/drkPvr/regs.o \
 ../plugs/drkPvr/nullRend.o \
 ../plugs/drkPvr/Renderer_if.o \
 ../plugs/drkPvr/SPG.o \
 ../plugs/drkPvr/ta.o \
 
 #PSP only files
 OBJS+= \
 ../psp/dc/sh4/rec_v2/psp_driver.o \
 ../psp/plugs/drkPvr/pspDveManager.o \
 ../psp/main.o \

INCDIR = ../
CFLAGS = -O2 -G 0
#CFLAGS += -g
CFLAGS += -mno-gpopt
#CFLAGS += -fcall-saved-t4 -fcall-saved-t5 -fcall-saved-t6 -fcall-saved-t7
#CFLAGS += -fcall-saved-t8 -fcall-saved-t9
#CFLAGS += -fcall-saved-k0 -fcall-saved-k1
CFLAGS += -Wall
CFLAGS += -frename-registers -fno-strict-aliasing -fsingle-precision-constant -fomit-frame-pointer -ffast-math
CFLAGS += -DRELEASE -DNOPSPLINK
#CFLAGS += -save-temps
CXXFLAGS = -fno-exceptions -fno-rtti -D _PSP
ASFLAGS = -c -x assembler-with-cpp

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_ICON = ICON0.PNG

LIBDIR =
LIBS =  -lpspgum -lpspgu -lm -lstdc++ -lpsprtc -lpsppower -lpspusb
LDFLAGS =

PSP_FW_VERSION=390
BUILD_PRX=0
#PSP_LARGE_MEMORY=1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
