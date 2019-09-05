# Name: Makefile
# Project: aquarium
# Author: Baranovskiy Konstantin
# Creation Date: 2015-12-28
# Tabsize: 4
# Copyright: (c) 2015-2019 Baranovskiy Konstantin
# License: GNU GPL v3 (see License.txt)

DEVICE  = atmega8
F_CPU   = 8000000
FUSE_L  = 0x24
FUSE_H  = 0x97
AVRDUDE = avrdude -c pickit2 -p $(DEVICE) -v

SOURCES = main.c aquarium.c display.c ds18b20.c ds1302.c datetime.c uart.c crc8.c adc.c pwm.c
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
#        | +------------------ WDTON       ( WDT is initially enabled )
#        +-------------------- RSTDISBL    ( external reset enabled )
#
###############################################################################

# symbolic targets:
help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make flash ..... to build flash.hex"
	@echo "make eeprom .... to build eeprom.hex"
	@echo "make program ... to flash the firmware"
	@echo "make fuse ...... to flash the fuses"
	@echo "make setup ..... to write the configuration data to eeprom"
	@echo "make clean ..... to delete objects"
	@echo "make poweron ... to turn power on"
	@echo "make powerooff . to turn power off"

poweron:
	pk2cmd -pPIC16F73 -A5 -R -T

poweroff:
	pk2cmd -pPIC16F73 -A5 -R

flash: flash.hex

eeprom: eeprom.hex

# rule for uploading firmware:
program: flash.hex
	$(AVRDUDE) -U flash:w:$<:i

# rule for programming fuse bits:
fuse:
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m

# rule for uploading configuration data:
setup: eeprom.hex
	$(AVRDUDE) -U eeprom:w:$<:i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f *.o main.elf

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

main.elf: $(SOURCES:.c=.o)
	$(COMPILE) -o main.elf $(SOURCES:.c=.o)

flash.hex: main.elf
	rm -f flash.hex
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size flash.hex

eeprom.hex: main.elf
	rm -f eeprom.hex
	avr-objcopy -j .eeprom -O ihex --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 --no-change-warnings $< $@
	avr-size eeprom.hex

