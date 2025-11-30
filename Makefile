# Debian package dependencies:
# apt install build-essential wine32-tools libwine-dev libudev-dev:i386 libusb-1.0-0-dev:i386 libpipewire-0.3-dev:i386

# Build environment
WINEBUILD := /usr/bin/winebuild
WINECXX := wineg++
CC := gcc
#WINERC := wrc

M=32 #ARCH=i386
#M=64 #ARCH=x86_64

# Project settings
DIR_BLD := bld
DIR_SRC := RS_ASIO
LIB_NAME := RS_ASIO.dll

# The windows libraries
LIBS := -lpropsys -lole32
# The linux/wine libraries, using pkg-config to automatically select the 32-bit ones
#PKG_CONFIG := libudev libusb-1.0 libpipewire-0.3

# Suppressing "note:" via '-fcompare-debug-second': https://unix.stackexchange.com/a/548801
CFLAGS := -g -fPIC -std=c++20 -m$(M) \
	-Wno-pragma-pack -Wno-missing-field-initializers -Wno-attributes -fcompare-debug-second \
	-D_REENTRANT -DRSASIO_EXPORTS -D_USRDLL -DUNICODE
# -DWINE_DEVICES
WINDOWSINC := /usr/include/wine/wine/windows


HEADERS    := $(wildcard $(DIR_SRC)/*.h)
# DEF        := $(DIR_SRC)/exports.def
DEF        := RS_ASIO.dll.spec
BINARIES   := $(patsubst $(DIR_SRC)/%.cpp, $(DIR_BLD)/obj/%.o, $(wildcard $(DIR_SRC)/*.cpp))
#BINARIES   += $(patsubst $(DIR_SRC)/wine/%.cpp, $(DIR_BLD)/obj/wine/%.o, $(wildcard $(DIR_SRC)/wine/*.cpp))
BINARIES   += $(patsubst $(DIR_SRC)/%.S, $(DIR_BLD)/aobj/%.o, $(wildcard $(DIR_SRC)/*.S))

TARGET     := $(DIR_BLD)/$(LIB_NAME).so
WINETARGET := $(DIR_BLD)/$(LIB_NAME)

INSTALL_DIR := ${HOME}/.steam/debian-installation/steamapps/common/Rocksmith2014


# Make targets
all:
	make $(WINETARGET)
	make $(TARGET)

dir_guard=@mkdir -p $(@D)

$(DIR_BLD)/aobj/%.o: $(DIR_SRC)/%.S
	$(dir_guard)
	$(WINECXX) -m$(M) -c $< -o $@

$(DIR_BLD)/obj/%.o: $(DIR_SRC)/%.cpp $(HEADERS)
	$(dir_guard)
	$(WINECXX) $(DEFNS) $(CFLAGS) $(shell pkg-config --cflags $(PKG_CONFIG)) -I$(WINDOWSINC) -c $< -o $@

$(WINETARGET): $(BINARIES) $(DEF)
	$(WINEBUILD) -m$(M) --dll --fake-module -E $(DEF) $(BINARIES) -o $@

$(TARGET): $(BINARIES) $(RESOURCES) $(DEF)
	$(WINECXX) -shared -m$(M) $(BINARIES) $(RESOURCES) $(DEF) $(shell pkg-config --libs $(PKG_CONFIG)) $(LIBS) -o $@

install: $(TARGET) $(WINETARGET)
	cp -f $(TARGET) $(INSTALL_DIR)
	cp -f $(WINETARGET) $(INSTALL_DIR)
	chmod 555 $(INSTALL_DIR)/$(LIB_NAME)
	chmod 555 $(INSTALL_DIR)/$(LIB_NAME).so

clean:
	rm -rf $(DIR_BLD)

.PHONY: all install clean
