/*
 
  The circuit:
 * LCD RS pin to digital pin 9
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 2
 * LCD D5 pin to digital pin 3
 * LCD D6 pin to digital pin 4
 * LCD D7 pin to digital pin 5
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */

// include the library code:
#include <LiquidCrystal.h>
#define bool boolean

// Analog sensor base class.
template<int TPin>
class AnalogSensor
{
  int _lastValue; // Current value of sensor reading in range [0, 1024)
public:
  AnalogSensor() : _lastValue(0) {}
  void Update()
  {
    _lastValue = analogRead(TPin);
  }
  int GetValue() const
  {
    return _lastValue;
  }
};

template<int TPin, bool TPressLevel = true, int TThreshold = 40>
class Button
{
  int _switchTime;
  bool _isDown;
  bool _isUp;
public:
  Button() :
   _switchTime(0),
   _isUp(false),
   _isDown(false)
   {
    pinMode(TPin, INPUT);
    digitalWrite(TPin, TPressLevel ? LOW : HIGH);
   }
   void Update()
   {
     bool level = digitalRead(TPin) == HIGH ? true : false;
     if (!TPressLevel) level != level;

     const long time = millis();
     if (level && !_isDown && !_isUp)
     {
      // Transition to pressed
      _isDown = true;
      _switchTime = time;
     }
     else if (!level && _isDown)
     {
      // Transition to unpressed
      
      if (time - _switchTime > TThreshold)
      {
        _isUp = true;
        _switchTime = time;
      }
      else
      {
        _isDown = false;
      }
     }

   }

   bool CheckPressedAndReset()
   {
    const bool isPressed = _isDown && _isUp && ( millis() - _switchTime ) >= TThreshold;
    if (isPressed)
    {
      _isDown = _isUp = false;
    }
    return isPressed;
   }
};

template<int TPin, bool TActiveLevel = true>
class DigitalOut
{
public:
 DigitalOut()
 {
  pinMode(TPin, OUTPUT);
  digitalWrite(TPin, TActiveLevel ? LOW : HIGH);
 } 
 void Set(bool on)
 {
  if (TActiveLevel)
  {
    digitalWrite(TPin, on ? HIGH : LOW);
  }
  else
  {
    digitalWrite(TPin, on ? LOW : HIGH);
  }
 }
};


class Settings
{
public:
  int GetMinWateringTime() const { return 2000; }
  int GetMaxWateringTime() const { return 5000; }
  int GetMoistureStartTrigger() const { return 900; } // Maximum allowed resistance of sensor to start watering.
  int GetMoistureStopTrigger() const { return 2000; }
  int GetMinWateringInterval() const { return 20000; }
  int GetMutetModeLightingSensor() const { return 500; } // Resistance of light sensor exciding which whatering is forbidden.
};


class State
{
public:
  State(){};
  virtual ~State(){};
  virtual void OnEnter(){};
  virtual void OnExit(){};
  virtual State* Update() = 0;
};

class Application
{
  State* _current;
public:
  Application(State* initial)
  {
    _current = initial;
  }

  void Update()
  {
    MoistureSensor.Update();
    LButton.Update();
    CButton.Update();
    RButton.Update();

    State* newState = _current->Update();
    if (newState != _current)
    {
      _current->OnExit();
      newState->OnEnter();
      _current = newState;
    }
  }
public:
  AnalogSensor<A1> MoistureSensor;
  Button<6> LButton;
  Button<7> CButton;
  Button<8> RButton;
  Settings Settings;
};

Application* TheApp;



class ControlState : public State
{
  long _enterTime;
public:
  ControlState () : _enterTime(0)
  {

  }
  virtual State* Update()
  {
    // TODO: !!!
    //    if (TheApp->MoistureSensor.GetValue() > 2 )
    return this;
  }
};























// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 11, 2, 3, 4, 5);
const int sensorPin = A1;    // select the input pin for the potentiometer
const int sensetivityPin = A2;    // select the input pin for the potentiometer
const int pumpPin = 13;

const int lButtonPin = 7;
const int cButtonPin = 7;
const int rButtonPin = 6;
int waterMin = 1024, waterMax = 0;
long lastPumpTime = 0;
const int minPumpDelta = 1000;
const int maxPumpWorkingTime = 2000;
boolean pumpState = false;

void setup() {
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  long currentTime = millis();
  if (!pumpState){
    if (currentTime - lastPumpTime > minPumpDelta){
      digitalWrite(pumpPin, HIGH);
      lastPumpTime = currentTime;
      pumpState = true;
    }
  }else{
    if (currentTime - lastPumpTime  > maxPumpWorkingTime){
      digitalWrite(pumpPin, LOW);
      lastPumpTime = currentTime;
      pumpState = false;
    }
  }
    
  
  // print the number of seconds since reset:
  int waterSensor = analogRead(sensorPin);
  waterMin = min( waterSensor, waterMin);
  waterMax = max( waterSensor, waterMax);
  lcd.setCursor(0, 0);
  lcd.print(waterSensor);
  lcd.print("     ");
  lcd.setCursor(0, 1);
  lcd.print(waterMin);
  lcd.print("  ");
  lcd.setCursor(8, 1);
  lcd.print(waterMax);
  lcd.print("  ");
}

