#---------------------------------------------------------------------------------
# CLEAR VARIABLES
#---------------------------------------------------------------------------------
export DEVKITPRO	:=	/opt/devkitpro
export TOPDIR	:=	$(CURDIR)
export PREFIX	:=	aarch64-none-elf-
export PATH	:=	$(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PATH)
export LD	:=	aarch64-none-elf-g++

export TARGET		:=	3ds_to_zcci_compressor
export APP_TITLE	:=	3DS to ZCCI Converter
export APP_AUTHOR	:=	elcoke.cl
export APP_VERSION	:=	1.1.4
export APP_ICON	:=	../icon.jpg
export NROFLAGS	:=	--icon=$(APP_ICON) --nacp=$(OUTPUT).nacp

include $(DEVKITPRO)/libnx/switch_rules

# Override LDFLAGS with Windows-conforming path for libnx specs
LDFLAGS	:=	-specs=$(LIBNX)/switch.specs -g $(ARCH) -fPIE -pie -Wl,-Map,$(notdir $*.map)
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# OPTIONS FOR COMPILER
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft

CFLAGS	:=	-Wall -O2 -ffunction-sections -fdata-sections -fPIE $(ARCH)
CXXFLAGS:=	$(CFLAGS) -std=gnu++20

#---------------------------------------------------------------------------------
# LIST ALL LIB DIRECTORIES HERE
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(DEVKITPRO)/portlibs/switch $(LIBNX)

#---------------------------------------------------------------------------------
# no need to edit past this line
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LDFLAGS

.PHONY: $(BUILD) clean

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nacp

else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
DEPENDS	:=	$(OFILES:.o=.d)

CFLAGS	+=	$(INCLUDE)
CXXFLAGS+=	$(INCLUDE)

LIBS	:=	-lSDL2_image -lSDL2_ttf -lSDL2 -lglad -lEGL -lglapi -ldrm_nouveau -lfreetype -lzstd -lnx

all: $(OUTPUT).nro

$(OUTPUT).nro:	$(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf:	$(OFILES)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
