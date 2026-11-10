#include "encoder.h"
#include "midi.h"
#include "uart.h"
#include <avr/io.h>
#include <util/delay.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))

void timer_init(void) {
	/* Run Timer0 at F_CPU/1024 Hz */
	TCCR0B = 1 << CS02 | 1 << CS00;
}

struct {
	uint8_t channel;
} tx81z;

void tx81zparameter(int dir) {
	uint8_t i, msg[7] = {0xf0, 0x43, 0, 0x13, 0, 0, 0xf7};

	if (!dir)
		return;

	msg[2] = 0x10 | (tx81z.channel & 0xf);
	msg[4] = dir > 0 ? 72 : 71;
	msg[5] = 0x7f;
	for (i = 0; i < nelem(msg); i++)
		uart_tx(msg[i]);
	msg[5] = 0;
	for (i = 0; i < nelem(msg); i++)
		uart_tx(msg[i]);
}

int main(void) {
	uint8_t time = 0;
	struct encoder encoder;
	uart_init();
	timer_init();
	encoderinit(&encoder);

	while (1) {
		uint8_t delta = TCNT0 - time;
		time += delta;
		tx81zparameter(encoderupdate(&encoder));
	}
}
