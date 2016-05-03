// We love our ET-312b box, but the random modes don't quite do
// what we want.  So let's hook up an arduino via a serial converter
// and have it do some fun things.
//
// We connect to the box.
//
// 1. We wait for some random time between 30 seconds and 3 minutes.
// 2. We mute the outputs
// 3. We wait for some random time between 5 seconds and 2 minutes.
// 4. We unmute the outputs, picking a random mode from our list of favourites
//
// The idea being that the person in control can play with the A/B/MA knobs,
// but they don't have to bother changing the pattern.  This is better than
// "random1" since it includes just our favourite routines, and includes some
// annoying "downtime".  It's better than "torment" since when it's on it is on
// for a while.
//
// 2nd May 2016

#include <EEPROM.h>  // for storing the mod byte
#include <Venerate.h> // This is our own ET312 communication library 

// It's best to use hardware serial ports which limits you to Leonardo
// and clones.  However it does also work with the AltSoftSerial library
// but you are limited to what pins you can use.
// http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html 
//
// #include <AltSoftSerial.h>
// AltSoftSerial SS2; 

unsigned long starttime;

Venerate EBOX = Venerate(0);

void setup()
{
  starttime = millis();

  Serial1.begin(19200);
  EBOX.begin(Serial1);
  Serial.begin(19200);

  EBOX.setdebug(Serial, 0);
}

// The main loop; if we're not already connected to a box, try to
// connect to a box.  

byte nowtime = 0;

void set_mode(int p) {
  EBOX.setbyte(ETMEM_mode, p-1);
  EBOX.setbyte(ETMEM_pushbutton, ETBUTTON_setmode);
  delay(180);
  EBOX.setbyte(ETMEM_pushbutton, ETBUTTON_lockmode);
  delay(180);
}

void set_mute() {
   EBOX.setbyte(ETMEM_pushbutton,0x18); 
}

int state = 0;
unsigned long future;
int modepick[] = { ETMODE_waves, ETMODE_stroke, ETMODE_rhythm, ETMODE_climb, ETMODE_orgasm, ETMODE_phase2, ETMODE_random2, ETMODE_phase2, ETMODE_random2, ETMODE_stroke,ETMODE_phase2 };
int nummodes = 11;

void loop()
{  
  if (EBOX.isconnected()) {
    
// if state = 0 set timer, state = 1
// state = 1 wait for 30 seconds to 2 minutes
// mute the box
// state = 2 wait for 5 seconds to 1 minute
// change mode to random,  set state to 0
        
    if (state == 0) {
      future = millis()+(1000*random(30,240));
      state = 1; 
    } else if (state == 1) {
      if (millis() > future) {
        set_mute();
        future = millis()+(1000*random(5,120));
        state = 2;
      }
    } else if (state == 2) {      
      if (millis() > future) {
        set_mode(modepick[random(0,nummodes)]);
        state = 0;
      }
    }
  
  } else {
  // Try to connect every couple of seconds, don't connect until first second
        if ((millis() - starttime) > 1500) {
           starttime = millis();
           digitalWrite(13,HIGH);
           EBOX.hello();
           digitalWrite(13,LOW);                          
        }
        nowtime++;    
        delay(100);        
  }
}
