/*  Example of Wavepacket synthesis, using Mozzi sonification library.
 This sketch draws on Miller Puckette's 
 Pure Data example, F14.wave.packet.pd, with
 two overlapping streams of wave packets.
 
 Circuit: Audio output on digital pin 9 (for STANDARD mode on a Uno or similar), or 
 check the README or http://sensorium.github.com/Mozzi/
 
 Mozzi help/discussion/announcements:
 https://groups.google.com/forum/#!forum/mozzi-users
 
 Tim Barrass 2013.
 This example code is in the public domain.
 */

#include <WavePacket.h>
#include <RollingAverage.h>
#include <EventDelay.h>
#include <twi_nonblock.h>
#include <Oscil.h>
#include <tables/triangle_analogue512_int8.h> // wavetable
#include <tables/triangle512_int8.h> // wavetable
#include <AudioDelayFeedback.h>
#include <mozzi_midi.h> // for mtof


EventDelay sleepDelay;


/** i2c **/

#define SLAVE_SENDER_DEVICE 4   // Touche device address
#define SENT_BYTES 3  //The Number of Bytes you want to send and receive


static volatile uint8_t acc_status = 0;
#define ACC_IDLE 0
#define ACC_READING 1
#define ACC_WRITING 2

int state0 = 0;
int state1 = 0;
int state2 = 0;



uint8_t receivedBytes[SENT_BYTES]; // Array to store your received bytes

void setup_wire(){
  initialize_twi_nonblock();

  acc_status = ACC_IDLE;
}

/// ---------- non-blocking version ----------
void initiate_read_wire(){
  // Reads num bytes starting from address register on device in to _buff array
  // indicate that we are transmitting
  //   transmitting = 1;
  // set address of targeted slave
  txAddress = SLAVE_SENDER_DEVICE;
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;  

  // put byte in tx buffer
  byte x = 0;
  txBuffer[txBufferIndex] = x; //>>>>>>>>???<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  ++txBufferIndex;
  // update amount in buffer   
  txBufferLength = txBufferIndex;

  twi_initiateWriteTo(txAddress, txBuffer, txBufferLength);
  acc_status = ACC_WRITING;
}

void initiate_request_wire(){
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
  // indicate that we are done transmitting
  //   transmitting = 0;

  uint8_t read = twi_initiateReadFrom(SLAVE_SENDER_DEVICE, SENT_BYTES);
  acc_status = ACC_READING;
}

void finalise_request_wire(){
  uint8_t read = twi_readMasterBuffer( rxBuffer, SENT_BYTES);

  // set rx buffer iterator vars
  rxBufferIndex = 0;
  rxBufferLength = read;

  uint8_t i = 0;
  while( rxBufferLength - rxBufferIndex > 0)         // device may send less than requested (abnormal)
  { 
    receivedBytes[i] = rxBuffer[rxBufferIndex];
    ++rxBufferIndex;
    i++;
  }
  //-----------------------PRINTOUT for DEBUGGING-------------------------
  //    Serial.print("[");
  //    for(int i = 0;i<SENT_BYTES;i++){
  //      Serial.print(receivedBytes[i]);
  //      Serial.print(",");
  //    }
  //    Serial.print("]");
  //    Serial.println();
  //    Serial.println("-------------");
  //----------------------------------------------------------------------

  acc_status = ACC_IDLE;
}
/// ----end------ non-blocking version ----------

// Writes val to address register on device
void device_writeTo(byte address, byte val) {
  //   Wire.beginTransmission(SLAVE_SENDER_DEVICE); // start transmission to device   
  twowire_beginTransmission(SLAVE_SENDER_DEVICE); // start transmission to device   
  //   Wire.send(address);             // send register address
  twowire_send( address );
  //   Wire.send(val);                 // send value to write
  twowire_send( val );  
  //   Wire.endTransmission();         // end transmission
  twowire_endTransmission();

}

void update_wire(){
  //------StateMachine----------------------------------------------------------------
  switch( acc_status ){
  case ACC_IDLE:
    //------------------Update your Variable from the array --------
    state0 = (int) (receivedBytes[0]); // receivedBytes[0] x reading
    state1 = (int) (receivedBytes[1]); // receivedBytes[0] x reading
    state2 = (int) (receivedBytes[2]); // receivedBytes[0] x reading

    //--------------------------------------------------------------
    initiate_read_wire();
    break;
  case ACC_WRITING:
    if ( TWI_MTX != twi_state ){
      initiate_request_wire();
    }
    break;
  case ACC_READING:
    if ( TWI_MRX != twi_state ){
      finalise_request_wire();
    }
    break;
  }
}

/** END - i2C **/


#define CONTROL_RATE 128 // powers of 2 please

Oscil<TRIANGLE_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aTriangle(TRIANGLE_ANALOGUE512_DATA); // audio oscillator
Oscil<TRIANGLE512_NUM_CELLS, CONTROL_RATE> kDelSamps(TRIANGLE512_DATA); // for modulating delay time, measured in audio samples

AudioDelayFeedback <128> aDel;

// the delay time, measured in samples, updated in updateControl, and used in updateAudio 
unsigned char del_samps;

#define FUNDAMENTAL_PIN 0
#define BANDWIDTH_PIN 1
#define CENTREFREQ_PIN 2

// for smoothing the control signals
// use: RollingAverage <number_type, how_many_to_average> myThing
RollingAverage <int, 32> kAverageF;
RollingAverage <int, 32> kAverageBw;
RollingAverage <int, 32> kAverageCf;

WavePacket <DOUBLE> wavey; // DOUBLE selects 2 overlapping streams

Q16n16 deltime;


void setup(){
  startMozzi();
  Serial.begin(115200);
  setup_wire();
  aTriangle.setFreq(mtof(20.f));
  kDelSamps.setFreq(.163f); // set the delay time modulation frequency (ie. the sweep frequency)
  aDel.setFeedbackLevel(127); // can be -128 to 127
}

boolean sleep = false;
float volume = 1.0;


int mapStatesA(byte state) {

  Serial.println(volume);
  
  if (state0 == 0 && state1 == 0 && state2 != 0) {
    sleepDelay.start(10000);
  } 
  else if (state0 == 0 && state1 == 0 && state2 == 0) {

    volume = volume - 0.05;
    volume = constrain(volume, 0.0, 1.0);

    if(sleepDelay.ready()){
      volume = 0;
      sleep = true;
      pauseMozzi();
    }
  }
  else if (state0 != 0 && state1 == 0 && state2 == 0 && sleep == true) {
    volume = 1.0;
    unPauseMozzi();
    sleep = false;
  }

  if (state0 != 0 || state1 != 0 || state2 != 0) {
    volume = 1.0;
  }




  if (state0 != 0 && state1 == 0) {
    return map(int(state), 0, 2, 0, 20);
  } 
  else if (state0 != 0 && state1 != 0) {
    return map(int(state), 0, 2, 0, 50);
  }
  else if (state0 == 0 && state1 == 0) {
    return 0;
  } 
  else {
    return map(int(state), 0, 2, 0, 100);
  }
}

int mapStatesB(byte state) {
  state = constrain(state, 1, 2);
  return map(int(state), 0, 2, 200, 720);

}

int mapStatesC(byte state) {  
  state = constrain(state, 1, 2);
  return map(int(state), 0, 2, 400, 1013);
}

int mapStatesD(byte state) {  
  return map(int(state), 0, 2, 17, 50);
}

void updateControl(){
  update_wire();
  wavey.set(kAverageF.next(mapStatesA(state0))+1, 

  kAverageBw.next(mapStatesB(state1)), 
  kAverageCf.next(mapStatesC(state2)));
  deltime = Q8n0_to_Q16n16(mapStatesD(state1)) + ((long)kDelSamps.next()<<12); 

}



int updateAudio(){
  char w = wavey.next()>>8; // >>8 for AUDIO_MODE STANDARD
  return w/8 + aDel.next(w, deltime)*volume; // mix some straight signal with the delayed signal

}



void loop(){
  audioHook(); // required here
}
















