#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <ADSR.h>
#include <tables/sin8192_int8.h> 
#include <mozzi_rand.h>
#include <mozzi_midi.h>

#define CONTROL_RATE 64

Oscil <8192, AUDIO_RATE> aOscil(SIN8192_DATA);
; 

// for triggering the envelope
EventDelay noteDelay;

ADSR <CONTROL_RATE, AUDIO_RATE> envelope;

boolean note_is_on = true;

void setup(){
  Serial.begin(115200);
  randSeed(); // fresh random
  noteDelay.set(2000); // 2 second countdown
  startMozzi(CONTROL_RATE);
}


unsigned int duration, attack, decay, sustain, release_ms;

byte state = 0;

byte const STATE_TOTAL = 2;

void updateControl(){
  if(noteDelay.ready()){


    byte attack_level = rand(240, 245);
    byte decay_level = rand(140,190);
    
    byte midi_note = 0;

    midi_note = rand(144, 154);

    sustain = rand(50, 60);
    decay = rand(50, 60);
    sustain = rand(1000, 1800);
    release_ms = rand(5, 10);

    int freq_note = (int)mtof(midi_note);

    envelope.setADLevels(attack_level,decay_level);
    envelope.setTimes(attack,decay,sustain,release_ms);    
    envelope.noteOn();
    aOscil.setFreq(freq_note);
    noteDelay.start(attack+decay+sustain+release_ms);
  }

  envelope.update();
} 


int updateAudio(){
  return (int) (envelope.next() * aOscil.next())>>8;
}


void loop(){
  audioHook(); // required here
}












