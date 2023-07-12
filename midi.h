#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>

/* Channel status values */
enum {
	MIDI_NOTE_OFF = 0x8,
	MIDI_NOTE_ON = 0x9,
	MIDI_AFTERTOUCH = 0xa,
	MIDI_CONTROL_CHANGE = 0xb,
	MIDI_PROGRAM_CHANGE = 0xc,
	MIDI_CHANNEL_PRESSURE = 0xd,
	MIDI_PITCH_BEND = 0xe
};

/* Global status values */
enum { MIDI_SYSEX = 0xf0, MIDI_SYSEX_END = 0xf7 };

/* A MIDI message has a non-zero status byte and 0, 1 or 2 data bytes. The */
/* number of data bytes is determined by the status byte. */
typedef struct midi_message {
	uint8_t status;
	uint8_t data[2];
} midi_message;

typedef struct midi_parser {
	uint8_t status;
	uint8_t prev;
} midi_parser;

uint8_t midi_read(midi_parser *parser, midi_message *msg, uint8_t b);

#endif
