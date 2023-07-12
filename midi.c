#include "midi.h"

uint8_t midi_read(midi_parser *parser, midi_message *msg, uint8_t b) {
	enum { PREV_FLAG = 0x80 };
	msg->status = 0;

	if (b >= 0xf8) {
		msg->status = b;
	} else if (b >= 0xf4) {
		msg->status = b;
		parser->status = 0;
	} else if (b >= 0x80) {
		parser->status = b;
		parser->prev = 0;
	} else if ((parser->status >= 0xc0 && parser->status < 0xe0) ||
		   (parser->status >= 0xf0)) {
		msg->status = parser->status;
		msg->data[0] = b;
	} else if (parser->status && parser->prev) {
		msg->status = parser->status;
		msg->data[0] = parser->prev & ~PREV_FLAG;
		msg->data[1] = b;
		parser->prev = 0;
	} else {
		parser->prev = b | PREV_FLAG;
	}

	return !!msg->status;
}
