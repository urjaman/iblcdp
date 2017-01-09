# AVR-GCC Makefile

COMMON_SOURCES=console.c lib.c appdb.c commands.c timer.c time.c cron.c
COMMON_DEPS=main.h cron.h lib.h commands.h

PROJECT=iblcdm64c1
SOURCES=$(COMMON_SOURCES) main.c uart.c commands_m64c1.c avrpgm.c avrpgmif.c uart_tx.S slmaster.c
DEPS=$(COMMON_DEPS) Makefile uart.h avrpgm.h avrpgmif.h slmaster.h
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega64c1

PROJECT2=iblcdm1284
SOURCES2=$(COMMON_SOURCES) main2.c slslave.c sluart.c glcd.c stlcdnr.c rgbbl.c commands_m1284.c relay.c adc.c backlight.c buttons.c
DEPS2=$(COMMON_DEPS) Makefile slslave.h sluart.h stlcdnr.h stlcdhw.h rgbbl.h font-dyn-meta.c relay.h adc.h backlight.h buttons.h

#AVRBINDIR=~/avrtc-test/bin/

SERIAL_DEV ?= /dev/ttyUSB3

#AVRDUDECMD=avrdude -p $(AVRDUDEMCU) -c ftdi-ib -P usb -B 1kHz
AVRDUDECMD=$(AVRBINDIR)avrdude -c arduino -b 115200 -p m64c1 -P $(SERIAL_DEV)
AVRDUDECMD2=$(AVRBINDIR)avrdude -p m1284 -c butterfly -b 2000000 -P $(SERIAL_DEV)

DFLAGS=-DM64C1
CFLAGS=-mmcu=$(MMCU) -Os -g -Wall -W -pipe -mcall-prologues -std=gnu99 -Wno-main $(DFLAGS)

DFLAGS2=-DM1284
CFLAGS2=-mmcu=atmega1284 -Os -g -Wall -W -pipe -mcall-prologues -std=gnu99 -Wno-main $(DFLAGS2)

all: $(PROJECT).hex $(PROJECT2).bin size

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) timer-ll.o
	$(AVRBINDIR)$(CC) $(CFLAGS) -flto -fwhole-program -flto-partition=none -mrelax -I./ -o $(PROJECT).out  $(SOURCES) timer-ll.o -lc -lm

timer-ll.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS) -I. -c -o timer-ll.o timer-ll.c

$(PROJECT2).hex: $(PROJECT2).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT2).out $(PROJECT2).hex

$(PROJECT2).bin: $(PROJECT2).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O binary $(PROJECT2).out $(PROJECT2).bin

$(PROJECT2).out: $(SOURCES2) timer-ll2.o adc-ll.o
	$(AVRBINDIR)$(CC) $(CFLAGS2) -flto -fwhole-program -flto-partition=none -mrelax -I./ -o $(PROJECT2).out  $(SOURCES2) timer-ll2.o adc-ll.o -lc -lm

timer-ll2.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS2) -I. -c -o timer-ll2.o timer-ll.c
adc-ll.o: adc-ll.c adc.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS2) -I./ -c -o adc-ll.o adc-ll.c


asm: $(SOURCES)
	$(AVRBINDIR)$(CC) -S $(CFLAGS) -I. -o $(PROJECT).S $(SOURCES)

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT).out | less

objdump2: $(PROJECT2).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT2).out | less


program: $(PROJECT).hex
	$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

program2: $(PROJECT2).bin serialprogrammer
	./serialprogrammer $(PROJECT2).bin $(SERIAL_DEV)

size: $(PROJECT).out $(PROJECT2).out
	$(AVRBINDIR)avr-size $(PROJECT).out
	$(AVRBINDIR)avr-size $(PROJECT2).out

clean:
	-rm -f *.o
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).S

serialprogrammer: serialprogrammer.c
	gcc -W -Wall -Os -o serialprogrammer serialprogrammer.c

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r
