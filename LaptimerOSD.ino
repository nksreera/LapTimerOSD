#include <SPI.h>
#include <max7456.h>
#include <IRremote.h>

#define MAX_ARRAY_SIZE 5
#define MIN_LAP_TIME 5000 // milliseconds

#define IR_EMITTER_AVAILABLE true
#define RACER_ID 9
#define MAX_BEST_TIME 999999999999999
#define FINISH_LINE_CODE 99


class RollingArray
{
public:
  unsigned int Size;
  unsigned long BestLapTime = MAX_BEST_TIME;
  unsigned long TimeStamps[MAX_ARRAY_SIZE] = {0};
  RollingArray();
  void add(unsigned long t);
  String ToString();
  String BestTimeStr();
  String PrevTimeStr();
  String CurrentTimeStr();
  byte* CurrentTimeByte();
  String formatTime(unsigned long t);
}
;

int RECV_PIN = 9;
int SEND_PIN = 13;
int LED_PIN = 12;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

RollingArray laptimes;


Max7456 osd;
unsigned long counter = 0;
byte tab[]={0xC8,0xC9};

void AddTimestamp(unsigned long t)
{
  laptimes.add(t);
}

void SendIR2Beacon()
{
  // now we send back to the beacon for two seconds

  unsigned long end_time = millis()+2000; // 2 seconds
  while(millis() < end_time)  
  {
    // Keep sending our Code
    if(IR_EMITTER_AVAILABLE)
    {
      irsend.sendSony(RACER_ID,12);
      delayMicroseconds(50);
    }
    else
    {
      // add a simple delay
      delay(100);
    }
  }
}

void handleIRSensor()
{
  unsigned long time;
  if (irrecv.decode(&results)) {
    time = millis();
    AddTimestamp(time);
   
    // SendIR2Beacon();
    
    irrecv.resume(); // Receive the next value
    irrecv.enableIRIn(); // Start the receiver
  }
}

void setup()
{
  SPI.begin();
  
  osd.init(6);
  osd.setDisplayOffsets(60,18);
  osd.setBlinkParams(_8fields, _BT_BT);
 
  osd.activateOSD();
  
  irrecv.enableIRIn(); // Start the receiver
}


unsigned long prevTime = 0;

void loop()
{
  handleIRSensor();
    
  if(prevTime > millis())
  {
    return;
  }
  
  prevTime = millis() + 100;
  
  // Lets not clear the screen
  // osd.clearScreen();
  
    char charBuf[50] = {'\0'};  
  // Show only if data available.
  if(laptimes.Size > 0)
  { 
    String s = laptimes.CurrentTimeStr();
    s.toCharArray(charBuf,50);
    osd.print(charBuf, 0, 11);
  
    s = laptimes.PrevTimeStr();
    s.toCharArray(charBuf,50);
    osd.print(charBuf, 14, 10);
  
    s = laptimes.BestTimeStr();
    s.toCharArray(charBuf,50);
    osd.print(charBuf, 14, 11);
  }
  
  // Voltage
  float v2 = analogRead(2);
  
  v2 *= 5;
  
  float volt = v2/60;
  
  String ss = "";
  ss += volt;
  ss += "v";
  ss.toCharArray(charBuf,50);
  osd.print(charBuf, 0, 1);
}

String RollingArray::formatTime(unsigned long t)
{
  String outStr = "";
  unsigned long seconds = (t/1000)%60;
  unsigned long deci = (t%1000)/10;
  unsigned long minutes = t/60000;
  
  if(minutes <1)
  {
    outStr += "00";
  }
  else if(minutes < 10)
  {
    outStr += "0";
    outStr += minutes;
  }
  else 
  {
    outStr += minutes;
  }
  
  outStr += ":";
  
  if(seconds < 10)
  {
    outStr += "0";
  }
  
  outStr += seconds;
  
  outStr += ":";
  
  if(deci < 1)
  {
    outStr += "00";
  }
  else if(deci < 10)
  {
    outStr += "0";
    outStr += deci;
  }
  else
  {
    outStr += deci;
  }
  
  return outStr;
  
}


/************* Rolling Array ******************/
RollingArray::RollingArray()
{
  Size = 0;
}

void RollingArray::add(unsigned long t)
{
    if(Size > 0)
    {
      // check if this can be added
      if(t - TimeStamps[Size-1] < MIN_LAP_TIME)
      {
        return; // cannot add this 
      }
    }
    
    if(Size >= MAX_ARRAY_SIZE)
    {
      //  we need to re-shuffle
      for(int i=0;i<Size-1;i++)
      {
        TimeStamps[i]=TimeStamps[i+1];
      }
      TimeStamps[Size-1]=t;
    }
    else
    {
      TimeStamps[Size++] = t;
    }
    
    // check for best time
    if(Size > 1)
    {
      // check to see if we have a new best time
      if((TimeStamps[Size-1]-TimeStamps[Size-2]) < BestLapTime)
      {
        BestLapTime = TimeStamps[Size-1]-TimeStamps[Size-2];
      }
    }
}

String RollingArray::BestTimeStr()
{
  String outStr = "";
  if(BestLapTime < MAX_BEST_TIME && Size > 1)
  {
    outStr = "Best ";
    outStr += formatTime(BestLapTime);
  }
  return outStr;
}

String RollingArray::PrevTimeStr()
{
  String outStr = "";
  if(Size > 1)
  {
    outStr = "Prev ";
    unsigned long laptime = TimeStamps[Size-1] - TimeStamps[Size-2];
    outStr += formatTime(laptime);
  }
  return outStr;
}

String RollingArray::ToString()
{
  String outStr = "";
  if(BestLapTime < MAX_BEST_TIME)
  {
    outStr = "Best ";
    unsigned long seconds = BestLapTime/1000;
    unsigned long deci = (BestLapTime%1000)/10;
    outStr += seconds;
    outStr += ":";
    outStr += deci;
  }
  return outStr;
}
String RollingArray::CurrentTimeStr()
{
    String outStr = "";
  if(Size > 0)
  {
    outStr = "Curr ";
    unsigned long laptime = millis() - TimeStamps[Size-1];
    outStr += formatTime(laptime);
  }
  return outStr;
}

byte* RollingArray::CurrentTimeByte()
{
  // curr 00:00
  byte outbyte[5];
  outbyte[0] = 99;
  outbyte[1] = 0x75;
  outbyte[2] = 0x72;
    outbyte[3] = 0x72;
      outbyte[4] = 0x00;
      //outbyte[5] = 
      return outbyte;
}
