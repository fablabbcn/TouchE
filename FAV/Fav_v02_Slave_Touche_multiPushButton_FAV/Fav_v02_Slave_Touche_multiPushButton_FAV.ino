//****************************************************************************************
// Illutron take on Disney style capacitive touch sensor using only passives and Arduino
// Dzl 2012
//****************************************************************************************


//                              10n
// PIN 9 --[10k]-+-----10mH---+--||-- OBJECT
//               |            |
//              3.3k          |
//               |            V 1N4148 diode
//              GND           |
//                            |
//Analog 0 ---+------+--------+
//            |      |
//          100pf   1MOmhm
//            |      |
//           GND    GND


#define SET(x,y) (x |=(1<<y))				//-Bit set/clear macros
#define CLR(x,y) (x &= (~(1<<y)))       		// |
#define CHK(x,y) (x & (1<<y))           		// |
#define TOG(x,y) (x^=(1<<y))            		//-+

//****************************************************************************************
// FabLabBCN take on provided Disney Style capacitive touch sensor to:
//  1. Store & Calculate all Values on the Arduino
//  2. Enhance Resolution through comparing Arrays rather then Maxima 
// FabLabBCN && clemniem 2014
// https://github.com/clemniem/fabTouche
// https://github.com/fablabbcn/TouchE
//****************************************************************************************

//-----------Changeable Variables-----------------<<----<<--
#define N 120             //How many frequencies (max depends on memory and NUMBER)
#define NUMBER 3          // How Many Buttons(States)
#define CALIBRATION 100   // Define Calibration Here (From 0-255) Best works ~100-180



//----------------Include Libraries needed------------------
#include <EEPROM.h>
#include <Wire.h>

//----------------Measure Values----------------------------
int results[N];         //-Filtered result buffer
int freq[N];            //-Filtered result buffer
int sizeOfArray = N;
byte counter = 0;

//----------------Store Norm Values-------------------------
int baseline[N]; // baseline
int button1 = 2; // Change Pins accordingly
int button2 = 4; // "
int button3 = 6; // "
int buttons[] = {
  button1 , button2, button3};  // add additional Buttons
//byte saveButton = 3; // To store away Values in EEPROM
byte buttonPin = 13;
int addr = 0;

//-----Calculate Current Area Differences----------------------
byte areaDiffTemp[N]; // Aray to store current AreaDifferences

// -----------Store current Area Differences Values------------
byte areaDifferences[NUMBER][N];      // Array to store the AreaDifference for each Frequency per State
byte areaDifferencesSum[NUMBER][1];
//byte lastButtonState[] ={LOW,LOW,LOW};
byte lastButtonState = 0;    //
int buttonPushCounter = 0;   // counter for the number of button presses
byte buttonState = LOW;
unsigned long previousMillis = 0;
int tempAD;
//------------------------Compare------------------
int currentOutput = 0;
int output = 0;
byte stateOutput = 0;
byte stateOutputs[3] = {
  0,0,0};
//-------------------Output------------------------
int output1 = 6;  // 6 = Red
int output2 = 7;  // 7 = Blue
int output3 = 8;  // 8 = Green
int outputs[] = {
  output1, output2, output3}; // add additional Outputs

//......................Output...END...............


void setup()
{
  //-----------------------Touche by Illutron-----------------------
  TCCR1A=0b10000010;        //-Set up frequency generator
  TCCR1B=0b00011001;        //-+
  ICR1=110;
  OCR1A=55;

  pinMode(9,OUTPUT);        //-Signal generator pin
  Serial.begin(9600);

  for(byte i=0;i<N;i++) results[i]=0;//-Preset results 

  //-------------------------FabTouche by FabLabBCN-----------------

  Wire.begin(4);                // join i2c bus (address optional for master)
  Wire.onRequest(requestEvent); // register event

    pinMode(button1, INPUT);     // Setup Input Buttons
  pinMode(button2, INPUT);     // |
  pinMode(button3, INPUT);     //-+

  pinMode(output1, OUTPUT);    // Setup Output Buttons
  pinMode(output2, OUTPUT);    // |
  pinMode(output3, OUTPUT);    //-+

  flash(60);


  for(byte d=0;d<N;d++){
    baseline[d] = 0;           //Preset-values for baseline
  }

  for(byte i=0;i<NUMBER;i++){  // Restore saved areaDifferences from EEPROM
    for(byte d=0;d<N;d++){
      //addr = (d+i*N);
      areaDifferences[i][d] = EEPROM.read(d+i*N);
    }
  }


}

void loop(){

  //============Refreshing TempValues========================
  //  byte tempAD = 0;
  byte tempAreaDifference = 0;
  currentOutput = -1;

  //  if(digitalRead(saveButton)){
  //    saveButtonRoutine();
  //  }

  //   

  //=====================Measure Values======================
  for(byte d=0;d<N;d++){
    int v=analogRead(A0);                   //-Read response signal
    CLR(TCCR1B,0);                          //-Stop generator
    TCNT1=0;                                //-Reload new frequency
    ICR1=d;                                 // |
    OCR1A=d/2;                              //-+
    SET(TCCR1B,0);                          //-Restart generator
    results[d]=results[d]*0.5+(int)(v)*0.5; //Filter results
    freq[d] = d;
  }


  //===========Store "Zero"-Values (in baseline)=============
  if(counter <= 5){ // for 5 cicles save the zero value
    for(byte d=0;d<N;d++){
      baseline[d] = results[d];
    }
    counter++;
    if(counter == 6){
      saveAreaDiffArray(0); // Store the No-Touch-Value     
      Serial.println("0"); 
      flashRed(3);
    }
  }

  //==================Calculate Current AreaDiff================
  for(byte d=0;d<N;d++){
    tempAD = (byte) abs(results[d]-baseline[d]); // Difference between the current (results) and the stored "Zero"-Values (baseline)
    areaDiffTemp[d] = tempAD;
  }

  //===============Store Current AreaDiff=========================
  //  for(byte i=0;i<NUMBER;i++){        // When a Button gets pressed the Current areaDifferences are 
  //    buttonState = digitalRead(buttons[i]);
  //    if(buttonState != lastButtonState[i]){  //stored in the Array related to that Button
  //      lastButtonState[i] = buttonState;
  //      if(buttonState == HIGH){
  //        saveAreaDiffArray(i);
  //      }  
  //    }
  //  }

  //-----
  buttonState = digitalRead(buttonPin);

  unsigned long currentMillis = millis();

  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      buttonPushCounter++;
      previousMillis = currentMillis;   
    } 
  }

  lastButtonState = buttonState;

  if(currentMillis - previousMillis > 1000 && buttonPushCounter > 0) {
    if (buttonPushCounter == 1) {
      saveAreaDiffArray(1);
      Serial.println("1");
    } 
    else if (buttonPushCounter == 2) {
      saveAreaDiffArray(2);
      Serial.println("2");
    }
    else if (buttonPushCounter >= 3) {
      saveButtonRoutine();
      Serial.println("SAVE");
    }
    buttonPushCounter = 0;
  }

  //===============Compare AreaDifferences============================ 
  for(byte i=0;i<NUMBER;i++){
    //int calibrationPot = analogRead(A1);        // You can also add a potentiometer to A1 and try calibrating it this way 
    output = 0;                                   // Refresh Temp Value
    int tempOutput = 0;                           // "
    for(byte d=0;d<N;d++){
      tempOutput = abs(areaDifferences[i][d] - areaDiffTemp[d]);  //Calculate Differece between values for this frequency and
      //save it away in tempOutpout
      if(tempOutput > CALIBRATION){                               //check if difference is bigger than CALIBRATION/calibrationPot (if potentiometer on A1)
        output += tempOutput;                                     //accumulate Area of values that are bigger
      }
    }  
    areaDifferencesSum[i][0] = output;
    if(output < currentOutput || currentOutput < 0){ //Compare the areas. The number of the lowest (=most corresponding) 
      currentOutput = output;                        //will be saved in stateOutput
      stateOutput = i;
    }
  }

  //=====================Output=========================
  byte actualOutput = outputs[stateOutput]; //convert State to the corresponding output Pin

  //  for(byte i=0;i<NUMBER;i++){               //Refresh outputs. All on LOW
  //    digitalWrite(outputs[i],HIGH);
  //  }


  switch (stateOutput) {
  case 0:
    digitalWrite(outputs[0],HIGH);
    digitalWrite(outputs[1],HIGH);
    digitalWrite(outputs[2],HIGH);
    break;
  case 1:
    digitalWrite(outputs[0],HIGH);
    digitalWrite(outputs[1],LOW);
    digitalWrite(outputs[2],LOW);
    break;
  case 2:
    digitalWrite(outputs[0],LOW);
    digitalWrite(outputs[1],LOW);
    digitalWrite(outputs[2],LOW);
    break;
  }



  stateOutputs[2] = stateOutputs[1];
  stateOutputs[1] = stateOutputs[0];
  stateOutputs[0] = stateOutput;
  //  Serial.print("[");
  //  Serial.print(stateOutputs[0]);
  //  Serial.print(",");
  //  Serial.print(stateOutputs[1]);
  //  Serial.print(",");
  //  Serial.print(stateOutputs[2]);
  //  Serial.print("]");  

  ////==================printOut for Debugging============
  //  for(byte i=0;i<NUMBER;i++){
  //    Serial.print("i: ");
  //    Serial.print(i)
  //    Serial.print(" : ")
  //    Serial.print(areaDifferencesSum[i][0])
  //    Serial.print(" || ")
  //  }
  //  Serial.println("-------------------------------");

  ////==========Sending Results to Processing=============
  ////!!!! You have to Uncomment the SendDataTab !!!! <<<<<-----

  //  PlottArray(1,freq,results); //Sending Results to Processing via Serial Port

}

// -----------------------------
// Function that Flashes n-times with all LEDs, for debugging
void flash(int times){
  for(byte i=0;i<times;i++){
    for(byte i = 0;i<NUMBER;i++){        
      digitalWrite(outputs[i],LOW);
    }
    delay(50);
    for(int i = 0;i<NUMBER;i++){        
      digitalWrite(outputs[i],HIGH);
    }
    delay(50); 
  }  
}

void flashRed(int times){
  for(byte i=0;i<times;i++){
    digitalWrite(outputs[0],LOW);
    digitalWrite(outputs[1],HIGH);
    digitalWrite(outputs[2],HIGH);
    delay(50);
    digitalWrite(outputs[0],HIGH);
    digitalWrite(outputs[1],HIGH);
    digitalWrite(outputs[2],HIGH);    
    delay(50); 
  }  
}

//------------------------------
// saves away the current areaDifferences into the states array
void saveAreaDiffArray(byte state){
  for(byte d=0;d<N;d++){
    areaDifferences[state][d] = areaDiffTemp[d];
  }
  flashRed(2);
}

//------------------------------
//stores Away the AreaDifferences in the EEPROM 




void saveButtonRoutine(){
  for(byte i=0;i<NUMBER;i++){
    for(byte d=0;d<N;d++){
      EEPROM.write((d+i*N), areaDifferences[i][d]);  //addr = (d+i*N); b1 => 0-N; b2 => N+1-2N; b3 => (2N+1)-3N 
    }
  }
  flashRed(10);
}

//------------------------------
//sends the last three states, which are saved in stateOutputs to the wire as slave
void requestEvent()
{
  Wire.write(stateOutputs, 3); // sends one byte  
}





