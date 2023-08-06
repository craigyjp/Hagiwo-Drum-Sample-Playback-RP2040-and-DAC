# Hagiwo-8-Drum-Sample-playback-with-MIDI and DAC

Modified version of the Hagiwo $8 Drum Sample playback module.

https://note.com/solder_state/n/n950ed088c3cb

I found the 10 bit sample playback and the sample.h format difficult to understand, so I rewrote the code to use a 16 bit DAC, but the ESP32-C3 does not talk to i2s DACs very well so I ported the code to RP2040, in particular the YD-RP2040 to take advantage of the large memory capacity and low cost.

Using the YD-RP2040 16Mb version it should be possible to increase the amount of drum samples to nearly 200 or increase the sample length for longer samples and reduce the amount around 100 samples per drum.

Reads MIDI CC messages and program changes on channel 1 for control over the sample number, tuning and filter.

* CC 08 controls the tuning of the drum in 128 steps.
* CC 09 controls the volume, CC07 was a problem for some reason.
* CC 10 controls the panning let to right.
* PGM change 0-47 select the sample to playback and triggers the sound.
* MIDI note on to trigger drum has to be fixed per unit.
* LED indicator for 48khz sample speed

I moved the trigger input to another pin.

