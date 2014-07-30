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

    state++;

    // choose envelope levels
    //    byte attack_level = 1000;
    //    byte decay_level = 10;
    //    int freq_note = random(1200, 2000);
    //
    //    attack = 1000;
    //    decay = 40;
    //    sustain = 2000;
    //    release_ms = 0;


    byte attack_level = rand(240, 245);
    byte decay_level = rand(140,190);



  byte midi_note = 0;

//    switch (state){
//    case 1:
      midi_note = rand(144, 154);
//      break; 
//    case 2:
//      midi_note = rand(120, 140);
//      break; 
//    }


//    sustain = rand(1000/2, 1800/2);
//    decay = rand(200, 400);
//    sustain = rand(1000/2, 1800/2);
//    release_ms = rand(90, 170);

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

    // print to screen
    Serial.print("state\t"); 
    Serial.println(state);
    Serial.print("midi_note\t"); 
    Serial.println(midi_note);
    Serial.print("attack_level\t"); 
    Serial.println(attack_level);
    Serial.print("decay_level\t"); 
    Serial.println(decay_level);
    Serial.print("attack\t\t"); 
    Serial.println(attack);
    Serial.print("decay\t\t"); 
    Serial.println(decay);
    Serial.print("sustain\t\t"); 
    Serial.println(sustain);
    Serial.print("release\t\t"); 
    Serial.println(release_ms);
    Serial.println();

    if (state >= STATE_TOTAL) {
      state = 0;
    } 

  }

  envelope.update();
} 


int updateAudio(){
  return (int) (envelope.next() * aOscil.next())>>8;
}


void loop(){
  audioHook(); // required here
}











