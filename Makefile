# AVR-GCC Makefile
PROJECT=iblcdm64c1
SOURCES=main.c uart.c console.c lib.c appdb.c commands.c timer.c time.c cron.c avrpgm.c avrpgmif.c uart_tx.S
DEPS=Makefile main.h cron.h uart.h lib.h avrpgm.h avrpgmif.h
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega64c1

AVRBINDIR=~/avrtc-test/bin/

#AVRDUDECMD=avrdude -p $(AVRDUDEMCU) -c ftdi-ib -P usb -B 1kHz
AVRDUDECMD=$(AVRBINDIR)avrdude -c arduino -b 115200 -p m64c1 -P /dev/ttyUSB0

DFLAGS=
CFLAGS=-mmcu=$(MMCU) -Os -g -Wall -W -pipe -mcall-prologues -std=gnu99 -Wno-main $(DFLAGS)

all: $(PROJECT).hex size

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) timer-ll.o
	$(AVRBINDIR)$(CC) $(CFLAGS) -flto -fwhole-program -flto-partition=none -mrelax -I./ -o $(PROJECT).out  $(SOURCES) timer-ll.o -lc -lm

timer-ll.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS) -I. -c -o timer-ll.o timer-ll.c

asm: $(SOURCES)
	$(AVRBINDIR)$(CC) -S $(CFLAGS) -I. -o $(PROJECT).S $(SOURCES)

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT).out | less


program: $(PROJECT).hex
	$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

size: $(PROJECT).out
	$(AVRBINDIR)avr-size $(PROJECT).out

clean:
	-rm -f *.o
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).S

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r
