PROGRAM    = optiboot

MCU_TARGET = atmega64c1
LDSECTIONS = -Wl,--section-start=.text=0xfc00 -Wl,--section-start=.version=0xfffc
OBJ        = $(PROGRAM).o
OPTIMIZE   = -Os -fno-split-wide-types -mrelax
LIBS       = -lc
CC        = $(AVRBINDIR)avr-gcc
CFLAGS     = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=16000000L
LDFLAGS    = $(LDSECTIONS) -Wl,--relax -nostartfiles -nostdlib
OBJCOPY    = $(AVRBINDIR)avr-objcopy
OBJDUMP    = $(AVRBINDIR)avr-objdump
SIZE       = $(SVRBINDIR)avr-size

all: $(PROGRAM).hex $(PROGRAM).elf

%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	$(SIZE) $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -j .version --set-section-flags .version=alloc,load -O ihex $< $@

clean:
	-rm *.o *.elf *.hex

flash: $(PROGRAM).hex
	$(AVRBINDIR)avrdude -c ftdi-ib -B 1kHz -p m64c1 -P usb -U efuse:w:0b11111010:m -U hfuse:w:0b11010110:m -U lfuse:w:0b11011111:m -U flash:w:optiboot.hex

flash-os: $(PROGRAM).hex
	$(AVRBINDIR)avrdude -c ftdi-ib -B 50Hz -p m64c1 -P usb -U flash:w:optiboot.hex

