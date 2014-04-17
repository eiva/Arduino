#include <Stepper.h>

const int stepsPerRevolution = 250;

Stepper myStepper(stepsPerRevolution, 4,2,3,5);            

int stepCount = 0;

void setup() {
  myStepper.setSpeed(30);
}

void loop() {
  
  // step one step:
  myStepper.step(100);
  //stepCount++;
  delay(200);
}
