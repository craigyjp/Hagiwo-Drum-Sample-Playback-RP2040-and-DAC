#include "sample.h"  //sample file
#include <EEPROM.h>
#include <MIDI.h>

#include <I2S.h>

// Create the I2S port using a PIO state machine
I2S i2s(OUTPUT);

// GPIO pin numbers
#define pBCLK 20
#define pWS (pBCLK + 1)
#define pDOUT 22

#define TRIGGER 5
#define MIDI_CHANNEL 1

const int sampleRate = 48000;  // minimum for UDA1334A

float left_vol;
float right_vol;
volatile bool samplePlaying = false;  // add this to your global variables

float i;  //sample play progress
int freq = 48000;
int midifreq = 23;
float volume = 1.00;
float panl = 0.50;
float panr = 0.50;
float midivolume = 1.00;
float midipanl = 0.50;
float midipanr = 0.50;
bool trig1, old_trig1, done_trig1;
int sound_out;          //sound out PWM rate
byte sample_no = 0;     //select sample number
long timer = 0;         //timer count for eeprom write
bool eeprom_write = 0;  //0=no write,1=write

//-------------------------timer interrupt for sound----------------------------------
//hw_timer_t *timer0 = NULL;
//portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;

const uint8_t *sample_array[] = { smpl0, smpl1, smpl2, smpl3, smpl4 };
const int speed_array[] = { 8000, 10000, 12000, 14000, 16000, 18000, 20000, 22000, 24000, 26000, 28000, 30000, 32000, 34000, 36000, 38000, 40000, 42000, 44100, 46000, 48000, 50000, 52000, 54000, 56000, 58000, 60000, 62000, 64000, 66000, 68000, 70000, 72000, 74000, 76000, 78000, 80000, 82000, 84000, 86000, 88000, 90000, 92000, 94000, 96000 };


MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);


void setup() {

  Serial.begin(31250);
  i2s.setBCLK(pBCLK);
  i2s.setDATA(pDOUT);
  i2s.setBitsPerSample(16);

  // start I2S at the sample rate with 16-bits per sample
  if (!i2s.begin(sampleRate)) {
    Serial.println("Failed to initialize I2S!");
    while (1)
      ;  // do nothing
  }


  EEPROM.begin(1);           //1byte memory space
  EEPROM.get(0, sample_no);  //callback saved sample number

  if (sample_no >= 48) {  //countermeasure rotary encoder error
    sample_no = 0;
  }

  pinMode(TRIGGER, INPUT);  //trigger in
  timer = millis();         //for eeprom write

  MIDI.begin(MIDI_CHANNEL);
  delay(300);
}

void eeprom_update() {
  EEPROM.put(0, sample_no);
  EEPROM.commit();
}

// update playSample function
void playSample() {
  if (!samplePlaying) {
    return;
  }

  const int chunk_size = 512;
  int16_t *i2s_data = new int16_t[chunk_size * 2];
  static int i = 0;
  int chunk_index = 0;

  if (done_trig1 == 1) {
    i = 0;
    done_trig1 = 0;
  }

  for (chunk_index = 0; chunk_index < chunk_size && i < 28800; chunk_index++, i++) {
    uint16_t sample_data = (((pgm_read_byte(&(sample_array[sample_no][i * 2]))) | (pgm_read_byte(&(sample_array[sample_no][i * 2 + 1]))) << 8));

    // Convert sample_data to float, apply volume and convert back to int16_t
    int16_t volume_adjusted_sample = (int16_t)((float)sample_data * 1);

    // Implement simplistic stereo panning
    i2s_data[chunk_index * 2] = volume_adjusted_sample * left_vol;       // Left channel
    i2s_data[chunk_index * 2 + 1] = volume_adjusted_sample * right_vol;  // Right channel
  }

  for (chunk_index = 0; chunk_index < chunk_size; chunk_index++) {
    i2s.write(i2s_data[chunk_index * 2]);      // write left channel data
    i2s.write(i2s_data[chunk_index * 2 + 1]);  // write right channel data
  }

  if (i >= 28800 && done_trig1 == 0) {
    i = 0;
    samplePlaying = false;  // stop playing the sample once it's finished
  }

  delayMicroseconds(10);

  delete[] i2s_data;
}

void loop() {

  playSample();

  int type, noteMsg, velocity, channel, d1, d2;
  if (MIDI.read(MIDI_CHANNEL)) {
    byte type = MIDI.getType();
    switch (type) {

      case midi::NoteOn:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
        switch (d1) {
          case 36:  // 36 for bottom C Bass
            if (d2 != 0) {
              done_trig1 = 1;
              i = 0;
              samplePlaying = true;
            }
            break;
        }
        break;

      case midi::ControlChange:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
        switch (d1) {
          case 8:
            midifreq = map(d2, 0, 127, 0, 44);  // range from 8kHz to 48kHz for simplicity, adjust as needed
            freq = speed_array[midifreq];
            i2s.setFrequency(freq);
            break;
          case 9:                     // CC7 for volume
            midivolume = d2 / 127.0;  // MIDI CC messages have a range from 0 to 127
            volume = constrain(midivolume, 0.0, 1.0);
            break;
          case 10:                  // CC10 for panning
            midipanl = d2 / 127.0;  // pan will be a float ranging from 0 (full left) to 1 (full right)
            panl = constrain(midipanl, 0.0, 1.0);
            midipanr = map(d2, 0, 127, 127, 0) / 127.0;
            panr = midipanr;
            break;
        }
        left_vol = panl * volume;
        right_vol = panr * volume;
        break;

      case midi::ProgramChange:
        d1 = MIDI.getData1();
        sample_no = d1;
        done_trig1 = 1;  //1 shot play when sample changed
        i = 0;
        samplePlaying = true;
        timer = millis();
        eeprom_write = 1;  //eeprom update flug on
        break;
    }
  }

  // From 0 to 127
  //-------------------------trigger----------------------------------
  old_trig1 = trig1;
  trig1 = digitalRead(TRIGGER);
  if (trig1 == 1 && old_trig1 == 0) {  //detect trigger signal low to high , before sample play was done
    done_trig1 = 1;
    i = 0;
    samplePlaying = true;
  }

  //-------------------------save to eeprom----------------------------------
  if (timer + 5000 <= millis() && eeprom_write == 1) {  //Memorized 5 seconds after sample number change
    eeprom_write = 0;
    eeprom_update();
  }
}
