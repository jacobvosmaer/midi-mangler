#include "encoder.h"
void encoderinit(struct encoder *enc) {
	struct encoder zero = {0};
	*enc = zero;
	DDRD &= ~3;
	PORTD |= 3;
}
int8_t encoderupdate(struct encoder *enc) {
	/* lookup table from
	 * https://chome.nerpa.tech/mcu/reading-rotary-encoder-on-arduino/ */
	int8_t ret = 0,
	       lookup[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
	enc->history = (enc->history << 2) | (PIND & 3);
	enc->pulses -= lookup[enc->history & 0xf];
	if (enc->pulses >= 2) {
		enc->pulses = 0;
		ret = 1;
	} else if (enc->pulses <= -2) {
		enc->pulses = 0;
		ret = -1;
	}
	return ret;
}
