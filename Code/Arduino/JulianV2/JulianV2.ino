//This is version 2 of the Julian painting.
//TLC5947 has been removed. The 
#include <SD.h>  //SD card
#include <SPI.h>              //display
#include <Adafruit_GFX.h>      //display
#include <Adafruit_PCD8544.h>  //display
#include <Wire.h>    //clock
#include "RTClib.h"  //clock

//Display start
// Software SPI (slower updates, more flexible pin options):
// pin 8 - Serial clock out (SCLK)
// pin 7 - Serial data out (DIN)
// pin 6 - Data/Command select (D/C)
// LCD chip select (CS)  (Connected to ground in the display)
// pin 5 - LCD reset (RST)
//Adafruit_PCD8544(int8_t SCLK, int8_t DIN, int8_t DC, int8_t RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 8, 6, 5);

//#define LOGO16_GLCD_HEIGHT 16
//#define LOGO16_GLCD_WIDTH  16

//static const unsigned char PROGMEM logo16_glcd_bmp[] =
//{ B00000000, B11000000,
//  B00000001, B11000000,
//  B00000001, B11000000,
//  B00000011, B11100000,
//  B11110011, B11100000,
//  B11111110, B11111000,
//  B01111110, B11111111,
//  B00110011, B10011111,
//  B00011111, B11111100,
//  B00001101, B01110000,
//  B00011011, B10100000,
//  B00111111, B11100000,
//  B00111111, B11110000,
//  B01111100, B11110000,
//  B01110000, B01110000,
//  B00000000, B00110000 };

//Display end

//Clock start
//A4 - SDA
//A5 - SCL
RTC_DS1307 rtc;
int currentMinute;  //used to keep track of when the minute changes
int currentDayIndex;   //used to keep track of when the day changes
DateTime currentDT;
char currentMode;   //indicates if we are running in regular mode or timewarp mode
int timeWarpIndex;  //used to warp time
//Clock end
//SD Card start
File myFile;
//SD Card end
//The max length for a special message
#define SPEC_MSG_MAX_LEN 24
#define MAX_POS_ENTRY 5  //The max position entries for a motor in one day

//Holds the position and timing for a motor position
typedef struct
{
  int motorIndex;
  int posIndex;
  int startHour;
  int startMinute;
} MotorPosEntry;

void GetDayData(int dayIndex);
MotorPosEntry DecodeMotorPosString(char motorPosStr[]);
void PrintCurrDayData();

//Store all data for a day such as the motor position schedules,
//special messages, and graphics
struct DayData
{
  MotorPosEntry motPosScheduleArray[3][MAX_POS_ENTRY];  
  char specialMessage[SPEC_MSG_MAX_LEN];     // a string to hold the special message
  int specialMsgLen;                        //the length of the current special message
  int dayIndex;
};

//Everything you need to know about the current day.
//This data will be read from the SD card once per day.
DayData currDayData;

int motorPWMPinArray[3];
int motorLEDPinArray[3];
int motorSensorPinArray[3];
int psThresh[3];     //Threshold for when the light hits the photosensor
int homeHoleMinIntervalArray_ms[3];  //The min time the home hole should be seen
int posHoleMinInterval_ms = 50;     //The min time a position hole should be seen
int currMotorPosArray[3];    //Store the current motor positions
//The amount of time we move the motor past a detected hole before looking for the next hole.
//This will prevent seeing the same hole again as we start to move past it. 
int holePassInterval_ms = 50;

//***********************************************************
//***********************SETUP*******************************
//***********************************************************
void setup()
{
  Serial.begin(9600);  
  motorPWMPinArray[0]=9; pinMode(motorPWMPinArray[0],OUTPUT);  //The pin on the Arduino that turns on the transistor that turns on the motor.
  motorPWMPinArray[1]=11; pinMode(motorPWMPinArray[1],OUTPUT);
  motorPWMPinArray[2]=2; pinMode(motorPWMPinArray[2],OUTPUT);
  motorLEDPinArray[0]=3; pinMode(motorLEDPinArray[0],OUTPUT);  //The pin on the Arduino that turns on the LED
  motorLEDPinArray[1]=12; pinMode(motorLEDPinArray[1],OUTPUT);
  motorLEDPinArray[2]=10; pinMode(motorLEDPinArray[2],OUTPUT);
  motorSensorPinArray[0]=0;  //Analog in A0
  motorSensorPinArray[1]=1;  //Analog in A1
  motorSensorPinArray[2]=2;  //Analog in A2
  psThresh[0] = 120;  //Motor 1. Threshold for when the light hits the photosensor
  psThresh[1] = 200;  //Motor 2
  psThresh[2] = 120;  //Motor 3
  homeHoleMinIntervalArray_ms[0] = 600;  //Motor 1. Threshold for the length of time we see the home hole.
  homeHoleMinIntervalArray_ms[1] = 575;
  homeHoleMinIntervalArray_ms[2] = 600;
  for(int i=0;i<3;i++)
    currMotorPosArray[i] = -1;   //Initially there is no position value
     
  //Clock stup begin
  #ifdef AVR
    Wire.begin();
  #else
    Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif
    rtc.begin();

  if (! rtc.isrunning())
    Serial.println("RTC is NOT running!");
  currentDayIndex = -1;
  currentMinute = -1;
  //clock setup end
  //SD card setup begin
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
   pinMode(53, OUTPUT);
 
  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  //SD card setup end 
 //Display setup begin
  display.begin();
  // Set the contrast to adapt the display for the best viewing.
  display.setContrast(50);
  //Display setup end
 //Welcome message
  display.clearDisplay();   // clears the screen and buffer
  display.setCursor(0,0);
  display.println("Begin launch sequence"); 
  display.display();
  delay(1000);
 
  currentMode='r'; //regular mode
  timeWarpIndex=0;  
}

void loop()
{    
  if (Serial.available())
  {    
    int inVal = Serial.parseInt();
    Serial.print("Getting data for day: ");
    Serial.println(inVal);
    GetDayData(inVal);
    PrintCurrDayData();
  }
  
  //Serial.println(freeRam()); //test
  //delay(500);
  
  //Check for serial input. The only commands that can come from serial
  //are to start/stop time warp mode.
  char inChar = Serial.read();
  if(inChar == 'w')
  {
    currentMode = 'w';
    timeWarpIndex=0;
    Serial.println("Time Warp!");
  }
  else if(inChar == 'r')
  {
    currentMode = 'r';
    Serial.println("Regular Time");
  }
  
  //Update the time
  if(currentMode=='r')
  {
    currentDT = rtc.now();
  }
  else if(currentMode=='w')
  {
     //currentDT = currentDT + 3600*timeWarpIndex;  //add an hour each time
     currentDT = currentDT + TimeSpan(0, 1, 0, 0);  //add an hour each time
     
     timeWarpIndex++;
     if(timeWarpIndex>=8760)  //8760 hours in a year
       currentMode='r';
  }
   
  //Check if the minute value has updated
  //This is used to update the time
  if(currentDT.minute() != currentMinute || currentMode=='w')
  {
    currentMinute = currentDT.minute();
    //****************Display stuff begin*************************
    display.clearDisplay();   // clears the screen and buffer
    display.setCursor(0,0);
    //Display the date and time
    DisplayDateAndTime();
    //Show the message for the day
    display.println(currDayData.specialMessage);
    //Check to see if we have reached a new day
    int newDayIndex = GetDayIndex(currentDT);
    if(newDayIndex!=currentDayIndex)
    {
       currentDayIndex = newDayIndex; 
       Serial.print("Current day index:");
       Serial.println(currentDayIndex);
       //Load the new day's data
      GetDayData(currentDayIndex);
      //PrintCurrDayData(); //testing      
      //Force the motors to reposition at the beginning of every day.
      for(int i=0;i<3;i++)
        currMotorPosArray[i] = -1;
    }
    
    display.display();
    //****************Display stuff end*************************
    //***********Motor update stuff begin***********************
    UpdateMotorPositions();
    //***********Motor update stuff end***********************
    //Testing begin
    Serial.print(currentDT.year(), DEC);
    Serial.print('/');
    Serial.print(currentDT.month(), DEC);
    Serial.print('/');
    Serial.print(currentDT.day(), DEC);
    Serial.print(' ');
    Serial.print(currentDT.hour(), DEC);
    Serial.print(':');
    Serial.print(currentDT.minute(), DEC);
    Serial.print(':');
    Serial.print(currentDT.second(), DEC);
    Serial.println();
    //Testing end
  
    delay(1000);
  }
  
  
  
  
  // text display tests
//  display.setTextSize(1);
//  display.setTextColor(BLACK);
//  display.setCursor(0,0);
//  display.println("Hello, Courtney!");
//  display.setTextColor(WHITE, BLACK); // 'inverted' text
//  display.println("How much text can we fit on the screen? Is this too much? Or can we fit more?");
//  display.display();
//  delay(20000);
//  
//  // miniature bitmap display
//  display.clearDisplay();
//  display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
//  display.display();

  delay(1000);
}

//Display the date and time
void DisplayDateAndTime()
{
  display.print(currentDT.month());
  display.print('/');
  display.print(currentDT.day());
  display.print('/');
  display.println(currentDT.year());
  
  display.print(currentDT.hour());
  display.print(':');
  display.print(currentDT.minute());
  display.print(':');
  display.println(currentDT.second());
}

//Check to see if the motor positions need to be updated.
//
void UpdateMotorPositions()
{
  Serial.println("UpdateMotorPositions");
  
  //Loop through each motor
  for(int motInd=0;motInd<3;motInd++)
  {
    Serial.print("motInd");
    Serial.println(motInd);
    int newMotorPos=-1;
    //Compare the current time to the motor position entries
    //Loop through each motor position entry
    for(int motPosInd=0;motPosInd<MAX_POS_ENTRY;motPosInd++)
    {
      //Get the data for the current entry
      int currEntryPos = currDayData.motPosScheduleArray[motInd][motPosInd].posIndex;
      int currEntryStartHour = currDayData.motPosScheduleArray[motInd][motPosInd].startHour;
      int currEntryStartMin = currDayData.motPosScheduleArray[motInd][motPosInd].startMinute;
      Serial.print("currEntryPos: ");
      Serial.println(currEntryPos);
      Serial.print("currEntryStartHour: ");
      Serial.println(currEntryStartHour);
      Serial.print("currEntryStartMin: ");
      Serial.println(currEntryStartMin);
      Serial.print("currentDT.hour(): ");
      Serial.println(currentDT.hour());
      Serial.print("currentDT.minute(): ");
      Serial.println(currentDT.minute());
      //If the position is equal to -1 then there is no position data stored here.
      if(currEntryPos > -1)
      {
        //Check if the current time is later than the start time for this position entry
        if(currentDT.hour()>=currEntryStartHour ||  (currentDT.hour()==currEntryStartHour && currentDT.minute()>=currEntryStartMin))
          newMotorPos = currEntryPos;
      }
    }
    Serial.print("newMotorPos: ");
      Serial.println(newMotorPos);
      Serial.print("currMotorPosArray[motInd]: ");
      Serial.println(currMotorPosArray[motInd]);
    //Check to see if we found a valid motor position
    //and that this position is different than the current position.
    if(newMotorPos>=0 && newMotorPos!=currMotorPosArray[motInd])
    {
      SetMotorPosition(motInd, newMotorPos);
      currMotorPosArray[motInd]=newMotorPos;
    }
  }
}

void GetDayData(int dayIndex)
{
  //clear the current day data
  for(int motInd=0;motInd<3;motInd++)
    for(int i=0;i<MAX_POS_ENTRY;i++)
      currDayData.motPosScheduleArray[motInd][i].posIndex = -1;
 
  // open the file for reading:
  myFile = SD.open("JULIAN.TXT");
  if (myFile) 
  { 
     Serial.println("Begin reading file.");
    // read from the file until there's nothing else in it:
    int currEntryIndex=0;
    char currEntry[SPEC_MSG_MAX_LEN];  //The longest entry will be the daily message
    //int m1PosEntryIndex=0;  //Keeps track of where to put the next position data that is read from the file. 
    //int m2PosEntryIndex=0;
    //int m3PosEntryIndex=0;
    int posEntryIndexArray[]={0,0,0};  //Keeps track of where to put the next position data that is read from the file.
    bool dayIndexFound = false;
    while (myFile.available()) 
    { 
      char inChar = myFile.read();
      int currEntryLen=0;
      //Read until a comma or newline is reached
      while(inChar!=',' && inChar!='\n')
      {
        currEntry[currEntryLen] = inChar;
        currEntryLen++;
        inChar = myFile.read();
        if(currEntryLen==SPEC_MSG_MAX_LEN)
          break;
      }
      currEntry[currEntryLen] = '\0';  //Null terminate
      Serial.print("Current entry: ");
      Serial.println(currEntry);
            
      if(currEntryIndex==0) //Day Index
      {
        char * pEnd;
        int currRowDayIndex = strtol(currEntry,&pEnd,10);
        if(dayIndex!=currRowDayIndex)
        {
          //This is not the day we are looking for.
          //Advance to the end of the row          
          while(inChar!='\n')
            inChar = myFile.read();
          //Move to the next row.
          continue;
        }
        else
        {
          currDayData.dayIndex = currRowDayIndex;
          currEntryIndex++;   
           dayIndexFound=true;       
        }
      }
      else if(currEntryIndex==1) //Daily message
      {
        if(currEntryLen>0)
          strncpy(currDayData.specialMessage, currEntry, currEntryLen);
        else
          currDayData.specialMessage[0] = '\0';  
        currEntryIndex++;
      }
      else if(currEntryIndex>1)  //Motor position data or Lights data
      {
         if( currEntry[0]=='M')
         {
           //Get the data for this motor position entry
           MotorPosEntry newMotorPosEntry = DecodeMotorPosString(currEntry);
           //Find which motor this entry is for and check that we have space to
           //add it to the list.
           int motInd = newMotorPosEntry.motorIndex-1;
           if(posEntryIndexArray[motInd]<MAX_POS_ENTRY)
           {
             currDayData.motPosScheduleArray[motInd][posEntryIndexArray[motInd]] = newMotorPosEntry;
             posEntryIndexArray[motInd]++;
           } 
         }
         currEntryIndex++;
      }  //else if(currEntryIndex>1)
      //If we have already found the day entry and we reach a newline then we are done with the datafile.
      if(inChar=='\n' && dayIndexFound==true)
        break;      
    }
    // close the file:
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println("error opening JULIAN.TXT");
  }
}

MotorPosEntry DecodeMotorPosString(char motorPosStr[])
{  
  MotorPosEntry newPosEntry;
  //Extract the motor index value
  char indexStr[2];
  strncpy(indexStr, motorPosStr+1, 1);
  indexStr[1] = '\0'; // place the null terminator
  Serial.print("DecodeMotorPosString: index string=");
  Serial.println(indexStr);
  char * pEnd;
  newPosEntry.motorIndex = strtol(indexStr,&pEnd,10);
  //Extract the position value
  char posStr[2];
  strncpy(posStr, motorPosStr+2, 1);
  posStr[1] = '\0'; // place the null terminator
  Serial.print("DecodeMotorPosString: pos string=");
  Serial.println(posStr);
  newPosEntry.posIndex = strtol(posStr,&pEnd,10);
  //Extract the start hour
  char hsStr[3];
  strncpy(hsStr, motorPosStr+3, 2);
  hsStr[2] = '\0'; // place the null terminator
  newPosEntry.startHour = strtol(hsStr,&pEnd,10);
  //Extract the start minute
  char msStr[3];
  strncpy(msStr, motorPosStr+5, 2);
  msStr[2] = '\0'; // place the null terminator
  newPosEntry.startMinute = strtol(msStr,&pEnd,10);
  
  return newPosEntry;
}

//Set the motor to a certain position
//Position 1 = home position
void SetMotorPosition(int motorIndex, int motorPosition)
{
  //Debugging begin
  Serial.print("Moving motor: ");
  Serial.println(motorIndex);
  Serial.print("to position: ");
  Serial.println(motorPosition);
  delay(2000);
  //debugging end
  //Turn on LED
  analogWrite(motorLEDPinArray[motorIndex],255);
    
  //Move motor
  analogWrite(motorPWMPinArray[motorIndex],255);
  //In case we are on a hole now, allow the motor to keep moving until it goes past the current hole.
  delay(holePassInterval_ms);
  //Find home position
  //Home position is indicated by a large hole in the indexer,
  //The home hole should be much larger than any of the position holes.
  boolean homeFound = false;
  unsigned long firstHoleTime; //the time when the hole was first seen
  boolean isOverHole = false; //true if we are currently over a hole
  while(homeFound==false)
  {
    //read the photosensor value
    int psIn = analogRead(motorSensorPinArray[motorIndex]);
    //Serial.print("Analog value is ");
    //Serial.println(psIn);
    if(psIn>=psThresh[motorIndex] && isOverHole==false)
    {
      isOverHole = true;
      // store the time since the Arduino started running in a variable 
      firstHoleTime = millis();
    }
    else if(psIn<psThresh[motorIndex] && isOverHole==true)
    {
      unsigned long currentTime = millis();
      if(currentTime-firstHoleTime>homeHoleMinIntervalArray_ms[motorIndex])
      {
        homeFound=true;
        Serial.println("Reached home");
        Serial.print("Hole interval = ");
        Serial.println(currentTime-firstHoleTime);
                  
      }
      else
      {
        //This is a position hole. It is not HomeHole.
        Serial.print("Hole interval = ");
        Serial.println(currentTime-firstHoleTime);
        isOverHole=false;
      }
    }
  }
  
 //stop motor
 analogWrite(motorPWMPinArray[motorIndex],0);
  
  //Move to the desired position
  Serial.print("Moving to position = ");
  Serial.println(motorPosition);
  
 if(motorPosition>1)    //Position 1 = home
 {
   int currPos=1;
   while(currPos<motorPosition)
   {
     isOverHole = false; //true if we are currently over a hole
     //Move motor
     analogWrite(motorPWMPinArray[motorIndex],255);
      delay(holePassInterval_ms);  //Move past the current hole.
      int turn=1;
      while(turn==1)
      {
        //read the photosensor value
        int psIn = analogRead(motorSensorPinArray[motorIndex]);
        //Serial.print("Analog value is ");
        //Serial.println(psIn);                    
        if(psIn>=psThresh[motorIndex] && isOverHole==false)  //A new hole is found
        {
          Serial.println("Hole started");   
          isOverHole = true;
          // store the time since the Arduino started running in a variable 
          firstHoleTime = millis();
        }
        else if(psIn<psThresh[motorIndex] && isOverHole==true)  //We reached the end of a hole
        {
          unsigned long currentTime = millis();
          if(currentTime-firstHoleTime>posHoleMinInterval_ms)
          {
            turn=0;
            //Stop motor
            analogWrite(motorPWMPinArray[motorIndex],0);
            currPos++;
          
            Serial.println("Reached a position hole");
            Serial.print("Current position:");
            Serial.println(currPos);
            Serial.print("Hole interval = ");
            Serial.println(currentTime-firstHoleTime);                
          }
          else
          {
            //This is a blip. It is not a position hole.
            Serial.println("Warning: Blip found!");
            Serial.print("Hole interval = ");
            Serial.println(currentTime-firstHoleTime);              
          }
          isOverHole=false;
        }
      }        
   }
 }
 //Turn off LED
 analogWrite(motorLEDPinArray[motorIndex],0);
}

//test function
int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void PrintCurrDayData()
{
   Serial.print("Day index: ");
   Serial.println(currDayData.dayIndex);
   Serial.print("Special message: ");
   Serial.println(currDayData.specialMessage);
   for(int motInd=0;motInd<3;motInd++)
   {
     Serial.print("Motor 1: ");
     Serial.print(motInd+1);
     Serial.println(": ");
      for(int i=0;i<MAX_POS_ENTRY;i++)
      {
        if(currDayData.motPosScheduleArray[motInd][i].posIndex>=0)
        {
          Serial.print("Position entry: ");
          Serial.println(i);
          Serial.print("Position value: ");
          Serial.println(currDayData.motPosScheduleArray[motInd][i].posIndex);
          Serial.print("Start hour: ");
          Serial.println(currDayData.motPosScheduleArray[motInd][i].startHour);
          Serial.print("Start minute: ");
          Serial.println(currDayData.motPosScheduleArray[motInd][i].startMinute);
        }
      }   
   } 
}

//Get the day of the year from 1-365.
int GetDayIndex(DateTime currTime)
{
  //Create a daytime for Jan 1st
  DateTime yearBegin(currTime.year(), 0, 0);
  //Get the number of days since the beginning of the year
  TimeSpan ts = currTime - yearBegin;
  
  return ts.days();
}
