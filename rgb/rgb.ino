/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

const int ON = LOW;
const int OFF = HIGH;

int led = 13;
int ledR = 2;
int ledG = 3;
int ledB = 4;

void rgb(int R, int G, int B)
{
  digitalWrite(ledR, R == 1?ON:OFF);
  digitalWrite(ledG, G == 1?ON:OFF);
  digitalWrite(ledB, B == 1?ON:OFF);
  delay(500);
}

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(ledR, OUTPUT);     
  pinMode(ledG, OUTPUT);     
  pinMode(ledB, OUTPUT);    
  pinMode(led, OUTPUT);
  rgb(OFF, OFF, OFF);
}

// the loop routine runs over and over again forever:
void loop() {
  rgb(0,0,1);
  rgb(0,1,0);
  rgb(0,1,1);
  rgb(1,0,0);
  rgb(1,0,1);
  rgb(1,1,0);
  rgb(1,1,1);
}
