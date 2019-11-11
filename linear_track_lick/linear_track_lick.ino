#include <TimedAction.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// define the character variable for interfacing with Python.
char handshake;

//define miniscope syncing pin (must be 2 on Arduino Uno). 
int miniscope_pin = 2;

// define ports corresponding to relays/solenoids/water ports here.
int valves[] = {3, 4, 5, 6, 7, 8, 9, 10};

// define rewarded relays/solenoids/water ports here.
boolean rewarded[] = {1, 0, 0, 1, 0, 1, 0, 0};

// define duration of solenoid opening (ms).
int pumpOpen = 50;

// number of ports and misc variables.
int nSensors = sizeof(valves) / sizeof(valves[0]);
int nVisits = 0;          //number of visits this lap.
int i = 0;                //for iteration.
bool justdrank[] = {0, 0, 0, 0, 0, 0, 0, 0};  //for tracking which ports were drank. 
int miniscope_frame = 0;  //miniscope frame counter.

// define capacitive sensor stuff.
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t curr_touched = 0;

// function for advancing miniscope frames.
void advance_miniscope_frame() {
  miniscope_frame += 1;
  //Serial.println(String(miniscope_frame)); //debugging purposes. 
}

// function for writing lick timestamps.
void record_lick() {
  Serial.print(String(i));          // port number that was drank. 
  Serial.print(", ");
  Serial.print(String(miniscope_frame));  //miniscope frame number. 
  Serial.print(", ");
  Serial.print(String(millis()));   //timestamp in ms (relative to Arduino reboot). 
  Serial.println();                 //write new line.
}

// Function for dispensing water. Writes LOW to a pin that opens the solenoid.
void dispense_water(int valve) {
  digitalWrite(valve, LOW);
  delay(pumpOpen);  // open for this long (ms).
  digitalWrite(valve, HIGH);
}

// Function for counting number of visits this lap.
void count_visits() {
  nVisits = 0;    //reset counter each time. 
  
  // Sum across wells.
  for (int well = 0; well < nSensors; well++) {
    int val = justdrank[well];
    nVisits += val;

    // If there have been three visits, reset the counts for each well.
    if (nVisits >= 3) {
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
  Serial.print("Lap");
  Serial.print(", ");
  Serial.print(miniscope_frame);  //miniscope frame number. 
  Serial.print(", ");
  Serial.print(String(millis())); //timestamp in ms (relative to Arduino reboot).
  Serial.println();
}

// define record_lick threading.
TimedAction lickThread = TimedAction(50, record_lick);

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
  cap.setThresholds(2,1);

  // wait for handshake signal from Python function. To start the program, 
  // Python must write 'g' to the Serial port. I think single quotes are required. 
  while (handshake != 'g') {
    if (Serial.available() > 0) {
      handshake = Serial.read();
    }
  }

  // print the current timestamp (in milliseconds) relative to when the Arduino
  // rebooted (from Python sync). 
  Serial.println(millis());
}

// ***************** LOOPITY LOOP ***************
void loop() {
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
        justdrank[i] = true;

        //Find total number of visits and reset port mask if lap elapsed.
        count_visits();
      }
    }
  }
}
