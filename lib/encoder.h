#ifndef ENCODER_H
#define ENCODER_H
#include <avr/io.h>
struct encoder {
	uint8_t history;
	int8_t pulses;
};
void encoderinit(struct encoder *);
int8_t encoderupdate(struct encoder *);
#endif
