#include "Adafruit_TLC5947.h"    //PWM

//PWM
// How many boards do you have chained?
#define NUM_TLC5974 1
#define data   2
#define clock   3
#define latch   4
#define oe  -1  // set to -1 to not use the enable pin (its optional)
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5974, clock, data, latch);
 
int motorPWMPinArray[3];
int motorLEDPinArray[3];
int motorSensorPinArray[3];
int psThresh[3];     //Threshold for when the light hits the photosensor
int homeHoleMinIntervalArray_ms[3];  //The min time the home hole should be seen
int posHoleMinIntervalArray_ms[3];     ////The min time a position hole should be seen
//The amount of time we move the motor past a detected hole before looking for the next hole.
//This will prevent seeing the same hole again as we start to move past it. 
int holePassInterval_ms = 50;  
bool isMotorSelected = false;
int selectedMotorIndex = -1;
 
void setup() 
{ 
  //PWM setup begin
  tlc.begin();
  if (oe >= 0) {
    pinMode(oe, OUTPUT);
    digitalWrite(oe, LOW);
  }
  for(int i=0;i<24;i++)
    tlc.setPWM(i,0);
  tlc.write();
  //PWM setup end
  
  motorPWMPinArray[0]=0;  //The channel on the PWM breakout board
  motorPWMPinArray[1]=1;
  motorPWMPinArray[2]=2;
  motorLEDPinArray[0]=4;
  motorLEDPinArray[1]=5;
  motorLEDPinArray[2]=6;
  motorSensorPinArray[0]=0;  //Analog in A0
  motorSensorPinArray[1]=1;  //Analog in A1
  motorSensorPinArray[2]=2;  //Analog in A2
  psThresh[0] = 450;//200;  //Motor 1. Threshold for when the light hits the photosensor
  psThresh[1] = 200;  //Motor 2
  psThresh[2] = 200;  //Motor 3
  homeHoleMinIntervalArray_ms[0] = 1000; //Motor 1. If we see a hole for this amount of time, call it home.
  homeHoleMinIntervalArray_ms[1] = 700; //Motor 2
  homeHoleMinIntervalArray_ms[2] = 1000; //Motor 3
  posHoleMinIntervalArray_ms[0] = 75;  //Motor 1
  posHoleMinIntervalArray_ms[1] = 75;  //Motor 2
  posHoleMinIntervalArray_ms[2] = 75;  //Motor 3
  Serial.begin(9600);
  while (! Serial);
  Serial.println("Select motor: (1,2, or 3)");
} 
 
 
void loop() 
{ 
  int moveToPosition = -1;
  //NOTE: Use "No line ending" option when using
  //the serial monitor to send commands.
  if (Serial.available())
  {
    int inVal = Serial.parseInt();
    Serial.print("inVal: ");
    Serial.println(inVal);
    if (inVal >= 0)
    {
      if(isMotorSelected==false)
      {
         isMotorSelected = true;
         selectedMotorIndex=inVal-1;
         Serial.println("Enter desired motor position:");
      }
      else
      {
        moveToPosition = inVal;
        isMotorSelected=false;
      }
    }
  }
  
  if(moveToPosition >= 0)
  {
    Serial.print("Moving to position ");
    Serial.println(moveToPosition);
    //Turn on LED
    tlc.setPWM(motorLEDPinArray[selectedMotorIndex],4095);
    tlc.write();
    
    //Move motor
    tlc.setPWM(motorPWMPinArray[selectedMotorIndex],4095);
    tlc.write();
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
      int psIn = analogRead(motorSensorPinArray[selectedMotorIndex]);
      Serial.print("Analog value is ");
      Serial.println(psIn);
      if(psIn>=psThresh[selectedMotorIndex] && isOverHole==false)
      {
        isOverHole = true;
        // store the time since the Arduino started running in a variable 
        firstHoleTime = millis();
      }
      else if(psIn<psThresh[selectedMotorIndex] && isOverHole==true)
      {
        unsigned long currentTime = millis();
        Serial.print("Detected interval = ");
        Serial.println(currentTime-firstHoleTime);
        if(currentTime-firstHoleTime>homeHoleMinIntervalArray_ms[selectedMotorIndex])
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
   tlc.setPWM(motorPWMPinArray[selectedMotorIndex],0);
   tlc.write();
    
    //Move to the desired position
    Serial.print("Moving to position = ");
    Serial.println(moveToPosition);
    
   if(moveToPosition>0)
   {
     int currPos=0;
     while(currPos<moveToPosition)
     {
       isOverHole = false; //true if we are currently over a hole
       //Move motor
        tlc.setPWM(motorPWMPinArray[selectedMotorIndex],4095);
        tlc.write();
        delay(holePassInterval_ms);  //Move past the current hole.
        int turn=1;
        while(turn==1)
        {
          //read the photosensor value
          int psIn = analogRead(motorSensorPinArray[selectedMotorIndex]);
          //Serial.print("Analog value is ");
          //Serial.println(psIn);                    
          if(psIn>=psThresh[selectedMotorIndex] && isOverHole==false)  //A new hole is found
          {
            Serial.println("Hole started");   
            isOverHole = true;
            // store the time since the Arduino started running in a variable 
            firstHoleTime = millis();
          }
          else if(psIn<psThresh[selectedMotorIndex] && isOverHole==true)  //We reached the end of a hole
          {
            unsigned long currentTime = millis();
            if(currentTime-firstHoleTime>posHoleMinIntervalArray_ms[selectedMotorIndex])
            {
              turn=0;
              //Stop motor
              tlc.setPWM(motorPWMPinArray[selectedMotorIndex],0);
              tlc.write();
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
    tlc.setPWM(motorLEDPinArray[selectedMotorIndex],0);
    tlc.write();
    
    Serial.println("Select motor: (1,2, or 3)");
  }
  
} 
