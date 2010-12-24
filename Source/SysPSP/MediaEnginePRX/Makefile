TARGET = mediaengine
OBJS = me_stub.o main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall


CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBC=1
USE_KERNEL_LIBS=1

LIBDIR = ../../../SDK/lib
LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -lpspkernel

PSP_FW_VERSION=500


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

EXTRA_TARGETS = MediaEngine.S

MediaEngine.S : $(PRX_EXPORTS)
	psp-build-exports --build-stubs $(PRX_EXPORTS)

all: $(FINAL_TARGET) MediaEngine.S
	cp $(FINAL_TARGET) "../../.."
