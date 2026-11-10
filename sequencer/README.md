# MIDI step sequencer with transposition

On power-up, this firmware echoes MIDI IN to OUT. Turn the knob to go into "load" mode. Up to 128 notes are stored in the sequencer as a monophonic sequence. The first note after entering load mode determines the MIDI channel used by the sequencer.

Turn the knob a second time to enter "play" mode. Each incoming note triggers a step from the sequence, with relative pitch, gate length and velocity determined by the incoming note. Send MIDI note 60 to get the original sequence values back.

Another turn of the knob resets the sequence to its starting position. To erase the sequence, reboot the machine.
