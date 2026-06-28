/* MIDI clock merge box */
#include "pinin.h"
#include "uart.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#define nelem(x) (int)(sizeof(x) / sizeof(*(x)))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define VOL(type, addr) *(volatile type *)(addr)
struct queue {
	int buf[16];
	uint8_t head, tail;
};
void push(struct queue *q, int x) {
	VOL(int, q->buf + (q->head % nelem(q->buf))) = x;
	VOL(uint8_t, &q->head) = q->head + 1;
}
int pop(struct queue *q, int *x) {
	int notempty = VOL(uint8_t, &q->head) != q->tail;
	*x = VOL(int, q->buf + (q->tail % nelem(q->buf)));
	q->tail += notempty;
	return notempty;
}
struct queue clock, midi, compare;
/* MIDI clock is 24 PPQN. the formula below calculates the number of prescaled
 * timer ticks that corresponds to 1 MIDI clock pulse for the given tempo in BPM
 */
#define fTimer1 (F_CPU / 64)
int bpmticks(int n) { return (fTimer1 / 2 * 5) / n; }
#define MIDI_CLOCK 0xf8
ISR(TIMER1_COMPA_vect) {
	int newcompare;
	while (pop(&compare, &newcompare))
		OCR1A = newcompare;
	push(&clock, MIDI_CLOCK);
}
int main(void) {
	struct encoder encoder = ENCODER(D, 0, D, 1);
	uint8_t time = 0;
	int bpm = 120;
	uart_init();
	TCCR0B = 1 << CS02 | 1 << CS00; /* Timer0 prescaler 1024 */
	encoder_init(&encoder);
	TCCR1B = 1 << CS11 | 1 << CS10; /* Timer1 prescaler 64 */
	TCCR1B |= 1 << WGM12;		/* CTC mode */
	OCR1A = bpmticks(bpm);
	TIMSK1 = 1 << OCIE1A;
	sei();
	while (1) {
		uint8_t delta = TCNT0 - time, in;
		int dir = encoder_debounce(&encoder, delta);
		time += delta;
		if (dir) {
			bpm += dir;
			bpm = max(min(bpm, 300), 30);
			push(&compare, bpmticks(bpm));
		}
		if (uart_rx(&in) && in != MIDI_CLOCK)
			push(&midi, in);
		if (uart_tx_ready()) {
			int out;
			if (pop(&clock, &out) || pop(&midi, &out))
				uart_tx(out);
		}
	}
}
