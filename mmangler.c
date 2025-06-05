#include "midi.h"
#include "pin.h"
#include <avr/io.h>
#include <util/delay.h>

#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))

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

pin yellow = {&PORTB, &DDRB, PORTB0}, green = {&PORTD, &DDRD, PORTB5};

struct {
	volatile uint8_t *const port, *const ddr, *const pin;
	uint8_t bit[2], debounce[2], elapsed;
} encoder = {&PORTD, &DDRD, &PIND, {PORTD0, PORTD1}, {0}, 0};

void encoder_init(void) {
	uint8_t mask = (1 << encoder.bit[0]) | (1 << encoder.bit[1]);
	*encoder.ddr &= ~mask;
	*encoder.port |= mask;
}

int encoder_debounce(uint8_t delta) {
	int i, a, b;

	encoder.elapsed += delta;
	if (encoder.elapsed < 16)
		return 0;
	encoder.elapsed = 0;

	for (i = 0; i < nelem(encoder.debounce); i++)
		encoder.debounce[i] = (encoder.debounce[i] << 1) |
				      !!(*encoder.pin & (1 << encoder.bit[i]));

	a = encoder.debounce[0] & 3;
	b = encoder.debounce[1] & 3;
	return (a == 1 && b == 0) - (a == 2 && b == 0);
}

void timer_init(void) {
	/* Run Timer0 at F_CPU/1024 Hz */
	TCCR0B = 1 << CS02 | 1 << CS00;
}

int notes[128];

#define PITCHMAX 6912

uint8_t sysex[274] = {0xf0, 0x43, 0,   0x7e, 0x02, 0x0a, 'L', 'M',
		      ' ',  ' ',  'M', 'C',  'R',  'T',	 'E', '1'};

void retune(int octave) {
	int i;
	uint8_t checksum;
	float step = (float)octave / 12.0;

	for (i = 0; i < nelem(notes); i++) {
		notes[i] = (float)i * step;
		while (notes[i] > PITCHMAX)
			notes[i] -= octave;
	}

	/* sysex[2] = deviceno - 1; */
	for (i = 0; i < 128; i++) {
		sysex[16 + 2 * i] = notes[i] >> 6;
		sysex[16 + 2 * i + 1] = notes[i] & ((1 << 6) - 1);
	}
	for (i = 6, checksum = 0; i < nelem(sysex) - 2; i++)
		checksum += sysex[i];
	sysex[nelem(sysex) - 2] = (0x80 - checksum) & 0x7f;
	sysex[nelem(sysex) - 1] = 0xf7;

	for (i = 0; i < nelem(sysex); i++)
		uart_tx(sysex[i]);
}

int main(void) {
	int octave = 768;
	uint8_t time = 0;
	uart_init();
	timer_init();
	encoder_init();
	pin_init(&yellow);
	pin_init(&green);

	retune(octave);
	while (1) {
		uint8_t delta = TCNT0 - time;
		int dir = encoder_debounce(delta);
		time += delta;
		if (dir) {
			octave += 3 * dir;
			retune(octave);
		}
	}
}
