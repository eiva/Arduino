#include <Stepper.h>

class PaperFeed
{
   Stepper _stepper;
public:
   PaperFeed(int steps, int _1, int _2, int _3, int _4)
   : _stepper(steps, _1, _2, _3, _4)
   {
     _stepper.setSpeed(30);
   }
   void NextLine()
   {
     _stepper.step(5);
   }
   void PageBeak()
   {
     _stepper.step(250);
   }
};

class PrinterHead
{
   int _latch;
   int _strobe;
   int _clock;
   int _input;
   int _power;
public:
   PrinterHead(int latch, int strobe, int clock, int input, int power)
   : _latch(latch)
   , _strobe(strobe)
   , _clock(clock)
   , _input(input)
   , _power(power)
   {
      // Set mode for TPH
      pinMode(_latch, OUTPUT);
      pinMode(_strobe, OUTPUT);
      pinMode(_clock, OUTPUT);
      pinMode(_input, OUTPUT);
      pinMode(_power, OUTPUT);
      // Initial: We need to switch strobe off as soon it is possible to prevent TPH overheating.
      digitalWrite(_latch, HIGH); // Inverted
      digitalWrite(_strobe, HIGH); // Inverted
      digitalWrite(_clock, LOW);
      digitalWrite(_input, LOW);
      digitalWrite(_power, LOW);
   }
   void Burn(int expositionTime, byte mask[16]) // need 16 bytes
   {
     digitalWrite(_power, HIGH);
     delay(10);
     for (int i = 0; i < 16; ++i)
     {
        for (int j = 0; j < 8; ++j)
        {
           uint8_t pinValue = bitRead(mask[i], j) != 0 ? HIGH : LOW;
           digitalWrite(_input, pinValue);
           delayMicroseconds(10);
           digitalWrite(_clock, HIGH);
           delayMicroseconds(10);
           digitalWrite(_clock, LOW);
           delayMicroseconds(10);
        }
     }
     delayMicroseconds(10);
     digitalWrite(_latch, LOW);
     delayMicroseconds(10);
     digitalWrite(_latch, HIGH);
     delayMicroseconds(10);
     noInterrupts();
     digitalWrite(_strobe, LOW);
     delay(expositionTime);
     digitalWrite(_strobe, HIGH);
     interrupts();
     digitalWrite(_input, LOW);
     digitalWrite(_power, LOW);
   }
};


PaperFeed feeder(250, 4,2,3,5);
PrinterHead head(10, 9, 12, 11, 6);
String string;

void setup() {
   // start serial port:
   Serial.begin(9600);
   while (!Serial) {
      ; // wait for serial port to connect. Needed for Leonardo only
   }
}
void loop() {
  while (Serial.available() <= 0)
  {
  }
  string = Serial.readStringUntil(' ');
  if (string != "C"){
      Serial.println("Error: Waiting for command");
      Serial.flush();
      return;
   }
   string = Serial.readStringUntil(' ');
   if (string == "T")
   {
      Serial.println("PONG");
      Serial.flush();
   }
   else if (string == "S")
   {
      processStepper();
   }
   else if (string == "P")
   {
      processPrint();
   }

   Serial.flush();
}


void processStepper()
{
   string = Serial.readStringUntil(' ');
   if (string == "L")
      feeder.NextLine();
   else
      feeder.PageBeak();
   Serial.println("DONE!");
}

void processPrint()
{
   string = Serial.readStringUntil(' ');
   int fireTime = string.toInt();
   fireTime = constrain(fireTime, 1, 10);

   byte masks[16];

   for (int i = 0; i < 16; ++i)
   {
      string = Serial.readStringUntil(' ');
      masks[i] = string.toInt();
   }
   
   head.Burn(fireTime, masks);
   Serial.println("DONE!");
}
