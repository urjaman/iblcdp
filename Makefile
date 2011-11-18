# AVR-GCC Makefile
PROJECT=carlcdp
SOURCES=main.c uart.c console.c lib.c appdb.c commands.c hd44780.c lcd.c timer.c backlight.c buttons.c adc.c relay.c tui.c saver.c tui-other.c dallas.c tui-modules.c tui-calc.c batlvl.c
DEPS=Makefile
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=atmega168
#AVRBINDIR=~/avr-tools/bin/
AVRDUDECMD=avrdude -p m168 -c avrispmkII -P usb
CFLAGS=-mmcu=$(MMCU) -Os -fno-inline-small-functions -fno-tree-scev-cprop -frename-registers -g -Wall -W -pipe -mcall-prologues
 
$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex
 
$(PROJECT).out: $(SOURCES) timer-ll.o
	$(AVRBINDIR)$(CC) $(CFLAGS) -flto -fwhole-program -I./ -o $(PROJECT).out $(SOURCES) timer-ll.o

timer-ll.o: timer-ll.c timer.c main.h
	$(AVRBINDIR)$(CC) $(CFLAGS) -I./ -c -o timer-ll.o timer-ll.c
	

asm: $(SOURCES)
	$(AVRBINDIR)$(CC) -S $(CFLAGS) -I./ -o $(PROJECT).S $(SOURCES)

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xd $(PROJECT).out | less 


program: $(PROJECT).hex
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:w:$(PROJECT).hex

clean:
	-rm -f *.o
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).S

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r
