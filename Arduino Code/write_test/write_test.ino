int var = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  while (var < 60) {
    Serial.println('lick');
    delay(1000);
    var++;
  };

}
