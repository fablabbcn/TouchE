/*  Example of flanging,
 using Mozzi sonification library.
 
 Demonstrates AudioDelayFeedback.
 
 Circuit: Audio output on digital pin 9 (on a Uno or similar), or 
 check the README or http://sensorium.github.com/Mozzi/
 
 Mozzi help/discussion/announcements:
 https://groups.google.com/forum/#!forum/mozzi-users
 
 Tim Barrass 2012-13.
 This example code is in the public domain.
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/triangle_analogue512_int8.h> // wavetable
#include <tables/triangle512_int8.h> // wavetable
#include <AudioDelayFeedback.h>
#include <mozzi_midi.h> // for mtof
#include <EventDelay.h>

#define CONTROL_RATE 128 // powers of 2 please

Oscil<TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTriangle(TRIANGLE_ANALOGUE512_DATA); // audio oscillator
Oscil<TRIANGLE512_NUM_CELLS, CONTROL_RATE> kDelSamps(TRIANGLE512_DATA); // for modulating delay time, measured in audio samples

AudioDelayFeedback <128> aDel;

// the delay time, measured in samples, updated in updateControl, and used in updateAudio 
unsigned char del_samps;



Q16n16 deltime;

byte const NOTE_START = 40;
byte note = NOTE_START;
byte const NOTE_TOTAL = 100;

byte state = 0;

EventDelay changeState;


void setup(){
  startMozzi(CONTROL_RATE);
  aTriangle.setFreq(mtof(48.f));
  kDelSamps.setFreq(.163f); // set the delay time modulation frequency (ie. the sweep frequency)
  aDel.setFeedbackLevel(-111); // can be -128 to 127
  changeState.set(2000); // 2 second countdown
  Serial.begin(115200);
}


void updateControl(){
//
//  if(changeState.ready()){
//    state = random(0, 3);
//    Serial.println(state);
//    changeState.start();
//  }

  state = 3;

  switch (state){
  case 0:
    note = 0;
    break; 
  case 1:
    note = note + 40;
    break; 
  case 2:
    note = 100;
    break; 
  case 3:
    note = 48.f;
    break; 
  }


  int freq_note = (int)mtof(note);


  aTriangle.setFreq(freq_note);

  // delay time range from 0 to 127 samples, @ 16384 samps per sec = 0 to 7 milliseconds
  //del_samps = 64+kDelSamps.next();

  // delay time range from 1 to 33 samples, @ 16384 samps per sec = 0 to 2 milliseconds
  //del_samps = 17+kDelSamps.next()/8;

  deltime = Q8n0_to_Q16n16(17) + ((long)kDelSamps.next()<<12); 

  if (note >= NOTE_TOTAL) {
    note = NOTE_START;
  } 


}


int updateAudio(){
  char asig = aTriangle.next(); // get this so it can be used twice without calling next() again
  //return asig/8 + aDel.next(asig, del_samps); // mix some straight signal with the delayed signal
  //return aDel.next(aTriangle.next(), del_samps); // instead of the previous 2 lines for only the delayed signal
  return asig/8 + aDel.next(asig, deltime); // mix some straight signal with the delayed signal
}


void loop(){
  audioHook();
}







