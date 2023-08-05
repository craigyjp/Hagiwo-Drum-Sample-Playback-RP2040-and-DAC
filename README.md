# Hagiwo-8-Drum-Sample-playback-with-MIDI and DAC

Modified version of the Hagiwo $8 Drum Sample playback module.

https://note.com/solder_state/n/n950ed088c3cb

Converted to RP2040 as the ESP32-C3 does not work well with i2s DACs

Reads MIDI CC messages and program changes on channel 1 for control over the sample number, tuning and filter.

* CC 08 controls the tuning of the drum in 128 steps.
* CC 09 controls the volume, CC07 was a problem for some reason.
* CC 10 controls the panning let to right.
* PGM change 0-47 select the sample to playback and triggers the sound.
* MIDI note on to trigger drum has to be fixed per unit.
* LED indicator for 48khz sample speed

I moved the trigger input to another pin.

