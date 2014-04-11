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


//-----------Changeable---------------------------<<----<<--
#define N 140  //How many frequencies (max depends on memory)
#define NUMBER 3 // How Many Buttons(States)
#define CALIBRATION 150 // Define Calibration Here 
//------------------------------------------------<<----<<--

//---------------Processing Link--------------------
#define ENABLE_PROCESSING true


//---------------Measure Values--------------------
int results[N];          //-Filtered result buffer
int freq[N];            //-Filtered result buffer
int sizeOfArray = N;

//----------------Store Norm Values----------------
int values1[N]; //Messwerte der verschiedenen Zustände
int button1 = 2; // Change Pins accordingly
int button2 = 4; // "
int button3 = 6; // "
int buttons[] = {
  button1 , button2, button3};  // add additional Buttons

//-----Calculate Current Area----------------------
int areaCurrent;
byte areaDifArray[N];

// -----------Store current Area Values------------
byte areaDifferences[NUMBER][N];  // Array to store the AreaDifference for each Frequency per Button

//------------------------Compare------------------
float currentOutput = 0.0;
float output = 0.0;
byte stateOutput = 0;
boolean touch = false;

//-------------------Output------------------------
int output1 = 11; // leds, sound etc...
int output2 = 12;
int output3 = 13;
int outputs[] = {
  output1, output2, output3}; // add additional Outputs

//......................Output...END...............



void setup()
{
  //-----------------------OLDCODE-----------------------
  TCCR1A=0b10000010;        //-Set up frequency generator
  TCCR1B=0b00011001;        //-+
  ICR1=110;
  OCR1A=55;

  int START = 0;

  pinMode(button1, INPUT);
  pinMode(9,OUTPUT);        //-Signal generator pin
  pinMode(8,OUTPUT);        //-Sync (test) pin
  Serial.begin(9600);

  for(byte i=0;i<N;i++) results[i]=0;//-Preset results 

  //.........................OLDCODE..END..............

  //----------------------------NEWCODE----------------

  pinMode(button2, INPUT);
  pinMode(button3, INPUT);

  pinMode(output1, OUTPUT);
  pinMode(output2, OUTPUT);
  pinMode(output3, OUTPUT);

  for(byte d=0;d<N;d++){
    values1[d] = 0;       //Preset-values
  }

  //..................NEWCODE...END.......................

}

void loop(){

  //============Refreshing TempValues========================
  byte tempAD = 0;
  byte tempAreaDifference = 0;
  areaCurrent = 0;
  currentOutput = -1;


  //=====================Measure Values======================
  //Serial.println("---Measure Values---");
  for(unsigned int d=0;d<N;d++)
  {
    int v=analogRead(0);    //-Read response signal
    CLR(TCCR1B,0);          //-Stop generator
    TCNT1=0;                //-Reload new frequency
    ICR1=d;                 // |
    OCR1A=d/2;              //-+
    SET(TCCR1B,0);          //-Restart generator
    results[d]=results[d]*0.5+(int)(v)*0.5; //Filter results
    freq[d] = d;
  }
  //   plot(v,0);              //-Display
  //   plot(results[d],1);
  //   delayMicroseconds(1);

  //===========Store "Zero"-Values (in values1)=============
  //Serial.println("---Store "Zero" Values---");
  for(byte s=0;s<5;s++){ // for 5 cicles save the zero value
    for(int d=0;d<N;d++){
      values1[d] = results[d];
    }
  }

  //==================Calculate Current Area================
  //Serial.println("---Calculate Current Area---");
  for(byte d=0;d<N;d++){
    tempAD = (byte) abs(results[d]-values1[d]); // Difference between the current (results) and the stored "Zero"-Values (values1)
    areaDifArray[d] = tempAD;
    areaCurrent += tempAD;
  }

  //===============Store Current Area=========================

  for(byte i=0; i < NUMBER ; i++){        // When a Button gets pressed the Current areaDifferences are 
    if(digitalRead(buttons[i]) == HIGH){  //stored in the Array related to that Button
      for(byte d=0;d<N;d++){
        areaDifferences[i][d] = areaDifArray[d];
      }  
    }
  }

  ////----printOut----
  //    Serial.println("---Store Current Area---");
  //    Serial.print(" Button");
  //    Serial.print(i);
  //    Serial.print(" = ");
  //    Serial.print(digitalRead(buttons[i]));
  //    Serial.print(" Area: ");
  //    Serial.println(areaValues[i][0]);

  //===============Compare ============================ 

  for(int i=0;i<NUMBER;i++){
    output = 0;          // Refresh Temp Value
    int tempOutput = 0;  // "
    for(byte d=0;d<N;d++){
      tempOutput = abs(areaDifferences[i][d] - areaDifArray[d]);  //Calculate Differece between values for this frequency
      //Save it away in tempOutpout
      if(tempOutput > CALIBRATION){                               //check if difference is bigger than CALIBRATION
        output += tempOutput;                                     //accumulate Area of values that are bigger    
      }
    }  
    if(output < currentOutput || currentOutput < 0){ //Compare the areas. The number of the lowest (=most corresponding) 
      currentOutput = output;                        //will be saved in stateOutput
      stateOutput = i;
    }
  }

  ////------printout------
  //    Serial.println("---Compare---");
  //    Serial.print("tempOutput: ");
  //    Serial.print("i: ");
  //    Serial.print(i);
  //    Serial.print(": ");
  //    Serial.print(tempOutput);
  //    Serial.print("  stateoutput");
  //    Serial.print(" = ");
  //    Serial.println(stateOutput);

  //=====================Output=========================
  //Serial.println("---Output---");  
  int tempOutput = outputs[stateOutput]; //convert State to the corresponding output Pin

  for(int i = 0;i<NUMBER;i++){        //Refresh outputs. All on LOW
    digitalWrite(outputs[i],LOW);
  }
  digitalWrite(tempOutput, HIGH); // Put current OutputState to HIGH
  if(stateOutput > 0){
    touch = true;
  }
  else{
    touch = false;
  }


  //==========Sending Results to Processing=============

  #if ENABLE_PROCESSING
    PlottArray(1,freq,results); //Sending Results to Processing via Serial Port
    TOG(PORTB,0);            //-Toggle pin 8 after each sweep (good for scope)
  #endif
}






