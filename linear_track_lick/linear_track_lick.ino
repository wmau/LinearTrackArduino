#include <TimedAction.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// define ports corresponding to relays/solenoids here.
int valves[] = {2, 3, 4, 5, 6, 7};

// define rewarded relays/solenoids here.
boolean rewarded[] = {1, 0, 0, 1, 0, 1};

// define duration of solenoid opening (ms).
int pumpOpen = 50;

// number of ports and rewarded locations.
int nSensors = sizeof(valves) / sizeof(valves[0]);
int nVisits = 0;
int i = 0;
bool justdrank[] = {0, 0, 0, 0, 0, 0};

// define capacitive sensor stuff.
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t curr_touched = 0;


// function for writing lick timestamps.
void record_lick() {
  Serial.println(String(i));
}

// Function for dispensing water. Writes LOW to a pin that opens the solenoid.
void dispense_water(int valve) {
  digitalWrite(valve, LOW);
  delay(pumpOpen);
  digitalWrite(valve, HIGH);
}

// Function for counting number of visits this lap.
void count_visits() {
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

// Function that resets the visit counts for each well.
void lap() {
  for (int c = 0; c < nSensors; c++) {
    justdrank[c] = false;
  }
  Serial.println("Lap");
}

// define record_lick threading.
TimedAction lickThread = TimedAction(50, record_lick);

// ***************** SETUP ***************
void setup() {
  Serial.begin(115200);

  // needed to keep leonardo/micro from starting too fast!
  while (!Serial) {
    delay(10);
  }

  // Default address is 0x5A at 5V.
  // If tied to 3.3V it's 0x5B.
  if (!cap.begin(0x5A)) {
    Serial.println("Capacitive touch sensor not found. Check wiring.");
    while (1);
  }
  Serial.println("Sensor found!");

  // define all the relay ports as output.
  for (int j = 0; j < nSensors; j++) {
    digitalWrite(valves[j], HIGH);
    pinMode(valves[j], OUTPUT);
  }

  cap.setThresholds(2,1);
}

// ***************** LOOPITY LOOP ***************
void loop() {
  curr_touched = cap.touched();

  for (i = 0; i < nSensors; i++) {
    //if touched, record lick
    if (curr_touched & _BV(i)){         
      record_lick();

      //if port should be rewarded and
      //if mouse did not just drink from this port 
      if (rewarded[i] & !justdrank[i]) {
        //Dispense water.
        dispense_water(valves[i]);
        lickThread.check();

        //Flag port after drinking.
        justdrank[i] = true;

        //Find total number of visits and reset port mask.
        nVisits = 0;
        count_visits();
      }
    }
  }
}
