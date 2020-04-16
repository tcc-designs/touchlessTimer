//Develop a delay()-less way to sense hand waves. 
//To-Do, monitor the average and reset the arduino if it gets too high. 
#include <movingAvg.h>//to smooth the proximity sensor reading
#include <SevSeg.h>
#include <CapacitiveSensor.h>//to make a 2 pin capacitive sensor

//A no-touch capacitive sensor triggered count-down timer
CapacitiveSensor   cs_4_2 = CapacitiveSensor(4,2);        // 4 1M resistors in series between pins 4 & 2, pin 2 is sensor pin.

movingAvg handSensor(100); //this is the baseline slow moving average of the sensor. 
movingAvg fastAvg(6);//this is the smoothed transient response

SevSeg sevseg; //instantiate the seven segment display object.

//Global Variables
bool timerOn = false;
bool timerTrigger = false;
int counter = 2000; //standing for 20 seconds at .01 increments
unsigned long timer = 0;
int forcedReset = 5; //pin used for hard resetting

void setup() {

  //seven segment display setup
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
   
  byte numDigits = 4;   
  byte digitPins[] = {A0, A1, A2, A3}; //Digits: 1,2,3,4 <--put one resistor (ex: 220 Ohms, or 330 Ohms, etc, on each digit pin)
  byte segmentPins[] = {A4, A5, 6, 7, 8, 9, 11, 10}; //Segments: A,B,C,D,E,F,G,Period
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = true; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected
  //seven segment display initialization 
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);  

  //capacitive sensor initialization
  //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  handSensor.begin();
  fastAvg.begin();

  //forced reset setup
  digitalWrite(forcedReset, HIGH);
  delay(200);
  pinMode(forcedReset, OUTPUT);

  //standard serial communication
  Serial.begin(115200);
  while (millis()<2000){
    waveSensor();
  }
}

void loop() {
  timerTrigger = waveSensor();
  if (timerTrigger == true) {
    timerOn = true;
  }
  if (timerOn == true) {
    while (timerOn == true) {
      countDownTimer();
      //Serial.println("while looping");
    }
    handSensor.reset();
    fastAvg.reset();
  //timerTrigger = false;
  }
  sevseg.setNumber(0000);
  sevseg.refreshDisplay();
  
  
}

bool waveSensor() {
    static bool isSensing = false;
    static bool lastSensedState = false;
    static unsigned long startOfSensing = 0;
    static bool trigger = false;
    long start = millis();                        //for checking the capacitive sensor's reading performance
    long total1 =  cs_4_2.capacitiveSensor(30);   //take the capacitive sensor's reading
    int intTotal1 = int(total1);                  //make an integer value to hold the converted value of the capacitive sensor library
    int transient = fastAvg.reading(intTotal1);
    int capAvg = handSensor.reading(intTotal1);   //add the capacitive sensor reading value to the running average
    if (transient > (2*capAvg)) {                   //compare the average value of the sensor to the current to see if there's a spike indicating presence
      isSensing = true;
    } else {
      isSensing = false;
    }
    if ((isSensing == true) && (lastSensedState == false)) {
      lastSensedState = true;
      startOfSensing = millis();
      //Serial.println("sensed something");
    }
    if ((isSensing == false) && (lastSensedState == true)) {
      lastSensedState = false;
      trigger = false;
      startOfSensing = 0;//reset the sensor's sensing timer
      //Serial.println("stopped sensing something");
    }
    if ((isSensing == true) && (lastSensedState == true)) {
      if ((millis() - startOfSensing) >= 250) {
        trigger = true;
        //Serial.println("triggered"); 
      }
    }
    if (capAvg >= 300){
      Serial.println("reset due to lost sensitivity");
      digitalWrite(forcedReset, LOW);
    }
    //Serial.print(millis() - start);               // check on performance in milliseconds
    //Serial.print("\t");                           // tab character for debug window spacing

    Serial.print(transient);                       // print sensor output 1
    Serial.print("\t");

    Serial.println(capAvg);
    //Serial.print("\t");
    //Serial.print("isSensing=");
    //Serial.println(isSensing);
    delay(50);
    return trigger;
    
}
void countDownTimer() {
  //static unsigned long timer = millis();
  if (counter == 2000){
    timer = millis();
    counter = counter -1;
  }
  //Serial.print("timer value  ");
  //Serial.print(timer);
  //static int counter = 2000;
  //Serial.print("    counter  ");
  //Serial.println(counter);
  if (millis() - timer >= 10) {
    timer += 10;
    counter = counter -1; // 10 milliSeconds is equal to 1 hundredth of a second
    
    if (counter == 0) { // Reset to 0 after counting for 1000 seconds.
      //timerTrigger = false;
      timerOn = false;
      counter = 2000;
    }
    sevseg.setNumber(counter, 2);
  }
  sevseg.refreshDisplay();
}
