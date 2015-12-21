// Bargraph LED display for an ET312 e-stim box 
//
// We love our ET312 box, but the little red LED on the front isn't
// enough to really know where in the cycle the user is we really want to
// be able to display the state, based on the level and probably
// frequency/width.  Then someone controlling the box will know exactly
// when it's at the peak.
//
// You can use an Arduino, a serial converter, and a string of ws2812b
// 'neopixels' to give a real-time view of what's going on.
//
// June 2015

#include <WS2812.h>  // from https://github.com/cpldcpu/light_ws2812 o
// we don't use the adafruit neopixel library to avoid interrupts and mess with serial port
#include <EEPROM.h>  // for storing the mod byte
#include <Venerate.h> // This is our own ET312 communication library 

// It's best to use hardware serial ports which limits you to Leonardo
// and clones.  However it does also work with the AltSoftSerial library
// but you are limited to what pins you can use.
// http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html 
//
// #include <AltSoftSerial.h>
// AltSoftSerial SS2; 

#define board_prototype_1

#ifdef board_prototype_1
// Board 1 is a leonardo like, a dfrobot beetle with 2 strings of LEDs
// 60/metre .5 metre each string connected to separate pins
byte numleds = 30; 
WS2812 LEDA(numleds);
WS2812 LEDB(numleds);
#define ledapin 9
#define ledbpin 11
#endif

#ifdef board_prototype_2
// Board 2 is a leonardo with 2 strings of LEDs
// 60/metre .5 metre each string connected to separate pins
byte numleds = 30;
WS2812 LEDA(numleds);
WS2812 LEDB(numleds);
#define ledapin 7
#define ledbpin 11
#endif

unsigned long starttime;

Venerate EBOX = Venerate(0);

// Set all LEDs off
//
void clearleds() {
  cRGB value = { 0, 0, 0};
  for (byte i = 0; i < (numleds); i++) {
    LEDA.set_crgb_at(i, value);
    LEDB.set_crgb_at(i, value);
  }
  LEDA.sync();
  LEDB.sync();
}

void setup()
{
  starttime = millis();
  
  pinMode(13,OUTPUT);

  LEDA.setOutput(ledapin);
  LEDB.setOutput(ledbpin);
  Serial1.begin(19200);
  EBOX.begin(Serial1);
  Serial.begin(19200);

  EBOX.setdebug(Serial, 1);
  clearleds();  
}

// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
// http://stackoverflow.com/questions/340209/generate-colors-between-red-and-green-for-a-power-meter
//
// We always want full saturation so we just remove some of the calculations below
// to save some processing effort

cRGB h1v_to_rgb(byte inh, byte inv)
{
    cRGB rgb;
    unsigned char region, p, q, t;
    unsigned int h, s, v, remainder;

    h = inh;
    v = inv;
    region = h / 43;
    remainder = (h - (region * 43)) * 6; 
    //p = (v * (255 - s)) >> 8;
    //q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    //t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    // we know s is always full, so just cheat save some cpu
    p = 0;
    q = (v * (255 - remainder)) >> 8;
    t = (v * remainder) >> 8;
    
    switch (region)  {
        case 0:
            rgb.r = v;            rgb.g = t;            rgb.b = p;
            break;
        case 1:
            rgb.r = q;            rgb.g = v;            rgb.b = p;
            break;
        case 2:
            rgb.r = p;            rgb.g = v;            rgb.b = t;
            break;
        case 3:
            rgb.r = p;            rgb.g = q;            rgb.b = v;
            break;
        case 4:
            rgb.r = t;            rgb.g = p;            rgb.b = v;
            break;
        default:
            rgb.r = v;            rgb.g = p;            rgb.b = q;
            break;
    }
    return rgb;
}

// Save some serial comms, only grab a/b knob values every few times
// around, so let's global remember some stuff

int knobb = 0;
int knoba = 0;
int barcount = 0;

void bargraph(void) {
    int levelb,levela,widtha,freqa,widthb,freqb;

    byte gateb = EBOX.getbyte(ETMEM_gateb) & 1;
    byte gatea = EBOX.getbyte(ETMEM_gatea) & 1;

    if (gateb == 1) {
        levelb = EBOX.getbyte(ETMEM_levelb);
        widthb = EBOX.getbyte(ETMEM_widthb);
        freqb = EBOX.getbyte(ETMEM_freqb);
    } else {
        levelb = widthb = freqb = 0;
    }
    if (gatea == 1) {
        levela = EBOX.getbyte(ETMEM_levela);
        widtha = EBOX.getbyte(ETMEM_widtha);
        freqa = EBOX.getbyte(ETMEM_freqa);
    } else {
        levela = widtha = freqa = 0;
    }
    if (knoba == 0 || knobb == 0 || barcount++ > 5) {
      knobb = EBOX.getbyte(ETMEM_knobb);
      knoba = EBOX.getbyte(ETMEM_knoba);
      barcount = 0;
    }
    // let's try the frequency is the brightness, but the knob is the colour
    // int intensitya = max(10,(max(widtha-128,0) *4 + max(freqa-128,0)*2)/3);
    // int intensityb = max(10,(max(widthb-128,0) *4 + max(freqb-128,0)*2)/3);

    levela = max(0,levela-128) + max(0,widtha-128);
    levelb = max(0,levelb-128) + max(0,widthb-128);

    int intensitya = max(20,255-freqa); //max(0,freqa-64)+64;
    int intensityb = max(20,255-freqb);  //max(0,freqb-64)+64;

    // we want 255 redish and 0 greenish so lets shift
    // the hue so 255 is red and knob <24 is just floor near green
    byte hue = knobb+10;  // overflow okay
    if (knobb < 64) hue = 74;
    cRGB value = h1v_to_rgb( hue, intensityb);    

    for (byte i = 0; i < numleds; i++) {
      if (levelb <= i * (256/numleds)) value = {0,0,0};
      LEDB.set_crgb_at(numleds-1-i, value);
    }
    LEDB.sync();

    hue = knoba+10; // overflow okay
    if (knoba < 62) hue = 72;
    value = h1v_to_rgb( hue, intensitya);  

    for (byte i = 0; i < numleds; i++) {
      if (levela <= i * (256/numleds)) value = {0,0,0};
      LEDA.set_crgb_at(numleds-1-i, value);
    }
    LEDA.sync();
}

byte cylon = 0;

// The main loop; if we're not already connected to a box, try to
// connect to a box.  If we're connected then grab some values and
// light up some LEDs

byte nowtime = 0;

void loop()
{  
  if (EBOX.isconnected()) {
    bargraph();
  } else {
  // Try to connect every couple of seconds, don't connect until first second
        if ((millis() - starttime) > 2000) {
           starttime = millis();
           cRGB value = { 0, 0, 0 };
           LEDA.set_crgb_at(cylon, value);
           LEDB.set_crgb_at(cylon, value);
           cylon++;
           if (cylon>numleds) cylon = 0;           
           value = { 255, 255, 255 };  // 100 0 0 
           LEDA.set_crgb_at(cylon, value);
           LEDB.set_crgb_at(cylon, value);
           LEDA.sync();          
           LEDB.sync();           
           digitalWrite(13,HIGH);
           EBOX.hello();
           digitalWrite(13,LOW);                          
        }
        nowtime++;    
        delay(100);        
  }
}
