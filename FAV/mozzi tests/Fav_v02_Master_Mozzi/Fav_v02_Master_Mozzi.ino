/*  Example reading an Arduino Slave-Sender
    using I2C communication without blocking audio synthesis,
    using Mozzi sonification library.
  
    Demonstrates use of twi_nonblock functions
    to replace processor-blocking Wire methods.
  
    Circuit: Audio output on digital pin 9.
  
    Mozzi help/discussion/announcements:
    https://groups.google.com/forum/#!forum/mozzi-users
  
    Marije Baalman 2012.
    Small modifications by TIm Barrass to retain Mozzi compatibility.
    This example code is in the public domain.
*/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <twi_nonblock.h>

#define CONTROL_RATE 128 // powers of 2 please

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);



#define SLAVE_SENDER_DEVICE 4   // Touche device address
#define SENT_BYTES 3  //The Number of Bytes you want to send and receive
#define VOLUME 80     //Set Output Volume


static volatile uint8_t acc_status = 0;
#define ACC_IDLE 0
#define ACC_READING 1
#define ACC_WRITING 2

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


void setup(){
  Serial.begin(9600);
  setup_wire();
  startMozzi(CONTROL_RATE);
  aSin.setFreq(800);
}

int accx = 0;


void updateControl(){  
  if (accx == 0){
    aSin.setFreq( 800 );
  } 
  else if (accx == 1) {
    aSin.setFreq( 1000  );
  } 
  else if (accx == 2) {
    aSin.setFreq( 1400 );
  }
//------StateMachine----------------------------------------------------------------
  switch( acc_status ){
  case ACC_IDLE:
    //------------------Update your Variable from the array --------
    accx = (int) (receivedBytes[0]); // receivedBytes[0] x reading
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
 //-----------------------------------------------------------------------
}


int updateAudio(){
  return (aSin.next()*VOLUME)>>8;
}


void loop(){
  audioHook(); // required here
}

