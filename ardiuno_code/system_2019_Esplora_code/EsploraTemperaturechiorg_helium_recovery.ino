/*
 Esplora control of helium recovery
 This turns on/off a relay (OUT_A and led) driving a valve alowing compressed air into ballons so that 
 the helium located in adjascent ballons is flushed into a (leaking) helium recovery system.
 There are multiple steps:
 0 (green): filling helium - wait for the helium sensor (INPUT_B) to annonce that ballons are fulll or a timeout of 8h (by default...
   this can be modified by clicking during 1s up and down the keyboard buttons)
 1 (green/blue): pushing helium without testing the air pressure sensor (time : see Ref1)
 2 (blue): pushing helium testing the air pressure sensor (INPUT_A) to stop it (time : see Ref2)
 Here could go back to 0, but the optional cycles below allow to fill the balloons with and external source of He
 3 (yellow): filling helium without testing the helium pressure sensor to avoid missfire 
 4 (red): filling ext helium without testing the helium pressure sensor to avoid missfire 
 5 (yellow): filling helium without testing the helium pressure sensor to avoid missfire.
    This is to allow the direct Helium to get in before it rises too much in pressure.
 6 (red): filling ext helium without testing the helium pressure sensor to avoid missfire 

 The right button increments the step (press during exactly 1s - screen refresh is slow - every 4 seconds)
 The displyed text is written on the SD card (if present) 
*/
 
#include <SD.h>
const int chipSelect = 8;//for esplora. For aruino set to 4
#include <Esplora.h>
#include <TFT.h>            // Arduino LCD library
#include <SPI.h>
//#if ARDUINO < 105
const byte CH_TINKERKIT_INA = 8;   // Add values missing from Esplora.h
const byte CH_TINKERKIT_INB = 9;
const byte INPUT_A          = 0;
const byte INPUT_B          = 1;
int        OUT_A            = 3;
int        OUT_B            = 11;
const byte LED              = 13;

byte min_press_he = 9.0;// minimal pressure to stop filling.
                        // decrease pressing the left button during 1 s
const byte min_press_air = 5.0;// for air switch

String filename = "";

unsigned int readTinkerkitInput(byte whichInput) {      // return 0-1023 from Tinkerkit Input A or B
   return readChannel(whichInput+CH_TINKERKIT_INA); }   //   as defined above
 
unsigned int readChannel(byte channel) {                // as Esplora.readChannel is a private function
     digitalWrite(A0, (channel & 1) ? HIGH : LOW);      //  we declare our own as a hack
     digitalWrite(A1, (channel & 2) ? HIGH : LOW);      //
     digitalWrite(A2, (channel & 4) ? HIGH : LOW);      // digitalWrite sets address lines for the input
     digitalWrite(A3, (channel & 8) ? HIGH : LOW);
     return analogRead(A4);               // analogRead gets value from MUX chip
}

void setup()
{
  Serial.begin(9600);      // initialize serial communications with your computer

 // Put this line at the beginning of every sketch that uses the GLCD
  EsploraTFT.begin();

  // clear the screen with a black background
  EsploraTFT.background(0, 0, 0);

  // set the text color to magenta
  EsploraTFT.stroke(200, 20, 180);
  // set the text to size 2
  EsploraTFT.setTextSize(1);//coudl be 2 ...
  // start the text at the top left of the screen
  // this text is going to remain static

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }else{// determine file name
      Serial.println("card initialized.");
    for(int loo=0;loo<10000;loo++){
      filename="logd";
      filename+=String(loo);
      filename+=".txt";
     if (SD.exists(filename)==0) {
      Serial.println("will use file :");
      Serial.println(filename);
      break;
     }
    }
  }
    
File dataFile = SD.open(filename, FILE_WRITE);
  pinMode(OUT_A, OUTPUT);     
  pinMode(OUT_B, OUTPUT);     
  //pinMode(INPUT_A, INPUT_PULLUP);    does not seem to do anythign,, 

  digitalWrite(OUT_A, LOW);
  digitalWrite(OUT_B, LOW);
  digitalWrite(LED, LOW);
  // if the file is available, write to it:
  if (dataFile) {
    // write header of file
    String dataString = "";
    dataString += String("INA");  dataString += ",";
    /* min/typ/max/unit
    Supply Voltage 4.75 5.00 5.25 V
    Supply Current - 2.0 mA
    Pressure range (fs) ±102 mmH2O
    Zero Output (Dif) 2.45 2.5 2.55 Vdc@0mmH2O
    Span Output (Dif) 2.00 Vdc(pos)
    Response time 25 ms
    Linearity -2.5 +2.5 %FS
    Thermal Hysteresis -0.1 +0.1 %FS
    Temp coeff. Offset -2.5 +2.5 %FS
    Temp coeff. Span -2 +2 %FS
    Pressure overload 10X rating
    Temp compensation 0 50 °C
    Operating Temp range -20 70 °C
    Storage temperature -40 125 °C*/

    dataString += String("IN2");  dataString += ",";
    
    dataString += String("nb_full_cycles");  
   /*Low Voltage Operation (+2.7 V to +5.5 V)
    Calibrated Directly in °C
    10 mV/8°C Scale Factor 
    ±2°C Accuracy Over
    Temperature (typ)
    ±0.5°C Linearity (typ)
    Stable with Large Capacitive Loads
    Specified -40 °C to +125 °C, Operation to +150 °C
    Less than 50 µA Quiescent Current
    Shutdown Current 0.5 µA max */
      dataFile.println(dataString);
      dataFile.close();
      Serial.println(dataString);
  }
} 
 // unsigned int input_a_value_perv ;
unsigned int gate ;
long int count=0 ;
int input_a_value;
long int i;
float pressure;
float pressure2;
int first=1;

static float f_val = 123.6794;
static char outstr[15];
char tempPrintout[400];  // array to hold the temperature data
char tempPrintout2[2];  // array to hold the temperature data
String stringOne, stringTwo;

int stats=-1;
int cycling_pointer=-1;
unsigned long  timeout=1;
unsigned long  initial_timeout=1;
unsigned long  list_timeout[4*7+1];//4*8+1  7*6+1
unsigned long int  starttime = 0;
unsigned long int  endtime=0;
unsigned long main_time=8*3600;
unsigned long default_nb_hours = 8;//was 10....
unsigned long int main_counter_full_cycles=0;

void loop()
{
  // read the temperature sensor in Celsius, then Fahrenheit:
  int celsius = Esplora.readTemperature(DEGREES_C);
  count=0;
  
  float tour_per_sec;
  int inci;
//  endtime = starttime;

  int input_a_value = 512-readTinkerkitInput(INPUT_A);
  int input_b_value = 512-readTinkerkitInput(INPUT_B);
   
  first=0;
 
  stringOne= String("");// initialize the string.
  stringTwo= String("");// initialize the string.

  /*
  stringOne+="Temp.    ";
  dtostrf(celsius,4, 0, outstr);
  stringOne+=outstr;
  stringOne+=" deg. C\n";
  */
  
  /*
  stringOne+="Air fl.OFF(INA) ";
  // Serial.print(count);
  //  Serial.print(" holes/s ");
  tour_per_sec=(float)count/(float)8;
  dtostrf(tour_per_sec,7, 2, outstr);
  stringOne+=outstr;
  stringOne+=" Hz\n";
  */

 stringOne+="Press.He (INB)";
 // Serial.print(input_b_value);
 pressure=input_b_value*10.002783/512;
 // Serial.print(" /512 ");
 //Serial.print(pressure);
 dtostrf(pressure,6, 2, outstr);
 // Serial.print(" ");
 stringOne+=outstr; 
 stringOne+=" ";
 if (pressure>min_press_he )
    stringOne+=">";
 else
    stringOne+="<";
 dtostrf(min_press_he,3, 1, outstr);
 stringOne+=outstr; 
 stringOne+="\n";

 stringOne+="Press.Air(INA)";
 // Serial.print(input_a_value);
 pressure2=input_a_value*10.002783/512;
 // Serial.print(" /512 ");
 //Serial.print(pressure2);
 dtostrf(pressure2,6, 2, outstr);
 // Serial.print(" ");
 stringOne+=outstr; 
 stringOne+=" ";
 if (pressure2>min_press_air )
    stringOne+=">";
 else 
    stringOne+="<";
 dtostrf(min_press_air,3, 1, outstr);
 stringOne+=outstr; 
 stringOne+="\n";

if (stats==0) {stringOne+="0 Fill He\n* Test He sensor\n";}
if (stats==1) {stringOne+="1 Push He\n  Ignore air sensor\n";}
if (stats==2) {stringOne+="2 Push He\n* Test air sensor\n";}
if (stats==3) {stringOne+="3 Fill He\n  Ignore He sensor\n";}
if (stats==4) {stringOne+="4 Fill EXT He\n  Open external input (1/2)\n";}
if (stats==5) {stringOne+="5 Fill He\n  Wait before second ext. in\n";}
if (stats==6) {stringOne+="6 Fill EXT He\n  Open external input (2/2)\n";}
stringOne+="Countdown : ";
ltoa(timeout,outstr,10);
stringOne+=outstr;
stringOne+=" s (";
ltoa(initial_timeout-timeout,outstr,10);
stringOne+=outstr;
stringOne+=")\n";

timeout-=1;// decrement the contdown
// see if action is needed... if timeout or if pressure swith or button on the keyboard
if (timeout==0) {
  list_timeout[cycling_pointer]=initial_timeout;
} else {
  if  (stats==0) if (pressure>min_press_he || Esplora.readButton(SWITCH_RIGHT)==LOW){    //switch of helium ... abort... loop
    list_timeout[cycling_pointer]=initial_timeout-timeout;
    timeout=0;
 }
 if  (stats==1) if ( Esplora.readButton(SWITCH_RIGHT)==LOW){   
    list_timeout[cycling_pointer]=initial_timeout-timeout;
    timeout=0;
 }
 if  (stats==2) if (pressure2>min_press_air  || Esplora.readButton(SWITCH_RIGHT)==LOW){    //switch of air ... abort... loop
    list_timeout[cycling_pointer]=initial_timeout-timeout;
    timeout=0;
 }
 if  (stats>2) if ( Esplora.readButton(SWITCH_RIGHT)==LOW){   
    list_timeout[cycling_pointer]=initial_timeout-timeout;
    timeout=0;
 }
}

//adjust the number of hours (main cycle)
if ( Esplora.readButton(SWITCH_DOWN)==LOW) default_nb_hours-=1;
if ( Esplora.readButton(SWITCH_UP)==LOW) default_nb_hours+=1;
if ( Esplora.readButton(SWITCH_LEFT)==LOW) min_press_he-=1;

main_time=default_nb_hours*3600-(20+5*60+2*4*60+2*5*60);// set the countdown according to the time...

if (timeout==0) {
  stats=stats+1;
  if (stats==7) {stats=0;main_counter_full_cycles+=1;  
  digitalWrite(OUT_B, LOW);// switch Helium on the relay
  }
  if (stats==0) {timeout=main_time;}
  if (stats==1) {timeout=20;  //Ref. 1 If change here also change main_time above...
  digitalWrite(OUT_A, HIGH);// switch on the relay
  digitalWrite(LED, HIGH);// turns on the led
  }
  if (stats==2) {timeout=5*60;}//Ref. 2
  if ((stats==3)|(stats==5)) {timeout=4*60; //Ref. 3 wait 4 minutes 
                                            //If change here also change main_time above...

  digitalWrite(OUT_A, LOW);// switch off the relay
  digitalWrite(LED, LOW);// turns off the led
  digitalWrite(OUT_B, LOW);// switch Helium on the relay not always necessary...
  }
  if ((stats==4)|(stats==6)) {timeout=5*60; //Ref. 3 now open for external source 2x with pause in between
                                            //If change here also change main_time above...
  digitalWrite(OUT_B, HIGH);// switch Helium on the relay fill ext. source (twice)
  }
  initial_timeout=timeout;
  cycling_pointer++;
  if (cycling_pointer==7*4) cycling_pointer=0;// 4*8 
}

// dump delays of earlyer cylcles (to see problems)
for (int lo=0;lo<4;lo++) {
 for (inci=0;inci<7;inci++) {//could be 7 but may not have enough space on line
  if (lo*7+inci==cycling_pointer){
   stringOne+=">";
  }else{
   stringOne+=" ";
  }
  ltoa(list_timeout[lo*7+inci],outstr,10);
  stringOne+=outstr;
  if (inci==2)
     stringOne+="\n";/// break line in two parts (3+4)
 }
 stringOne+="\n";// 
}

// add number of hours wait main cycle:
stringOne+=" (";
ltoa(default_nb_hours,outstr,10);
stringOne+=outstr;
stringOne+=" h)";

// add number full cycles:
stringOne+=" C:";
ltoa(main_counter_full_cycles,outstr,10);
stringOne+=outstr;
stringOne+="";

Serial.println(stringOne);//if pluged to computer get using ....

stringTwo="";
ltoa(stats,outstr,10);
stringTwo+=outstr;
  
if (((timeout)&3)==0){//every four seconds... 
  //erase ...
  EsploraTFT.stroke(0, 0, 0); // set the text color to black to erase by overwriting... not very elegant
  EsploraTFT.setTextSize(1);//coudl be 2 ...
  EsploraTFT.text(tempPrintout, 0, 0);
  EsploraTFT.setTextSize(3);
  EsploraTFT.text(tempPrintout2, 120,60);
  //write ...
  EsploraTFT.setTextSize(1);
  EsploraTFT.stroke(255, 255, 255); // set the text color to white
  stringOne.toCharArray(tempPrintout, 400);  // convert the string to a char array
  EsploraTFT.text(tempPrintout, 0, 0);
  EsploraTFT.stroke(250, 20, 180); // set color to magenta
  EsploraTFT.setTextSize(3);//text size
  stringTwo.toCharArray(tempPrintout2, 2);
  EsploraTFT.text(tempPrintout2, 120,60);
}
  

/*
 int slider = Esplora.readSlider();//0 to 1024
int jx=Esplora.readJoystickX();//-512 to 512
int jy=Esplora.readJoystickY();
  // convert the sensor readings to light levels
  byte bright  = slider/4;
  byte jxb  = jx/2;
  byte jyb  = jy/2;
  // write the light levels to the Red LED
  Esplora.writeRed(bright);
  Esplora.writeRGB(jxb,jyb,bright);//0 to 255
*/

  Esplora.writeRGB(5, 0, 0); // turn led red.... while writing on sd card
  /* String dataString = "";
  dataString += String(pressure);  dataString += ","; 
  dataString += String(pressure2);  dataString += ",";
  dataString += String(main_counter_full_cycles);  */
  File dataFile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
      //dataFile.println(dataString);//
      dataFile.println(stringOne);//
      dataFile.close();
      //Serial.println(dataString);
      Serial.println(stringOne);
  }
    
  // set RGB led color according to step number
  if (stats==0)  Esplora.writeRGB(0, 2, 0);
  if (stats==1)  Esplora.writeRGB(0, 2, 2);
  if (stats==2)  Esplora.writeRGB(0, 0, 2);
  if ((stats==3)|(stats==5))  Esplora.writeRGB(2, 2, 0);
  if ((stats==4)|(stats==6))  Esplora.writeRGB(2, 0, 0);

   // wait until the end of the second...
  while (abs(endtime - starttime) <=1000) // do this loop for up to 1 s
  {
     endtime = millis();
  }
  starttime=endtime;// reset
  
  // wait a second before runninging again:
  //delay(1000);
}
