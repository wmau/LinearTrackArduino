#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

int sensors[] = {2, 3};

int valves[] = {2, 3};

int pumpOpen = 200;

int nSensors = sizeof(sensors) / sizeof(int);

Adafruit_MPR121 cap = Adafruit_MPR121();

void setup() {
  Serial.begin(9600);

  // needed to keep leonardo/micro from starting too fast!
  while (!Serial) {
    delay(10);
  }

  Serial.println("Program initialized.");

  // Default address is 0x5A at 5V. 
  // If tied to 3.3V it's 0x5B. 
  if (!cap.begin(0x5A)) {
    Serial.println("Capacitive touch sensor not found. Check wiring.");
    while(1);
  }
  Serial.println("Sensor found!");

  for (byte i = 0; i < nSensors; i++){
    pinMode(sensors[i], INPUT);
  }

  for (byte i = 0; i < nSensors; i++){
    digitalWrite(valves[i], HIGH);
    pinMode(valves[i], OUTPUT);
  }
}

void loop() {

  for (byte i = 0; i < nSensors; i++){
    if (cap.touched() & _BV(i)){
      digitalWrite(valves[i], LOW);
      delay(pumpOpen);
      digitalWrite(valves[i], HIGH);
    }
  }
}
