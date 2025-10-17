PRG            = mmangler
OBJ            = mmangler.o midi.o pin.o

CC = avr-gcc
override CPPFLAGS = -Imidiparser -DF_CPU=16000000L
override CFLAGS = -g -Wall -Wextra -Werror -Os -flto -std=gnu89 -pedantic
override TARGET_ARCH = -mmcu=atmega32u4
override LDFLAGS = -Wl,-Map,$(PRG).map
override LDLIBS = 

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

all: $(PRG).bin $(PRG).lst

midi.o: midiparser/midi.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $^

$(PRG): midi.o

clean:
	rm -rf -- $(OBJ) $(PRG) $(PRG).lst $(PRG).map $(PRG).bin

%.lst: %
	$(OBJDUMP) -h -S $< > $@

%.bin: %
	$(OBJCOPY) -j .text -j .data -O binary $< $@

format:
	clang-format -i -style=file *.c *.h

AVRDUDE_PORT ?= /dev/tty.usbmodem1112401
upload: all
	ls /dev/|grep usbmodem
	stty -f $(AVRDUDE_PORT) 1200
	avrdude -D -p m32u4 -b 57600 -c avr109 -P $(AVRDUDE_PORT) -U flash:w:$(PRG).hex:i 
