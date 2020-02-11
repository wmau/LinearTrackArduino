#include <TimedAction.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

//define miniscope syncing pin (must be 2 on Arduino Uno). Do not use 13 on Uno.
int miniscope_pin = 2;

//define pin for triggering acquisition from DAQ box. Do not use 13 on Uno.
int trigger_pin = 12;

// define pins corresponding to relays/solenoids/water ports here.
int valves[] = {3, 4, 5, 6, 7, 8, 9, 10};

// define rewarded relays/solenoids/water ports here.
//boolean rewarded[] = {0, 0, 1, 0, 0, 0, 0, 1};    //modify as needed
boolean rewarded[] = {1, 1, 1, 1, 1, 1, 1, 1};  //reward everything

// define duration of solenoid opening (ms).
int pumpOpen = 15; //15 works well for my setup (depends on height of reservoir and volume)

// define length of recording here (ms).
unsigned long duration = 1200000;

// number of ports and misc variables.
char handshake;
int nSensors = sizeof(valves) / sizeof(valves[0]);
int nVisits = 0;          //number of visits this lap.
int i = 0;                //for iteration.
bool justdrank[] = {0, 0, 0, 0, 0, 0, 0, 0};  //for tracking which ports were drank. 
unsigned int miniscope_frame = 0;  //miniscope frame counter.
int nRewarded = 0;        //number of ports mouse needs to visit to reset ports.
unsigned long offset;     //time in between Arduino reboot and first action it can perform.
unsigned long ms;         //for timestamping.
unsigned int previous_frame;   //for timestamping.
unsigned int curr_frame;  //for timestamping.

// define capacitive sensor stuff.
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t curr_touched = 0;

// function for advancing miniscope frames.
void advance_miniscope_frame() {
  miniscope_frame += 1;
  Serial.println(String(miniscope_frame)); //debugging purposes. 
}

// function for writing information to serial port (converted to txt by Python function read_Arduino())
void write_timestamp(signed int val) {
  ms = millis();
  curr_frame = miniscope_frame;

  if ((curr_frame != previous_frame) || (val == -1)){
    String str = String(val) + ", " + curr_frame + ", " + String(ms);
    Serial.println(str);
  }

  previous_frame = curr_frame;
}

// function for writing lick timestamps.
void record_lick() {
  write_timestamp(i);
}

// Function for dispensing water. Writes LOW to a pin that opens the solenoid.
void dispense_water(int valve) {
  digitalWrite(valve, LOW);
  delay(pumpOpen);  // open for this long (ms).
  digitalWrite(valve, HIGH);

  // Also tell computer that water was delivered. 
  write_timestamp(-1);
}

// Function for counting number of visits this lap.
void count_visits() {
  nVisits = 0;    //reset counter each time. 
  
  // Sum across wells.
  for (int well = 0; well < nSensors; well++) {
    int val = justdrank[well];
    nVisits += val;

    // If there have been N visits, reset the counts for each well.
    if (nVisits >= 5) {
      lap();
    }
  }
}

// Function that resets the visit counts for each well and also 
// writes "Lap" to Serial port.
void lap() {
  //resets all drink flags. 
  for (int c = 0; c < nSensors; c++) {
    justdrank[c] = false;
  }
}


// function for resetting the capacitive sensor. Helps with noise issues 
// from relay board. 
void recalibrate() {
    // Clean the I2C Bus
    pinMode(A5, OUTPUT);
    for (int i = 0; i < 8; i++) {
     
      // Toggle the SCL pin eight times to reset any errant commands received by the slave boards.
      digitalWrite(A5, HIGH);
      delayMicroseconds(3);
      digitalWrite(A5, LOW);
      delayMicroseconds(3);
    }
    pinMode(A5, INPUT);
}

// function for triggering Miniscope recording. Make sure "Trigger Ext" is checked.
void start_recording() {
  digitalWrite(trigger_pin, HIGH);
}


// function for stopping Miniscope recording. Make sure "Trigger Ext" is checked.
void stop_recording() {
  digitalWrite(trigger_pin, LOW);
}

// define record_lick threading.
TimedAction lickThread = TimedAction(35, record_lick);

// ***************** SETUP ***************
void setup() {
  Serial.begin(115200);

  // needed to keep Arduino from starting too fast!
  while (!Serial) {
    delay(10);
  }

  // Default address for capacitive sensor is 0x5A at 5V.
  // If tied to 3.3V it's 0x5B.
  if (!cap.begin(0x5A)) {
    Serial.println("Capacitive touch sensor not found. Check wiring.");
    while (1);
  }

  // define all the relay ports as outputs.
  for (int j = 0; j < nSensors; j++) {
    digitalWrite(valves[j], HIGH);
    pinMode(valves[j], OUTPUT);
  }

  // define the miniscope pin and make an interrupt for counting miniscope frames.
  pinMode(miniscope_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(miniscope_pin), advance_miniscope_frame, CHANGE);


  // set capacitive sensor thresholds here. 
  cap.setThresholds(3, 2);

  // count number of rewarded ports.
  for (i = 0; i < nSensors; i++){
   if (rewarded[i]){
    nRewarded++;
     }
  }

  // wait for handshake signal from Python function. To start the program, 
  // Python must write 'g' to the Serial port. I think single quotes are required. 
  while (handshake != 'g') {
    if (Serial.available() > 0) {
      handshake = Serial.read();
    }
  }

  // At this point, the Arduino will reboot and will be unable to perform
  // any operations for about 1.4 seconds. This is measured in the variable
  // called "offset". 

  // begin the Miniscope recording. 
  start_recording();
  
  // print the current timestamp (in milliseconds) relative to when the Arduino
  // rebooted (from Python sync). 
  offset = millis();
  Serial.println(offset);
}

// ***************** LOOPITY LOOP ***************
void loop() {
  // stop the recording at the defined time. Be sure to compensate for the offset.
  if (millis() >= (duration + offset)) {
    stop_recording();
  }

  // recalibrate capacitive sensor to address possible noise. 
  recalibrate();
  
  // get capacitive sensor input. 
  curr_touched = cap.touched();

  // iterate through sensors. 
  for (i = 0; i < nSensors; i++) {
    //if touched, record lick
    if (curr_touched & _BV(i)){         
      record_lick();

      //if port should be rewarded and
      //if mouse did not just drink from this port 
      if (rewarded[i] & !justdrank[i]) {
        //Dispense water.
        dispense_water(valves[i]);
        lickThread.check(); // This lets you write lick events while the solenoid is open.

        //Flag port after drinking.
        justdrank[i] = 1;

        //Find total number of visits and reset port mask if lap elapsed.
        count_visits();

      }
    }
  }
}
