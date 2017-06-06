//This version of MotorTest does not use the TLC5974 board.
 
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
  motorPWMPinArray[0]=9; pinMode(motorPWMPinArray[0],OUTPUT);  //The pin on the Arduino that turns on the transistor that turns on the motor.
  motorPWMPinArray[1]=11; pinMode(motorPWMPinArray[1],OUTPUT);
  motorPWMPinArray[2]=2; pinMode(motorPWMPinArray[2],OUTPUT);
  motorLEDPinArray[0]=3; pinMode(motorLEDPinArray[0],OUTPUT);
  motorLEDPinArray[1]=12; pinMode(motorLEDPinArray[1],OUTPUT);
  motorLEDPinArray[2]=10; pinMode(motorLEDPinArray[2],OUTPUT);
  motorSensorPinArray[0]=0;  //Analog in A0
  motorSensorPinArray[1]=1;  //Analog in A1
  motorSensorPinArray[2]=2;  //Analog in A2
  psThresh[0] = 120;//200;  //Motor 1. Threshold for when the light hits the photosensor
  psThresh[1] = 200;  //Motor 2
  psThresh[2] = 120;  //Motor 3
  homeHoleMinIntervalArray_ms[0] = 600; //Motor 1. If we see a hole for this amount of time, call it home.
  homeHoleMinIntervalArray_ms[1] = 575; //Motor 2
  homeHoleMinIntervalArray_ms[2] = 600; //Motor 3
  posHoleMinIntervalArray_ms[0] = 50;  //Motor 1
  posHoleMinIntervalArray_ms[1] = 50;  //Motor 2
  posHoleMinIntervalArray_ms[2] = 50;  //Motor 3
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
    analogWrite(motorLEDPinArray[selectedMotorIndex],255);
    //tlc.setPWM(motorLEDPinArray[selectedMotorIndex],4095);
    //tlc.write();
    
    //Move motor
    analogWrite(motorPWMPinArray[selectedMotorIndex],255);
    //tlc.setPWM(motorPWMPinArray[selectedMotorIndex],4095);
    //tlc.write();
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
   analogWrite(motorPWMPinArray[selectedMotorIndex],0);
   //tlc.setPWM(motorPWMPinArray[selectedMotorIndex],0);
   //tlc.write();
    
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
       analogWrite(motorPWMPinArray[selectedMotorIndex],255);
        //tlc.setPWM(motorPWMPinArray[selectedMotorIndex],4095);
        //tlc.write();
        delay(holePassInterval_ms);  //Move past the current hole.
        int turn=1;
        while(turn==1)
        {
          //read the photosensor value
          int psIn = analogRead(motorSensorPinArray[selectedMotorIndex]);
          Serial.print("Analog value is ");
          Serial.println(psIn);                    
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
              analogWrite(motorPWMPinArray[selectedMotorIndex],0);
              //tlc.setPWM(motorPWMPinArray[selectedMotorIndex],0);
              //tlc.write();
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
   analogWrite(motorLEDPinArray[selectedMotorIndex],0);
   // tlc.setPWM(motorLEDPinArray[selectedMotorIndex],0);
   // tlc.write();
    
    Serial.println("Select motor: (1,2, or 3)");
  }
  
} 
