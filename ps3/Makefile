.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT must be set in the environment.")
endif

include $(PSL1GHT)/Makefile.base

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCE		:=	../
INCLUDE		:=	include
DATA		:=	data
LIBS		:=	-lgcm_sys -lreality -lsysutil -lio -lm -lstdc++

TITLE		:=	nullDCe/PS3
APPID		:=	TEST0VID3
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000
ICON0		:=	ICON0.PNG

CFLAGS		+= -g -O2 -Wall --std=gnu99 -I
CXXFLAGS	+= -g -O2 -Wall -I /home/skmp/ndce/stable/
CXXFLAGS	+= -D _PS3 -D RELEASE 
CXXFLAGS	+= -fno-exceptions -fno-rtti -frename-registers -fno-strict-aliasing -fsingle-precision-constant -fomit-frame-pointer -ffast-math

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCE),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export BUILDDIR	:=	$(CURDIR)/$(BUILD)
export DEPSDIR	:=	$(BUILDDIR)

CFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CXXFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))

CXXFILES = \
 ./cl/cl.cpp \
 ./config/config.cpp \
 ./dc/aica/aica_if.cpp \
 ./dc/asic/asic.cpp \
 ./dc/dc.cpp \
 ./dc/gdrom/gdromv3.cpp \
 ./dc/gdrom/gdrom_response.cpp \
 ./dc/maple/maple_if.cpp \
 ./dc/maple/maple_cfg.cpp \
 ./dc/maple/maple_devs.cpp \
 ./dc/maple/maple_helper.cpp \
 ./dc/mem/memutil.cpp \
 ./dc/mem/mmu.cpp \
 ./dc/mem/sb.cpp \
 ./dc/mem/sh4_area0.cpp \
 ./dc/mem/sh4_internal_reg.cpp \
 ./dc/mem/sh4_mem.cpp \
 ./dc/mem/_vmem.cpp \
 ./dc/pvr/pvrLock.cpp \
 ./dc/pvr/pvr_if.cpp \
 ./dc/pvr/pvr_sb_regs.cpp \
 ./dc/sh4/bsc.cpp \
 ./dc/sh4/ccn.cpp \
 ./dc/sh4/cpg.cpp \
 ./dc/sh4/dmac.cpp \
 ./dc/sh4/intc.cpp \
 ./dc/sh4/rtc.cpp \
 ./dc/sh4/sci.cpp \
 ./dc/sh4/scif.cpp \
 ./dc/sh4/sh4_cpu.cpp \
 ./dc/sh4/sh4_fpu.cpp \
 ./dc/sh4/sh4_if.cpp \
 ./dc/sh4/sh4_interpreter.cpp \
 ./dc/sh4/sh4_opcode_list.cpp \
 ./dc/sh4/sh4_registers.cpp \
 ./dc/sh4/tmu.cpp \
 ./dc/sh4/ubc.cpp \
 ./dc/sh4/rec_v2/blockmanager.cpp \
 ./dc/sh4/rec_v2/driver.cpp \
 ./dc/sh4/rec_v2/decoder.cpp \
 ./nullDC.cpp \
 ./plugins/plugin_manager.cpp \
 ./plugins/plugin_types.cpp \
 ./stdclass.cpp \
 ./plugs/nullExtDev/nullExtDev.cpp \
 ./plugs/drkMapleDevices/drkMapleDevices.cpp \
 ./plugs/EmptyAICA/aica_hax.cpp \
 ./plugs/EmptyAICA/aica_hle.cpp \
 ./plugs/EmptyAICA/EmptyAICA.cpp \
 ./plugs/ImgReader/ImgReader.cpp \
 ./plugs/ImgReader/cdi.cpp \
 ./plugs/ImgReader/common.cpp \
 ./plugs/ImgReader/iso9660.cpp \
 ./plugs/ImgReader/mds.cpp \
 ./plugs/ImgReader/mds_reader.cpp \
 ./plugs/drkPvr/drkPvr.cpp \
 ./plugs/drkPvr/regs.cpp \
 ./plugs/drkPvr/glesRend.cpp \
 ./plugs/drkPvr/tex_decode.cpp \
 ./plugs/drkPvr/Renderer_if.cpp \
 ./plugs/drkPvr/SPG.cpp \
 ./plugs/drkPvr/softRend.cpp \
 ./plugs/drkPvr/ta.cpp \
 ./ps3/main_psl1ght.cpp

export OFILES	:=	$(CFILES:.c=.o) \
					$(CXXFILES:.cpp=.o) \
					$(SFILES:.S=.o) \
					$(BINFILES:.bin=.bin.o)

export BINFILES	:=	$(BINFILES:.bin=.bin.h)

export INCLUDES	:=	$(foreach dir,$(INCLUDE),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean pkg run

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Clean...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT).pkg

pkg: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON0) $(BUILD)/pkg/
	@$(FSELF) -n $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN
	@$(SFO) --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).pkg

run: $(BUILD)
	@$(PS3LOADAPP) $(OUTPUT).self

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(BINFILES)

-include $(DEPENDS)

endif
