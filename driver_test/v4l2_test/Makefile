# ----------------------------------------------------------------------------
# Makefile for building tapp
#
# Copyright 2010 FriendlyARM (http://www.arm9.net/)
#

ifndef DESTDIR
DESTDIR			   ?= /home/lu/tmp/linux-3.10
endif

CFLAGS				= -Wall -O2
CC					= arm-linux-gcc
INSTALL				= install

TARGET				= v4l2_test


all: $(TARGET)

led: lcd_test.c
	$(CC) $(CFLAGS) $< -o $@


install: $(TARGET)
	$(INSTALL) $^ $(DESTDIR)/

clean distclean:
	rm -rf *.o $(TARGET)


# ----------------------------------------------------------------------------

.PHONY: $(PHONY) install clean distclean

# End of file
# vim: syntax=make

