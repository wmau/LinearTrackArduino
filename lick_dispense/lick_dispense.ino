#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

int sensors[] = {3, 4, 5, 6, 7, 8, 9, 10};

int valves[] = {3, 4, 5, 6, 7, 8, 9, 10};

//int pumpOpen = 200;
//int pumpOpen = 30;
int pumpOpen = 4000;
uint16_t curr_touched;

int nSensors = sizeof(sensors) / sizeof(int);

Adafruit_MPR121 cap = Adafruit_MPR121();

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
    pinMode(valves[i], OUTPUT);
    digitalWrite(valves[i], LOW);
  }

  cap.setThresholds(6, 2);

}

void loop() {
  recalibrate();
  // get capacitive sensor input. 
  curr_touched = cap.touched();
  
  for (byte i = 0; i < nSensors; i++){
    if (curr_touched & _BV(i)){
      digitalWrite(valves[i], HIGH);
      delay(pumpOpen);
      digitalWrite(valves[i], LOW);

      delay(500);
    }
  }
}
