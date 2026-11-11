/* MIDI sequencer */
#include "encoder.h"
#include "midi.h"
#include "uart.h"
#define nelem(x) (sizeof(x) / sizeof(*(x)))
int main(void) {
	uint8_t sequence[128], nsequence = 0, isequence, loading = 0,
			       midichannel = 0, gate = 0, b;
	struct encoder encoder;
	midi_parser parser = {0};
	uart_init();
	encoderinit(&encoder);
loop:
	if (encoderupdate(&encoder)) {
		if (!nsequence) {
			loading = 1;
		} else {
			loading = 0;
			isequence = 0;
			uart_tx(MIDI_NOTE_ON | midichannel);
		}
	}
	if (uart_rx(&b)) {
		midi_message msg = midi_read(&parser, b);
		if (!nsequence || loading)
			uart_tx(b);
		if (loading) {
			if (!nsequence && (msg.status & 0xf0) == MIDI_NOTE_ON)
				midichannel = msg.status & 0x0f;
			if (msg.status == (MIDI_NOTE_ON | midichannel) &&
			    msg.data[1]) {
				sequence[nsequence++] = msg.data[0];
				loading = nsequence < nelem(sequence);
			}
		} else if (nsequence && (msg.status & 0x0f) == midichannel) {
			uint8_t status = msg.status & 0xf0;
			if (gate && (status == MIDI_NOTE_ON ||
				     status == MIDI_NOTE_OFF)) {
				gate = 0;
				uart_tx(sequence[isequence]);
				uart_tx(0);
				if (++isequence >= nsequence)
					isequence = 0;
			}
			if (status == MIDI_NOTE_ON && msg.data[1]) {
				uart_tx(
				    (sequence[isequence] + msg.data[0] - 60) %
				    128);
				uart_tx(msg.data[1]);
				gate = 1;
			}
		}
	}
	goto loop;
}
