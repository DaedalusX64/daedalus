TARGET = dvemgr
OBJS = main.o exports.o sceGe_driver.o

CFLAGS = -Os -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division -DF_sceGe_driver_0013
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

INCDIR = ../../../SDK/include
LIBDIR = ../../../SDK/lib
BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -lpspdve_driver -lpsphprm_driver -lpspge_driver -lpspimpose_driver

PSP_FW_VERSION=500

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

EXTRA_TARGETS = pspDveManager.S pspDveManager_driver.S

pspDveManager.S : $(PRX_EXPORTS)
	psp-build-exports --build-stubs $(PRX_EXPORTS)

pspDveManager_driver.S : $(PRX_EXPORTS)
	psp-build-exports --build-stubs $(PRX_EXPORTS)

all: $(FINAL_TARGET) pspDveManager.S pspDveManager_driver.S
	cp $(FINAL_TARGET) "../../.."
