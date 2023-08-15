#include "midi.h"
#include "pin.h"
#include <avr/io.h>
#include <util/delay.h>

void uart_init(void) {
	enum { midi_baud = 31250, ubrr = (F_CPU / 16 / midi_baud) - 1 };
	UBRR1H = ubrr >> 8;
	UBRR1L = ubrr & 0xff;
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
}

void uart_tx(uint8_t c) {
	while (!(UCSR1A & (1 << UDRE1)))
		;
	UDR1 = c;
}

uint8_t uart_read(uint8_t *c) {
	uint8_t status;

	do
		status = UCSR1A;
	while (!(status & _BV(RXC1)));
	*c = UDR1;
	return !(status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1)));
}

pin yellow = {&PORTB, &DDRB, PORTB0};

int main(void) {
	uint8_t b;
	midi_parser parser = {0};
	midi_message msg = {0};

	pin_init(&yellow);

	uart_init();

	for (;;) {
		if (uart_read(&b) && midi_read(&parser, &msg, b) &&
		    (msg.status == (MIDI_NOTE_ON | 2)) && !msg.data[1])
			pin_set(&yellow, 0);
	}

	return 0;
}
