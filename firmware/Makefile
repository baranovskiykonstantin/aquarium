# Name: Makefile
# Project: aquarium
# Author: Baranovskiy Konstantin
# Creation Date: 2015-12-28
# Tabsize: 4
# Copyright: (c) 2015 Baranovskiy Konstantin
# License: GNU GPL v3 (see License.txt)
# This Revision: 1

DEVICE  = atmega8
F_CPU   = 8000000
FUSE_L  = 0x24
FUSE_H  = 0x97
#AVRDUDE = avrdude -c usbasp -p $(DEVICE) -v
#AVRDUDE = avrdude -c stk500v2 -P avrdoper -p $(DEVICE) -v
AVRDUDE = avrdude -c pickit2 -p $(DEVICE) -v

SOURCES = main.c display.c ds18b20.c ds1302.c datetime.c uart.c crc8.c adc.c pwm.c
CFLAGS  = -I. -DDEBUG_LEVEL=0
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

################################## ATmega8a ###################################
#
# ATmega8A FUSE_L (Fuse low byte):
# 0x24 = 0 0 1 0   0 1 0 0
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0  ( internal RC oscillator 8 MHz )
#        | |  +--------------- SUT 1..0    ( slowly rising power )
#        | +------------------ BODEN       ( enabled )
#        +-------------------- BODLEVEL    ( 4.0V )
#
# ATmega8A FUSE_H (Fuse high byte):
# 0x97 = 1 0 0 1   0 1 1 1
#        ^ ^ ^ ^   ^ \ / |
#        | | | |   |  |  +---- BOOTRST     ( reset vector )
#        | | | |   |  +------- BOOTSZ 1..0 ( min size )
#        | | | |   + --------- EESAVE      ( EEPROM reserved )
#        | | | +-------------- CKOPT       ( oscillator options disabled )
#        | | +---------------- SPIEN       ( SPI programming enabled )
#        | +------------------ WDTON       ( WDT disable )
#        +-------------------- RSTDISBL    ( external reset enabled )
#
###############################################################################

# symbolic targets:
help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ....... to build main.hex"
	@echo "make program ... to flash fuses and firmware"
	@echo "make fuse ...... to flash the fuses"
	@echo "make flash ..... to flash the firmware"
	@echo "make clean ..... to delete objects and hex file"
	@echo "make power ..... to turn power on"

power:
	pk2cmd -pPIC16F73 -A5 -R -T

hex: main.hex

program: flash fuse

# rule for programming fuse bits:
fuse:
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m

# rule for uploading firmware:
flash: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f *.o main.elf

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

main.elf: $(SOURCES:.c=.o)
	$(COMPILE) -o main.elf $(SOURCES:.c=.o)

main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex

