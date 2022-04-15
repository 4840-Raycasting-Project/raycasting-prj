ifneq (${KERNELRELEASE},)

CFLAGS = -Wall

USERLAND_OBJECTS = raycaster.o usbkeyboard.o 

# KERNELRELEASE defined: we are being compiled as part of the Kernel
        obj-m := column_decoder.o

else

# We are being compiled as a module: use the Kernel build system

	KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
        PWD := $(shell pwd)

default: module raycaster

module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

raycaster : $(USERLAND_OBJECTS)
	cc $(CFLAGS) -o raycaster $(USERLAND_OBJECTS) -lusb-1.0 -pthread -lm	

clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
	${RM} raycaster

TARFILES = Makefile README column_decoder.h column_decoder.c raycaster.c usbkeyboard.c
TARFILE = project.tar.gz
.PHONY : tar
tar : $(TARFILE)

$(TARFILE) : $(TARFILES)
	tar zcfC $(TARFILE) .. $(TARFILES:%=software/%)

raycaster.o : raycaster.c framebuffer.h usbkeyboard.h
usbkeyboard.o : usbkeyboard.c usbkeyboard.h

endif 
