
OBJS = \
 ../cl/cl.o \
 ../config/config.o \
 ../dc/aica/aica_if.o \
 ../dc/asic/asic.o \
 ../dc/dc.o \
 ../dc/gdrom/gdromv3.o \
 ../dc/gdrom/gdrom_response.o \
 ../dc/maple/maple_if.o \
 ../dc/maple/maple_helper.o \
 ../dc/maple/maple_cfg.o \
 ../dc/maple/maple_devs.o \
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
 ../plugs/drkPvr/gsRend.o \
 ../plugs/drkPvr/ta.o \
 

EE_INCS = -I../
EE_CFLAGS = -Os -G 0
EE_CFLAGS += -mno-gpopt
EE_CFLAGS += -Wall
EE_CFLAGS += -frename-registers -fno-strict-aliasing -fsingle-precision-constant -fomit-frame-pointer -ffast-math -ffunction-sections -fdata-sections
EE_CFLAGS += -DRELEASE -w  -flto
EE_CXXFLAGS = $(EE_CFLAGS) -fno-exceptions -fno-rtti -D _PS2 -D _MIPS -w 
EE_ASFLAGS = -c -x assembler-with-cpp

EE_BIN = nullDC.elf
EE_OBJS = main.o
EE_OBJS += $(OBJS)
EE_LIBS = -flto -Wl,--gc-sections -lm -lc -lstdc++ -ldraw -lgraph -lpacket -ldma

all: $(EE_BIN)

clean:
	rm $(EE_OBJS)
	
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal


