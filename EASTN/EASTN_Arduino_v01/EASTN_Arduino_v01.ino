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
#define N 150             //How many frequencies (max depends on memory and NUMBER)
#define NUMBER 3          // How Many Buttons(States)
#define CALIBRATION 100   // Define Calibration Here (From 0-255) Best works ~100-180



//----------------Include Libraries needed------------------
#include <EEPROM.h>

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
int buttons[] = {button1 , button2, button3};  // add additional Buttons
byte saveButton = 3; // To store away Values in EEPROM

//-----Calculate Current Area Differences----------------------
byte areaDiffTemp[N]; // Aray to store current AreaDifferences

// -----------Store current Area Differences Values------------
byte areaDifferences[NUMBER][N];      // Array to store the AreaDifference for each Frequency per State
byte areaDifferencesSum[NUMBER][1];
byte lastButtonState[] ={LOW,LOW,LOW};
byte buttonState = LOW;
//------------------------Compare------------------
int currentOutput = 0;
int output = 0;
byte stateOutput = 0;
int tempAD = 0;
//-------------------Output------------------------
int output1 = 12; // leds, sound etc...
int output2 = 8;
int output3 = 7;
int outputs[] = {output1, output2, output3}; // add additional Outputs



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
  pinMode(button1, INPUT);     // Setup Input Buttons
  pinMode(button2, INPUT);     // |
  pinMode(button3, INPUT);     //-+

  pinMode(output1, OUTPUT);    // Setup Output Buttons
  pinMode(output2, OUTPUT);    // |
  pinMode(output3, OUTPUT);    //-+
  
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
  int time = millis();
//============Refreshing TempValues========================
//  byte tempAD = 0;
  byte tempAreaDifference = 0;
  currentOutput = -1;
  
  if(digitalRead(saveButton)){
    saveButtonRoutine();
  }
  
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
      flash(3);
    }
  }
  
//==================Calculate Current Area================
  for(byte d=0;d<N;d++){
    tempAD = (byte) abs(results[d]-baseline[d]); // Difference between the current (results) and the stored "Zero"-Values (baseline)
    areaDiffTemp[d] = tempAD;
  }

//===============Store Current Area=========================
  for(byte i=0;i<NUMBER;i++){        // When a Button gets pressed the Current areaDifferences are 
    buttonState = digitalRead(buttons[i]);
    if(buttonState != lastButtonState[i]){  //stored in the Array related to that Button
      lastButtonState[i] = buttonState;
      if(buttonState == HIGH){
        for(byte d=0;d<N;d++){
          areaDifferences[i][d] = areaDiffTemp[d];
        }
      }  
    }
  }

//===============Compare ============================ 
  for(byte i=0;i<NUMBER;i++){
    //int calibrationPot = analogRead(A1);        // You can also add a potentiometer to A1 and try calibrating it this way 
    output = 0;                                   // Refresh Temp Value
    int tempOutput = 0;                           // "
    for(byte d=0;d<N;d++){
      tempOutput = abs(areaDifferences[i][d] - areaDiffTemp[d]);  //Calculate Differece between values for this frequency and
                                                                  //save it away in tempOutpout
      if(tempOutput > CALIBRATION){                              //check if difference is bigger than CALIBRATION/calibrationPot (if potentiometer on A1)
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
  for(byte i=0;i<NUMBER;i++){              //Refresh outputs. All on LOW
    digitalWrite(outputs[i],LOW);
  }
  digitalWrite(actualOutput, HIGH);         // Put current OutputState to HIGH
int timenew = time - millis();
Serial.println(timenew);// stop transmitting
delay(500);

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

void saveButtonRoutine(){
  for(byte i=0;i<NUMBER;i++){
    for(byte d=0;d<N;d++){
      EEPROM.write((d+i*N), areaDifferences[i][d]);  //addr = (d+i*N); b1 => 0-N; b2 => N+1-2N; b3 => (2N+1)-3N 
    }
  }
  flash(5);
}

void flash(int times){
  for(byte i=0;i<times;i++){
    for(byte i = 0;i<NUMBER;i++){        //Refresh outputs. All on LOW
      digitalWrite(outputs[i],LOW);
    }
    delay(50);
    for(int i = 0;i<NUMBER;i++){        //Refresh outputs. All on LOW
      digitalWrite(outputs[i],HIGH);
    }
    delay(50); 
  }  
}
    
 
