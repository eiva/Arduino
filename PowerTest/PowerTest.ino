/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
const int power_switch = 6;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(power_switch, OUTPUT);     
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(power_switch, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(power_switch, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second
}
