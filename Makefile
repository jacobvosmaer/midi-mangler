PRG = mmangler
OBJ = midi.o
VPATH = midiparser
CC = avr-gcc
OBJCOPY = avr-objcopy
CPPFLAGS = -Imidiparser -DF_CPU=16000000L
CFLAGS = -std=gnu89 -Os -flto -Wall -Wextra -Werror -pedantic
TARGET_ARCH = -mmcu=atmega32u4

all: $(PRG)

$(PRG): $(OBJ)

clean:
	rm -rf -- $(OBJ) $(PRG) $(PRG).hex

%.hex: %
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

format:
	clang-format -i -style=file *.c *.h

AVRDUDE_PORT ?= /dev/tty.usbmodem1112401
upload: $(PRG).hex
	ls /dev/|grep usbmodem
	stty -f $(AVRDUDE_PORT) 1200
	avrdude -D -p m32u4 -b 57600 -c avr109 -P $(AVRDUDE_PORT) -U flash:w:$(PRG).hex:i 
