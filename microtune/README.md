# Micro tuning and velocity mapper

This firmware bundles several functions intended for use between a MIDI keyboard and a sound source.

When the firmware boots, the system goes into "echo" mode, passing through all MIDI data. Press the encoder to cycle through the modes. If you get stuck just reset the MIDI box, all modes get reset to their default state.

## Velocity mapper mode

Turn the encoder to select a MIDI note. Every note sent to the input will come out as the chosen note, with velocity determined by the incoming note number. This makes it easier to play the same note with different velocity values. This can be useful for drum parts.

All other MIDI messages besides note on/off are dropped.

## Micro tuning mode

Each note that comes in goes out, preceded by a pitch bend message. Turn the encoder to change the pitch bend message associated with the last note that came in. The effect is that you can retune individual notes. This works best with monophonic sound sources. Polyphonic sound sources behave unpredictably.

All other MIDI messages besides note on/off are dropped.

## Compressed tuning mode

This is the opposite of "stretch tuning": there is less space between the keys. Assuming the pitch bend range of the sound source you're controlling is 2 semitones, this will evenly spread two semitones over one octave on the input keyboard. Turn the encoder to shift the keyboard.

All other MIDI messages besides note on/off are dropped.

