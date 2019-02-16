TARGET = mediaengine
OBJS = me_stub.o main.o sceMeCore_driver.o sceSysreg_driver371.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = $(TARGET).exp

USE_KERNEL_LIBC=1
USE_KERNEL_LIBS=1

LIBDIR =
LDFLAGS =-nostartfiles
LIBS = -lpspkernel


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

#all:
#	psp-build-exports -s $(PRX_EXPORTS)
#	@cp $(TARGET).prx ../../
#	@cp $(TARGET).S ../
#	@rm -f $(TARGET).prx
#	@rm -f $(TARGET).S
