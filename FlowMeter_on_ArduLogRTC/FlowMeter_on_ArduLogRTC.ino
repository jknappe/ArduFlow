//===========================================================================================
// PREAMBLE
//-------------------------------------------------------------------------------------------

//Logger type: Arduino Uno on ArduLogRTC shield (http://www.hobbytronics.co.uk/ardulog-rtc)
 
//Program author: Jan Knappe
//Contact: jan.knappe@tcd.ie

//===========================================================================================

//===========================================================================================
// PREAMBLE
//-------------------------------------------------------------------------------------------

// INCLUDE LIBRARIES
//---------------------------------------------------------  
#include <Wire.h>                                               //Connecting to RTC
#include <RTClib.h>                                             //Using RTC for timestamp
  RTC_DS1307 RTC;                                               //Define RTC object  
#include <SD.h>                                                 //Communicate with SD card  
  File file;                                                    //Defining File object for SD card

// DEFINE PINS
//---------------------------------------------------------  
const byte LEDPin1 = 5;                                         //LED pin on board
const byte LEDPin2 = 2;                                         //LED pin on button
const byte SDPin = 10;                                          //SD pin (CS/SS)
const byte switchPin = 3;                                       //Reed switch pin 

// DEFINE VARIABLES
//---------------------------------------------------------  
boolean LEDState = false;                                       //Indicate LED on or off
int LEDTimeBlink = 500;                                         //Duration of LED blink
unsigned long LEDTimeOn = 0;                                    //Initialize counter for time LED on
boolean switchEvent = false;                                    //Define default switch status (= disconnected)
const int switchIncrement = 1;                                  //Increment for each switch closure
int switchSum = 0;                                              //Initialize sum of switch closures
boolean LogState = false;                                       //Define trigger to initate logging to data file
int LogInterval = 60;                                           //Define interval for logging to SD card (in seconds)
int LogDelay = 1000;                                            //Define delay to prevent multiple logging events per seconds
unsigned long LastLogTime = 0;                                  //Initialize time since last logging
  
//==========================================================================================


//==========================================================================================
// VOID SETUP
//------------------------------------------------------------------------------------------

void setup() {                                                  //START PROGRAM
  Serial.begin(9600);                                           //Set terminal baud rate to 9600  
  
  Serial.println ("=============================");             //Print program name             
  Serial.println ("ArduLog Tipping Bucket Logger");
  Serial.println ("=============================");             
  
  pinMode(switchPin, INPUT);                                    //Switch pin as input
    digitalWrite(switchPin, HIGH);                              //Activate internal pullup resistor

// INITIALIZE RTC
//---------------------------------------------------------  
  Wire.begin();                                                 //Start I2C communication

  Serial.print ("Program last compiled: ");                     //Print compilation date
  Serial.print (__DATE__);
  Serial.print (", ");
  Serial.println (__TIME__);
  
  Serial.print(F("Initializing RTC: "));
  
  if (!RTC.begin()) {                                           //IF RTC is not found
    Serial.println(F("RTC not found."));                        //Print error message
    while(1);                                                   //And halt program
  }                                                             //End IF    
  Serial.println(F("successful")); 
  
  if (!RTC.isrunning()) {                                       //IF RTC is not running
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));             //Initialize RTC with time of compilation
  }                                                             //End IF    
  
  Serial.print(F("RTC time is set to: "));
  printNowTime();                                               //Print current RTC time
  Serial.println();

// INITIALIZE SD CARD
//---------------------------------------------------------  
  pinMode(SDPin, OUTPUT);                                       //Set SDPin as output
    digitalWrite(SDPin, HIGH);                                  //Activate internal pullup resistor
  Serial.print(F("Initializing SD Card: "));

  if (!SD.begin(SDPin)) {                                       //IF SD is not found
    error("Initializing unsuccessful.");                        //Call error function
  }                                                             //End IF    
  Serial.println("successful.");                                //Otherwise print success
  
// CREATE DATA FILE
//---------------------------------------------------------  
  char filename[] = "LOGGER00.CSV";                             //Create dummy filename 
  for (uint8_t i = 0; i < 100; i++) {                           //FOR 1 to 100
    filename[6] = i/10 + '0';                                   //Create numbered filenames 
    filename[7] = i%10 + '0';                                   //And add NULL as escape character
    
    if (! SD.exists(filename)) {                                //IF filename does not exists  
      file = SD.open(filename, FILE_WRITE);                     //Create and open file 
      break;                                                    //And leave the loop
    }                                                           //End IF 
    
  }                                                             //End FOR

  if (! file) {                                                 //IF file cannot be created, call error function
    error("Could not create data file. Format SD card and try again.");
  }                                                             //End IF 
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  Serial.println("");

// WRITE DATA FILE HEADERS
//---------------------------------------------------------  
  Serial.println("timestamp, tips");                            //write header to serial output
    file.println("timestamp, tips");                            //and data file
  
// BLINK AT START
//---------------------------------------------------------  
  pinMode(LEDPin1, OUTPUT);                                     //LED as output  
  pinMode(LEDPin2, OUTPUT);                                     //LED as output  
  
  for (int i = 1; i < 4; i ++) {                                //FOR 1 to 3
    digitalWrite(LEDPin1, HIGH);                                //Blink LED on/off
    digitalWrite(LEDPin2, HIGH);                                //Blink LED on/off
    delay(LEDTimeBlink);                                        //To indicate start of logging
    digitalWrite(LEDPin1, LOW);
    digitalWrite(LEDPin2, LOW);
    delay(LEDTimeBlink);                                 
  }                                                             //End FOR  
  
}                                                               //End VOID SETUP
//==========================================================================================


//==========================================================================================
// VOID LOOP
//------------------------------------------------------------------------------------------

void loop() {

  digitalWrite(LEDPin1, LEDState);                              //Set LED to LEDState (= on for tip in last 500 ms, off else)
  digitalWrite(LEDPin2, LEDState);                               
  unsigned long DeciSecSinceStart = millis()/100;               //Measure time since program start in deciseconds
  
  if ((unsigned long)(DeciSecSinceStart*100 - LEDTimeOn) >= LEDTimeBlink) {
    LEDState = false;                                           //IF LED blinked for long enough, switch it off
  }                                                             //End IF

  digitalRead(switchPin);                                       //Read switch status
  int switchReading = digitalRead(switchPin);                   //Write switch status to switch Reading variable
  delay(100);                                                   //Wait
  
   if ((switchEvent==false)&&(digitalRead(switchPin)==HIGH)) {  //IF status change from low to high occured
    switchEvent=true;                                           //Set switchEvent TRUE    
  }                                                             //End IF
  
   if ((switchEvent==true)&&(digitalRead(switchPin)==LOW)) {    //IF switch passing detected
      LEDState = true;                                          //Switch LED on
      LEDTimeOn = DeciSecSinceStart*100;                        //And set timer
      switchEvent=false;                                        //Reset switchEvent detector 
      switchSum+=switchIncrement;                               //Increase switch counter
    }                                                           //End IF

  if ((DeciSecSinceStart/10)%LogInterval == 0) {                //If LogInterval is reached
    LogState = true;                                            //Set log flag to TRUE
    
    if ((LogState == true)&&((unsigned long)(DeciSecSinceStart*100 - LastLogTime) <= LogDelay)) { 
      LogState = false;                                         //IF last logging not too long ago set log flag to FALSE
    }                                                           //End IF
    
    if (LogState == true) {                                     //IF log flag is TRUE   
      printNowTime();                                           //Call printNowTime function      
      Serial.print(", ");  
        file.print(", ");        
      Serial.println(switchSum);                                //Print number of switch closures withing logging interval    
        file.println(switchSum); 
      LastLogTime = DeciSecSinceStart*100;                      //Update time of most recent logging event
      LogState = false;                                         //Set log flag to FALSE
      switchSum = 0;                                            //Reset switchSum
    }                                                           //End IF
    
  }                                                             //End IF

  file.flush();                                                 //Close file on SD card
  
}                                                               //End VOID LOOP
//==========================================================================================


//==========================================================================================
// FUNCTION PRINTNOWTIME
//------------------------------------------------------------------------------------------

void printNowTime() {               //To print current RTC time in DD/MM/YYYY HH:MM:SS format
  
  DateTime now = RTC.now();         //Fetch time from RTC

  if (now.day() < 10){              //IF days < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.day(), DEC);     //Print day to serial        
    file.print(now.day(), DEC);     //and data file        
  Serial.print("/");                //Print separator to serial  
    file.print("/");                //and data file 
  if (now.month() < 10){            //IF month < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.month(), DEC);   //Print month to serial   
    file.print(now.month(), DEC);   //and data file  
  Serial.print(F("/"));             //Print separator to serial   
    file.print(F("/"));             //and data file   
  Serial.print(now.year(), DEC);    //Print year to serial    
    file.print(now.year(), DEC);    //and data file  
  Serial.print(F(" "));             //Print separator to serial
    file.print(F(" "));             //and data file
  if (now.hour() < 10){             //IF hour < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.hour(), DEC);    //Print hour to serial
    file.print(now.hour(), DEC);    //and data file
  Serial.print(F(":"));             //Print separator to serial
    file.print(F(":"));             //and data file
  if (now.minute() < 10){           //IF minute < 10
    Serial.print(0);                //Print leading zero to serial
      file.print(0);                //and data file
  }                                 //End IF
  Serial.print(now.minute(), DEC);  //Print minute
    file.print(now.minute(), DEC);  //Print minute
  Serial.print(F(":00"));           //Print separator and seconds
    file.print(F(":00"));           //Print separator and seconds
  
}                                   //End VOID PRINTNOWTIME
//==========================================================================================


//==========================================================================================
// VOID ERROR
//------------------------------------------------------------------------------------------
void error(char const *str) {               //If error function is called
  Serial.print("error: ");                  //Print to terminal: "error:"
  Serial.println(str);                      //Followed by specific error string and
  digitalWrite(LEDPin1, HIGH);              //Blink LED on
  digitalWrite(LEDPin2, HIGH);                                
  while(1);                                 //Pause the program until reset
}                                           //End VOID ERROR
//==========================================================================================
