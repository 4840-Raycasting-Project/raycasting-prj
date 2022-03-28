CFLAGS = -Wall

OBJECTS = raycaster.o framebuffer.o usbkeyboard.o

TARFILES = Makefile raycaster.c \
	framebuffer.h framebuffer.c \
	usbkeyboard.h usbkeyboard.c

raycaster : $(OBJECTS)
	cc $(CFLAGS) -o raycaster $(OBJECTS) -lusb-1.0 -pthread -lm

raycaster.tar.gz : $(TARFILES)
	rm -rf raycaster
	mkdir raycaster
	ln $(TARFILES) raycaster
	tar zcf raycaster.tar.gz raycaster
	rm -rf raycaster

raycaster.o : raycaster.c framebuffer.h usbkeyboard.h
framebuffer.o : framebuffer.c framebuffer.h
usbkeyboard.o : usbkeyboard.c usbkeyboard.h

.PHONY : clean
clean :
	rm -rf *.o raycaster
