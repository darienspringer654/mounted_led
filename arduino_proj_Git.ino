/*  Automated Aquarium Lighting System
 *  v 0.82
 *  Updated August 26, 2019
 *  Tested with Arudino Mega on August 26, 2019
 */


/* TABLE OF CONTENTS
 *   • Initialization
 *   • setup()
 *   • loop()
 *   • Main loop functions
 *   • Unused functions
 *   • Setup functions
 *   • Debugging functions
 */

 
/* INITIALIZATION BELOW
 *    Contains all of the necessary libraries,
 *    pin defintions, and global variables
 *    in order to run the program successfully.
 */

// Lighting is arranged in seven separate channels (2 Red @ 2700K, 2 Yellow @ 4000K, 2 Blue @ 6500K, 1 Moonlight @ 4000K)
// Pin 13 allows one of the 4000K channels to be split to allow extra-dim nighttime lighting (NOTE: This might be trickier with flexible strips)
// Defines pins for PWM control; 5 - 10 are day lights only, 4 is day/night and simulates moonlight
// Pin 13 is for fan control (Unlikely be implemented during this class)
// NOTE: Active pin assignment will only work for Arduino Mega, Zero & Due (These channels need to be changed to PWM-capable pins (Uno: 3, 5, 6, 9, 10, 11; Mega: 2 - 13, 44 - 46)

#include <IRremote.h>

#define RedOne      5
#define YellowOne   6
#define BlueOne     7

#define RedTwo      8
#define YellowTwo   9
#define BlueTwo     10

#define FanControl  13            // Does not need to be PWM-capable pin if paired with PWM-capable fan controller (Pin 13 is PWM-capable on Mega, Zero, & Due)        TODO: Finish MoonCloud setup and change FanControl pin to 11

#define Moonlight   YellowOne     // TODO: Remove after presentation in class; utilzed due to difficulty in running seventh strip of flexible LEDs on heatsink
#define MoonCloud   BlueOne

int secondsPassed = 0;
int previousSeconds = 0;
int loopCalled = 0;

int hours;
int minutes;
int seconds;
int totalMin;

const int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;

int currentFunctionNumber = 0;

//initializes the delays between sets of flashes in storm mode
int cycleDelay = 1000;


// Pin setup for Uno, Nano, Mini, Leonardo, Micro, & Yún (NOTE: Will not allow 6500K lights to be split into two channels, or allow PWM fan control without changing pin assignment)
// NOTE: Complete list of PWM-capable pins for Arduino boards can be found @ https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/

// Remember to connect SDA/SCL lines to Real-Time Clock (RTC)
// RCTLIB.h can be found @ https://github.com/adafruit/RTClib or downloaded directly via Sketch-->Include Library-->Manage Libraries within the Arduino IDE
#include "RTClib.h"
RTC_DS3231 Chronos;   // Code written for DS3231 Real-Time Clock (May also work with DS1307 & PCF8523, but is untested with these)
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // Allows days of week to be displayed in serial monitor


// Both ColorModifer and PeakColor variables allow adjustment of light level slope and maximum output for any given temperature
// ColorModifer variables allow either a more gradual or a steeper slope to be utilized during the sunrise/sunset periods of the day
// PeakColor variables don't effect slope angle, but rather change the maximum level possible for each light temperature

//ColorModifier variables
float RedModifier = 1.0;
float YellowModifier = 1.0;   // RedModifier, YellowModifier, and BlueModifier comprise the the ColorModifier variables
float BlueModifier = 1.0;

//PeakColor variables
int PeakRed = -1;
int PeakYellow = -1;          // PeakRed, PeakYelow, and PeakBlue comprise the PeakColor variables
int PeakBlue = -1;


// These variables save weather state globally for the day
int switchState = -1;             // Sets weather pattern for the day @ 6:44am
boolean weatherSelected = false;  // Allows weather to be set at times other than 6:44am if power has been lost
boolean isCloudy = false;         // Helps initialize proper nighttime light selection between 4000K lights and 6500K lights
/* End of initialization. */




/* PROGRAM SETUP BELOW
 *    This section contains the the actual code which
 *    is utilized during the startup of the Arduino.
 */

void setup() {                // Initializes Arudino for use
  Serial.begin(9600);

  randomSeed(analogRead(0));    // Seeds the random nubmer generator with noise from Analog 0 pin; probably will give funky results if this pin is used for something else

  pinMode(RedOne, OUTPUT);
  pinMode(YellowOne, OUTPUT);
  pinMode(BlueOne, OUTPUT);
  
  pinMode(RedTwo, OUTPUT);
  pinMode(YellowTwo, OUTPUT);
  pinMode(BlueTwo, OUTPUT);
  
  pinMode(Moonlight, OUTPUT);
  pinMode(FanControl, OUTPUT);

  findClock();
  resetClock();         // Necessary to set time if running RTC for first time, or battery dies
  clockSerialTest();

  Serial.println("\nSetup successful!");
  
  //enables the IR receiver
  irrecv.enableIRIn();
  irrecv.blink13(true);

}

void loop() 
{ 
  DateTime now = Chronos.now();
  hours = now.hour();
  minutes = now.minute();
  seconds = now.second();
  int totalMin = ((hours * 60) + minutes);

  /*If IR receiver does not receive signal, then current function is called again.
   * The current function cycles until the IR receiver receives another signal. Then
   * the current function changes*/
  if (irrecv.decode(&results))
  {
    Serial.println(results.value, HEX);
    irrecv.resume();
    switch(results.value)
    {
      case 0xFF30CF: //"1"
      currentFunctionNumber = 1;
      //standardLightCycle(totalMin);
      break;
      case 0xFF18E7: //"2"
      currentFunctionNumber = 2;
      //cloudyLightCycle();
      break;
      case 0xFF7A85: //"3"
      currentFunctionNumber = 3;
      //cloudyDay();
      break;
      case 0xFF10EF: //"4"
      currentFunctionNumber = 4;
      //cloudyNight();
      break;
      case 0xFF38C7: //"5"
      currentFunctionNumber = 5;
      //stormMode();
      break;
      case 0xFF5AA5: //"6
      currentFunctionNumber = 6;
      //lightCycleSelector();
      break;
      case 0xFF42BD: //"7"
      currentFunctionNumber = 7;
      //twoMinuteDemo();
      break;

      irrecv.resume();
      
    }
  }
 
 switch(currentFunctionNumber)
 {
      case 1: 
      standardLightCycle(totalMin);
      break;
      case 2:
      cloudyLightCycle();
      break;
      case 3:
      cloudyDay();
      break;
      case 4:
      cloudyNight();
      break;
      case 5:
      stormMode();
      break;
      case 6:
      lightCycleSelector();
      break;
      case 7:
      twoMinuteDemo();
      break;
 }

}


/* MAIN LOOP FUNCTIONS BELOW
 *    Various lighting functions which serve as the overall structure of the project.
 *    Includes lighting schema, schema selector, storm mode, and demo mode for presentation purposes.
 *    (May not all be utilized in final project--this section is a work-in progress.)
 *    
 *    Includes:
 *      • lightCycleSelector()    - Case switch for weather preset selectin; randomized each day
 *      • standardLightCycle()    - Base template for lighting presets; fully functional by itself
 *      
 *      Time-of-Day Functions:
 *        • morningPeriod()         - Controls sunrise and morning lighting
 *        • afternoonPeriod()       - Controls peak lighting
 *        • eveningPeriod()         - Controls sunset and evening lighting
 *        • nightPeriod()           - Controls moonlight & night lighting
 *      
 *      Presets:
 *        • cloudyDay()             - Simulates cloudy weather from morning - evening
 *        • cloudyLightCycle()      - Simulates cloudy weather all day, including night
 *        • cloudyNightCycle()      - Simulates a cloudy night, with a normal day
 */

void lightCycleSelector() {
  // Introduces slight randomness within lighting schema to allow for variability across days, simulating pseudorandom weather
  // Should work fine with switch cases as selector
  
  if (totalMin == 404 || weatherSelected == false) {        // Will turn off lights @ 6:44 for one minute, and randomly cycle through possible weather patterns
    switchState = random (1, 5);      // Random range is (min) to (max - 1); e.g., random(0, 5) will return 0 - 4
    analogWrite(Moonlight, 0);
    analogWrite(MoonCloud, 0);
    weatherSelected = true;
  }
  
  if (totalMin != 404) {        // Starts the selected weather pattern @ 6:45
    switch (switchState) {
      case 1:
        standardLightCycle(totalMin);
        break;
      case 2:
        cloudyLightCycle();
        break;
      case 3:
        cloudyDay();
        break;
      case 4:
        cloudyNight();
        break;
      case 5:
        stormMode();
        break;
      case 6:
        twoMinuteDemo();
        break;
        
        
    }
  }
}


void standardLightCycle(int totalMin) {
  // Basic 24-hour lighting scheme; should be good enough to be the sole lighting scheme
  // Ideally can serve as template in which other lighting schemes are based
  
  int value = -1;
  PeakRed = 255;      // Allows easier maximum level adjustment for each channel; can be used to set lower absolute threshold for PWM control; currently unutilized
  PeakYellow = 255;   
  PeakBlue = 255;

  if ( totalMin >= 405 && totalMin < 705 ) {         // 6:45am - 11:45am; simulates sunrise and morning lighting schedule
    morningPeriod(totalMin, value);
  }

  else if ( totalMin >= 705 && totalMin < 960 ) {    // 11:45am - 4:00pm; simulates peak lighting time during the afternoon
    afternoonPeriod(totalMin, value);
  }

  else if ( totalMin >= 960 && totalMin < 1305 ) {   // 4:00pm - 9:45pm; simulates sunset and evening schedule
    eveningPeriod(totalMin, value);
  }

  else {                                          // 9:45pm - 6:45am; simulates nighttime and soft moonlight
    nightPeriod();
  }
}


void morningPeriod(int totalMin, int value) {     // 6:45am - 11:45am; simulates sunrise and morning lighting schedule
  value = map(totalMin, 405, 645, 0, 255);
  value = constrain(value, 0, PeakRed);
  value = value * RedModifier;
  analogWrite(RedOne, value);               // Begin increasing 2700K lights (6:45 - 10:45am)
  analogWrite(RedTwo, value);
  
  if (totalMin >= 435 && totalMin <= 675) {    // Begin increasing 4000K lights (7:15am - 11:15am); doesnt effect Moonlight channel
    value = map(totalMin, 435, 675, 0, 255);
    value = constrain(value, 0, PeakYellow);
    value = value * YellowModifier;
    analogWrite(YellowOne, value); 
    analogWrite(YellowTwo, value);
    //analogWrite(Moonlight, value);
    
    if (totalMin >= 465 && totalMin < 705) {    // Begin increasing 6500K lights (7:45am - 11:45am)
      analogWrite(Moonlight, value);        // Allows longer rest of Moonlight channel during the morning (60min vs 30min otherwise)
      
      value = map(totalMin, 465, 705, 0, 255);
      value = constrain(value, 0, PeakBlue);
      value = value * BlueModifier;
      analogWrite(BlueOne, value);
      analogWrite(BlueTwo, value);
    }
  }
}


void afternoonPeriod(int totalMin, int value) {   // 11:45am - 4:00pm; simulates peak lighting time during the afternoon
  value = PeakRed;
  value = value * RedModifier;
  analogWrite(RedOne, value);
  analogWrite(RedTwo, value);

  value = PeakYellow;                     // value capable of being adjusted on the fly during peak
  value = value * YellowModifier;
  analogWrite(YellowOne, value);
  analogWrite(YellowTwo, value);
  analogWrite(Moonlight, value);

  value = PeakBlue;
  value = value * BlueModifier;
  analogWrite(BlueOne, value);
  analogWrite(BlueTwo, value);
}


void eveningPeriod(int totalMin, int value) {   // 4:00pm - 9:45pm; simulates sunset and evening schedule
  value = map(totalMin, 960, 1245, 255, 0);
  value = constrain(value, 0, PeakBlue);
  value = value * BlueModifier;
  analogWrite(BlueOne, value);                  // Begin dimming 6500K lights (4:00pm - 8:45pm)
  analogWrite(BlueTwo, value);

  if (totalMin >= 990 && totalMin <= 1275) {     // Begin dimming 4000K lights (4:30pm - 9:15pm)
    value = map(totalMin, 990, 1275, 255, 0);
    value = constrain(value, 0, PeakYellow);
    value = value * YellowModifier;
    analogWrite(YellowOne, value);
    analogWrite(YellowTwo, value);
    analogWrite(Moonlight, value);

    if (totalMin >= 1020 && totalMin < 1305) {  // Begin dimming 2700K lights (5:00pm - 9:45pm)
      value = map(totalMin, 1020, 1305, 255, 0);
      value = constrain(value, 0, PeakRed);
      value = value * BlueModifier;
      analogWrite(RedOne, value);
      analogWrite(RedTwo, value);
    }
  }
}


void nightPeriod() {      // 9:45pm - 6:45am; simulates nighttime and soft moonlight
  analogWrite(RedOne, 0);
  analogWrite(RedTwo, 0);

  //analogWrite(YellowOne, 0);
  analogWrite(YellowTwo, 0);

  //analogWrite(BlueOne, 0);
  analogWrite(BlueTwo, 0);

  if (isCloudy == false) {     // Turn on 4000K moonlight
    analogWrite(Moonlight, 3);
    analogWrite(MoonCloud, 0);
  }

  else {        // Turn on 6500K moonlight
    analogWrite(MoonCloud, 3);    // TODO: Test value for acceptable light levels
    analogWrite(Moonlight, 0);
  }
}


void cloudyDay() {      // Blue-shifted & dimmer cycle from morning until evening; normal night
 
  RedModifier = 0.3;
  YellowModifier = 0.4;
  BlueModifier = 1.0;
  isCloudy = false;
  
  standardLightCycle(totalMin);
  
  RedModifier = 1.0;
  YellowModifier = 1.0;
  BlueModifier = 1.0;
  isCloudy = false;
}


void cloudyLightCycle() {   // Blue-shifted and dimmer cycle during the entire photoperiod, including night
  RedModifier = 0.3;
  YellowModifier = 0.4;
  BlueModifier = 1.0;
  isCloudy = true;
  
  standardLightCycle(totalMin);
  
  RedModifier = 1.0;
  YellowModifier = 1.0;
  BlueModifier = 1.0;
  isCloudy = false;
}


void cloudyNight() {      // Normal day, cloudy night utilizng 6500K light
  RedModifier = 1.0;
  YellowModifier = 1.0;
  BlueModifier = 1.0;
  isCloudy = true;
  
  standardLightCycle(totalMin);
  
  RedModifier = 1.0;
  YellowModifier = 1.0;
  BlueModifier = 1.0;
  isCloudy = false;
}




/* UNUSED FUNCTIONS BELOW
 *    This section is a work in progress.
 *    It contains a myriad of half-baked ideas
 *    which may or may not go somewhere. 
 */

void cloudyAfternoon() {
  // Like standardLightCycle, but with a dimmer & blue-shifted period in the afternoon for several hours
}

void cloudyMorning() {
  // Like standardLightCycle, but with a dimmer & blue-shifted period in the morning for several hours
}

void cloudyAdjustable(int startTotalMin, int endTotalMin) {
  // May allow for simpler cloudy schema that does not rely upon presets (possibly replacing many of them entirely)
}

void extraSunnyDay() {
  // Extra-bright (may not be a great idea due to excessive algae or skittish fish)
}

void chaoticDay() {
  // Utilizes randomization...
}

void twoMinuteDemo() {
  // Quick demonstration of a 24-hour cycle that doesn't require actually waiting 24 hours to see; doesn't have to be two minutes
  // 700 seconds : 1 second if two-minute demo

  DateTime demo = Chronos.now();
  

  int howManySecondsHavePassed = 0;
  int secondCurrentlyIterating = demo.second();
  int secondPreviouslyIterating = -1;
  
  
  if (secondCurrentlyIterating != secondPreviouslyIterating) {
    howManySecondsHavePassed = howManySecondsHavePassed + 1;
    secondPreviouslyIterating = secondCurrentlyIterating;
  }
  

  //translates a range from 0 to 120 to a range from 0 to 86400 (which is how many seconds there are in a day)
  int translateSecondsToDayCycle = 0;
  translateSecondsToDayCycle = map(howManySecondsHavePassed, 0, 120, 0, 86400);

  //convert seconds to minutes
  int translatedMinutes = translateSecondsToDayCycle / 60;

  //int translatedTotalMinutes = ((hours * 60) + translatedMinutes);
  int translatedTotalMinutes = translatedMinutes;
  
  standardLightCycle(translatedTotalMinutes);

}


void stormMode() {
  //random amount of flashes in a set and a random amount of time between each set of flashes
  analogWrite (RedOne, 0);
  analogWrite (RedTwo, 0);
  analogWrite (YellowOne, 0);
  analogWrite (YellowTwo, 0);
  analogWrite (BlueOne, 0);
  analogWrite (BlueTwo, 0);
 
  int cycleDelay = random(0, 5000);
  delay(cycleDelay);
  
  PeakYellow = 255;
  
  randomSeed(analogRead(0));

  //the amount of delay between sets of flashes (max delay is 2 minutes)
  int amountOfFlashesInSet = random(1, 11);

  //the amount of delay between each individual flash in the set (max delay is 5 seconds)
  int flashDelay = 0;

  int i = amountOfFlashesInSet;

  while(i != 0)
  {
    int lightningSelect = random(1, 4);
    switch(lightningSelect) {
      case 1:
        analogWrite(YellowOne, PeakYellow);
        break;
      case 2:
        analogWrite(YellowTwo, PeakYellow);
        break;
      case 3:
        analogWrite(YellowOne, PeakYellow);
        analogWrite(YellowTwo, PeakYellow);
        break;
      
    }
       
    flashDelay = random(4, 10);
    delay(flashDelay);
    analogWrite(YellowOne, 0);
    analogWrite(YellowTwo, 0);

    --i;
  }
}




/* SETUP FUNCTIONS BELOW
 *    These three functions are run solely at setup.
 *    They are used to:
 *      • findClock()       - Determine if an RTC is connected to the Arduino
 *      • resetClock()      - Correctly set the time of the RTC if it is not set
 *      • clockSerialTest() - Display the current time of the RTC to the serial monitor
 */

void findClock() {        // Notifies if RTC cannot be found
  if (! Chronos.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
}


void resetClock() {       // Ensures that if battery is removed or dies, clock can be reset by connecting to a computer via USB
  if (Chronos.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // Following line sets the RTC to the date & time this sketch was compiled
    Chronos.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // Chronos.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}


void clockSerialTest() {    // Simple test to check timing on RTC; should output the current date & time to the serial monitor
  delay(500);   // Ensures RTC has time to start properly before being called to output serial data; probably unnecessary
  Serial.println("Startup time: ");
  
  DateTime now = Chronos.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}



/*
 * DEBUGGING FUNCTIONS BELOW
 *    Contains a series of functions which test components of project code in isolation.
 *    Originally contained helpful setup code to initialize the RTC for the first time, 
 *    and evaluate if an issue is detected with the RTC at startup, but these have
 *    been moved to their own section within SETUP FUNCTIONS.
 *    
 *    They are used to:
 *       • clockBlinkTest()         - Switches a pair of LEDs on/off based upon if the current minute is odd or even
 *       • clockPWMTest()           - Increases intensity of LEDs with PWM based upon input from RTC every second
 *       • ClockPWMChannelTest()    - Like clockPWMTest(), but with each channel scaling at a different rate
 *       • doNothing()              - Does exactly what it says it does for 50 seconds while adding nothing beyond delay(), because that's literally all it is
 *       • clockSerialDebugTest()   - Tests RTC response speed -- used to ensure Arudino can't call RTC so fast that errors start to occur
 *       • minuteTime               - Test function which translates hour and minute output from the RTC into a single variable called totalMin
 *       • mapTest()                - Applies map() function to totalMin to map a specific time (within a range) to be mapped to an associated PWM value
 *       • mathTest()               - Simple math test whcih examines multiplication behavior between integers and floats
 *       • systemStatus()           - Composite function which combines the functionality of minuteTime() and mapTest(); redundant
 *       • roundingTest()           - Evaluate how rounding of integers & floats is handled with random()
 *       • multiVariableClockTest() - Evaluate how if() statements are handled when paired with multiple conditions from RTC
 *
 
void clockBlinkTest() {    // Another simple debugging test to make sure clock is working properly; should switch between a pair of LEDs every minute
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  
  DateTime now = Chronos.now();
  while (now.minute() % 2 == 0) {   // Triggers on even minutes; e.g., 2, 4, 32, 58
    digitalWrite (8, HIGH);
    digitalWrite (9, LOW);
    break;
  }
  
  while (now.minute() % 2 == 1) {   // Triggers on odd minutes; e.g., 1, 3, 31, 57
    digitalWrite (9, HIGH);
    digitalWrite (8, LOW);
    break;
  }

}


void clockPWMTest() {         // Should scale brightness from 0 up to 255 and then back down to 0 using PWM, with an increase/decrease speed of 1 per second via feedback from RTC; Working
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  DateTime startTime = Chronos.now();
  DateTime checkTime = startTime;

  while (checkTime.second() == startTime.second()) {
    checkTime = Chronos.now();
    //Serial.print(". ");

    if (checkTime.second() != startTime.second()) {
       debugLightLevel++;
       startTime = checkTime;
       analogWrite(9, debugLightLevel);
       analogWrite(10, debugLightLevel);
       analogWrite(11, debugLightLevel);
       //Serial.println("");
       break;
    }
  }
}


void clockPWMChannelTest() {         // Should scale brightness from 0 up to 255 and then back down to 0 using PWM, with an increase/decrease speed of 1 - 3 per second via feedback from RTC; Working
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  DateTime startTime = Chronos.now();
  DateTime checkTime = startTime;

  while (checkTime.second() == startTime.second()) {
    checkTime = Chronos.now();
    //Serial.print(". ");

    if (checkTime.second() != startTime.second()) {
       debugLightLevel++;
       debugLightLevel2 = debugLightLevel2 + 2;
       debugLightLevel3 = debugLightLevel3 + 3;
       startTime = checkTime;
       analogWrite(9, debugLightLevel);
       analogWrite(10, debugLightLevel2);
       analogWrite(11, debugLightLevel3);
       //Serial.println("");
       break;
    } 
  }
}


void doNothing() {      // For testing pinMode functionality outside of setup (e.g., does pin state reset once function has ended? Answer: Apparently not!); Scrappable
  delay(50000);
}


void clockSerialDebugTest(int total) {      // Tests RTC response speed by requesting the current second for a number of times equal to totalIterations
  for (int current = 0; current < total; current++) {
    DateTime now = Chronos.now(); 
    Serial.println(now.second());
  }
}


void minuteTime() {           // Converts hour + minutes to only minutes (since midnight); e.g., 2:30am becomes 150 minutes
  DateTime now = Chronos.now();
  delay(1000);

  Serial.println("");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  
  int hours = now.hour();
  String hoursWords = "Hours: ";
  String hoursString = hoursWords + hours;
  Serial.println ("\n" + hoursString);
  
  int minutes = now.minute();
  String minutesWords = "Minutes: ";
  String minutesString = minutesWords + minutes;
  Serial.println (minutesString);
  
  int totalMin = ((hours * 60) + minutes);
  String totalWords = "Total Minutes: ";
  String totalString = totalWords + totalMin;
  Serial.println (totalString); 
}


void mapTest(int startMinute, int endMinute) {              // Takes the approach used in minuteTime() and maps it from 0 - 255, simulating a PWM scaling from completely off to completely on for a given length of time
  DateTime now = Chronos.now();
  
  int hours = now.hour();
  int minutes = now.minute();
  int totalMin = ((hours * 60) + minutes);

  int value = totalMin;
  value = map(value, startMinute, endMinute, 0, 255);
  value = constrain(value, 0, 255);
  String words = "PWM Value: ";
  //String totalMinutes = value;
  Serial.println(words + value);

  analogWrite(9, value * 0.2);
  analogWrite(10, value * 0.5);
  analogWrite(11, value);
}


void mathTest() {             // Checks operations between integers and floats, and associated output; for double-checking color temperature can be modified in this manner
  int testInt = 5;    
  float testFloat = 6.5;
  int answer = testInt * testFloat;
  Serial.println(answer);
  delay(500);
  
  answer = testInt + testFloat;
  Serial.println(answer);
  delay(500);
  
  testInt = 160;
  testFloat = 0.8;
  answer = testInt * testFloat;
  Serial.println(answer);
  delay(500);
}


void systemStatus() {       // Acts as a composite function of minuteTime() and mapTest(); redundant
  DateTime now = Chronos.now();

  Serial.println("");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  
  int hours = now.hour();
  String hoursWords = "Hours: ";
  String hoursString = hoursWords + hours;
  Serial.println ("\n" + hoursString);
  
  int minutes = now.minute();
  String minutesWords = "Minutes: ";
  String minutesString = minutesWords + minutes;
  Serial.println (minutesString);
  
  int totalMin = ((hours * 60) + minutes);
  String totalWords = "Total Minutes: ";
  String totalString = totalWords + totalMin;
  Serial.println (totalString); 

  int value = totalMin;
  value = map(value, 405, 720, 0, 255);
  String words = "PWM Value: ";
  Serial.println (words + value);

  delay(500);
}


void roundingTest() {     // Extremely simple test of the random() function to evaluate the effect of rounding on integers and floats
   float testRandom = random(40, 61);
   float randomDivide = testRandom / 100;
   //int testRandomRound = round(testRandom);
   Serial.println (randomDivide, 2);
   delay(300);
}


void multiVariableClockTest() {
  DateTime now = Chronos.now();
  
  if (now.hour() == 13 && now.minute() == 3) {
    Serial.println ("AHHHHHHHHHHHHHHHHHHH");
  }

  if (now.hour() == 13 && now.minute() == 4) {
    Serial.println ("NOOOOOOOOOOOOOOOOOOOO");
  }
  
}
*/
