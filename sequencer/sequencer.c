/* MIDI sequencer */
#include "encoder.h"
#include "led.h"
#include "midi.h"
#include "uart.h"
#define nelem(x) (sizeof(x) / sizeof(*(x)))
int main(void) {
	uint8_t sequence[128], out[nelem(sequence)],
	    n = 0, i, loading = 0, midichannel = 0, gate = 0, b;
	struct encoder encoder;
	midi_parser parser = {0};
	uart_init();
	encoderinit(&encoder);
	ledinit();
loop:
	if (encoderupdate(&encoder)) {
		if (!n) {
			loading = 1;
			ledyellow(1);
		} else {
			loading = 0;
			i = 0;
			ledyellow(0);
			ledgreen(1);
			uart_tx(MIDI_NOTE_ON | midichannel);
		}
	}
	if (uart_rx(&b)) {
		midi_message msg = midi_read(&parser, b);
		if (!n || loading)
			uart_tx(b);
		if (loading) {
			if (!n && (msg.status & 0xf0) == MIDI_NOTE_ON)
				midichannel = msg.status & 0x0f;
			if (msg.status == (MIDI_NOTE_ON | midichannel) &&
			    msg.data[1]) {
				sequence[n++] = msg.data[0] - 60;
				loading = n < nelem(sequence);
			}
		} else if (n && (msg.status & 0x0f) == midichannel) {
			uint8_t status = msg.status & 0xf0;
			if (gate && (status == MIDI_NOTE_ON ||
				     status == MIDI_NOTE_OFF)) {
				gate = 0;
				uart_tx(out[i]);
				uart_tx(0);
				if (++i >= n)
					i = 0;
			}
			if (status == MIDI_NOTE_ON && msg.data[1]) {
				out[i] = (sequence[i] + msg.data[0]) % 128;
				uart_tx(out[i]);
				uart_tx(msg.data[1]);
				gate = 1;
			}
		}
	}
	goto loop;
}
