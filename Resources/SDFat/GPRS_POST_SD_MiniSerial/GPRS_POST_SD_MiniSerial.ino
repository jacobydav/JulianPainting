#include <SdFat.h>
#include <SdFatUtil.h>
#ifndef UDR0
#error no AVR serial port0
#endif

#include <GSM.h>
#include <Adafruit_VC0706.h>
#include <SoftwareSerial.h> 


#define PINNUMBER "9509" // PIN number if necessary
#define GPRS_APN       "orangeworld" // replace with your GPRS APN
#define GPRS_LOGIN     "orange"    // replace with your GPRS login
#define GPRS_PASSWORD  "orange" // replace with your GPRS password

GSMClient client;
GPRS gprs;
GSM GSMAccess; // include a 'true' parameter for debug enabled

#define requestInterval  30000    // delay between requests; 30 seconds
//#define  DELIMITER  0x1E
#define  SEMICOLON  0x3B //';'
#define  NEWLINE  0x0D // char to byte using ther ASCII table

unsigned long lastAttemptTime = 0;     // last time you connected to the server, in milliseconds

boolean readingCmd = false;          // if you're currently reading the cmd

uint8_t motor_time = 0; //0-9 secs
uint8_t motor_direction = 0; //1 forward, 2 backward, 3 left, 4 rigth 
uint8_t separator_rcvd = 0;
uint8_t complete_cmd = 0;
   
SdFat sd;
SdFile myFile;

void setup()
{
  pinMode(10, OUTPUT);

  MiniSerial.begin(9600);

  MiniSerial.print(F("Start SRAM="));  MiniSerial.println(freeRam());
  
  // Start GSM shield
  connectGSM();


  //MiniSerial.print("SRAM=");  MiniSerial.println(freeRam());
  delay(100);
  connectToServer();
  //MiniSerial.print("SRAM=");  MiniSerial.println(freeRam());
}



void loop()
{

  if (client.connected() || client.available())
  {
    if (client.available())
    {
      // read incoming bytes:
      uint8_t inChar = client.read();
      //MiniSerial.print(inChar);
      if(readingCmd)
      {
        if (inChar == 0x1E )
        {
            // if you got a DELIMITER character,
            // you've reached the end of the cmd:
            readingCmd = false;
            MiniSerial.println(F("Cmd rcvd"));
            //MiniSerial.println(cmd);   
            // close the connection to the server:
            client.stop(); 
          }
          else
          {
            if(inChar==SEMICOLON)
            {
              separator_rcvd = true;
            }
            else
            {
              if(separator_rcvd)
              {
                motor_time = (char) inChar; //this is the second number, the time
                complete_cmd = 1;
                MiniSerial.print(F("Cmd TIME: ")); MiniSerial.println(motor_time);
              }
              else
              {
                motor_direction = (char) inChar; //this is the second number, the time
                MiniSerial.print(F("Cmd DIRECTION: ")); MiniSerial.println(motor_direction);                
              }
            }
            //MiniSerial.println((char)inChar);
          }
      }
      else
      {
        if (inChar == 0x1E )
        {
          readingCmd = true; 
          MiniSerial.println(F("Cmd Start "));
          motor_time = 0;
          motor_direction = 0;
          separator_rcvd = 0;
          complete_cmd = 0;
        }
        else
        {
          //MiniSerial.print(inChar);
        }      
      }
    }   
  }
  else if (millis() - lastAttemptTime > requestInterval)
  {
    // if you're not connected, and two minutes have passed since
    // your last connection, then attempt to connect again:
    connectToServer();
  }
  else
  {

    if(complete_cmd)
    {
      //MiniSerial.println(F("Exec Cmd"));
      complete_cmd = false;
    }
    else
    {
        MiniSerial.println(F("."));
        delay(1000);
    }

  }
}

/*
  Connect to Server
*/
void connectToServer()
{
  MiniSerial.println(F("try TCP"));
  if(gprs.ready())
  {
      // attempt to connect, and wait a millisecond:
      if (client.connect("fliperbaker-001-site1.smarterasp.net", 80))
      {
        delay(100);
        MiniSerial.println(F("POST"));
        // make HTTP GET request to twitter:
        client.println(F("POST /TCPEndPoint.ashx HTTP/1.0"));
        //client.print(path);
        //client.println(F(" HTTP/1.0")); 
        //client.println("Content-Type: text/plain"); MiniSerial.println("Content-Type: text/plain");
        //client.print(F("Host: ")); client.println(server); 
        client.println(F("Host: fliperbaker-001-site1.smarterasp.net"));
        client.print(F("Content-Length: "));
        //client.println(F("0")); 
        //client.println();         
        //need to know the size of the file
       //open the file 
        
        
        MiniSerial.print(F("SRAM="));  MiniSerial.println(freeRam());
        
        
        
        if (sd.begin(8, SPI_HALF_SPEED))
        {
          //MiniSerial.println(F("SD init 1"));
          
          if (myFile.open("IMAGE08.JPG", O_READ)) {
            //MiniSerial.println("file opened"); 
            long fileSize = myFile.fileSize();
            MiniSerial.print(F("file size: "));MiniSerial.print(fileSize);
            client.println(fileSize); 
            client.println(); 
            
            int data;
            while ((data = myFile.read()) >= 0) client.write(data);
  
            // close the file:
            myFile.close(); 
          }
          else
          {
            MiniSerial.println(F("File NOT opened")); 
          }

        } 
        else
        {
          MiniSerial.println(F("SD init ERROR"));
          client.println(F("0")); 
          client.println(); 
        }
        
      }
      else
      {
        MiniSerial.println(F("no TCP"));
      }
  }
  else
  {
    MiniSerial.println(F("GPRS is OFF"));
    connectGSM();
  }
  lastAttemptTime = millis();
}
void connectGSM()
{
    uint8_t notConnected = true;
    while(notConnected)
    {
      if((GSMAccess.begin(PINNUMBER)==GSM_READY) &
          (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)==GPRS_READY))
        notConnected = false;
      else
      {
        //MiniSerial.println(F("GPRS OFF"));
        delay(1000);
      }
    }   
    MiniSerial.println(F("GPRS ON"));
    client.setTimeout(2000);
}


int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);  
}

