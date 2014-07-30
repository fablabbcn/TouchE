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

/** i2c **/

#define SLAVE_SENDER_DEVICE 4   // Touche device address
#define SENT_BYTES 3  //The Number of Bytes you want to send and receive
#define VOLUME 80     //Set Output Volume


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
  //  Serial.print("[");
  //  for(int i = 0;i<SENT_BYTES;i++){
  //    Serial.print(receivedBytes[i]);
  //    Serial.print(",");
  //  }
  //  Serial.print("]");
  //  Serial.println();
  //  Serial.println("-------------");
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

EventDelay changeState;

#define FUNDAMENTAL_PIN 0
#define BANDWIDTH_PIN 1
#define CENTREFREQ_PIN 2

// for smoothing the control signals
// use: RollingAverage <number_type, how_many_to_average> myThing
RollingAverage <int, 32> kAverageF;
RollingAverage <int, 32> kAverageBw;
RollingAverage <int, 32> kAverageCf;

WavePacket <DOUBLE> wavey; // DOUBLE selects 2 overlapping streams

byte states[] = {
  0, 0, 0};



void setup(){
  startMozzi();
  Serial.begin(115200);
  changeState.set(1000); // 2 second countdown
  setup_wire();
}


void updateState(byte newState){
  states[2] = states[1];
  states[1] = states[0];
  states[0] = newState;
  for(int i = 0; i <= 2; i++) { 
    Serial.print(states[i]); 
    Serial.print(",");    

  } 
  Serial.println();
}

int mapStatesA(byte state) {
  return map(int(state), 0, 2, 0, 800);
}

int mapStatesB(byte state) {
  return map(int(state), 0, 2, 500, 1023);
}

int mapStatesC(byte state) {
  return map(int(state), 0, 2, 0, 500);
}

void updateControl(){
  update_wire();

  //  if(changeState.ready()){
  //    updateState(random(0, 3));
  //    changeState.start();
  //  }

  wavey.set(kAverageF.next(mapStatesA(state0))+1, 
  kAverageBw.next(mapStatesB(state1)), 
  kAverageCf.next(mapStatesC(state2)));
}



int updateAudio(){
  return wavey.next()>>8; // >>8 for AUDIO_MODE STANDARD
}



void loop(){
  audioHook(); // required here
}




