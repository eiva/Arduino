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


class SettingsContainer
{
public:
  int GetMinWateringTime() const { return 2000; }
  void SetMinWateringTime(const int& val){  }

  int GetMaxWateringTime() const { return 5000; }
  void SetMaxWateringTime(const int& val){  }

  int GetMoistureStartTrigger() const { return 900; } // Maximum allowed resistance of sensor to start watering.
  void SetMoistureStartTrigger(const int& val){  }

  int GetMoistureStopTrigger() const { return 2000; }
  void SetMoistureStopTrigger(const int& val){  }

  int GetMinWateringInterval() const { return 20000; }
  void SetMinWateringInterval(const int& val){  }

  int GetMutetModeLightingSensor() const { return 500; } // Resistance of light sensor exciding which whatering is forbidden.
  void SetMutetModeLightingSensor(const int& val){  }
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

  State *_controlState;
  State *_wateringState;
  State *_settingsState;

public:
  Application();

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

  State *GetControlState() const { return _controlState; }
  State *GetWateringState() const { return _wateringState; }
  State *GetSettingsState() const { return _settingsState; }
public:
  AnalogSensor<A1> MoistureSensor;
  Button<6> LButton;
  Button<7> CButton;
  Button<8> RButton;
  LiquidCrystal lcd;
  DigitalOut<A2> Pump;
  SettingsContainer Settings;
};

Application* TheApp;

class TimedState : public State
{
private:
  long _enteringTime;
  long _previousLeaveTime;
public:
  TimedState() : _enteringTime(0), _previousLeaveTime(0) { }
  virtual void OnEnter()
  {
    _enteringTime = millis();
  }
  virtual void OnExit()
  {
    _previousLeaveTime = millis();
  }
protected:
  // Gets duration since entering the state.
  long GetDuration() const { return millis() - _enteringTime; }
};

class ControlState : public TimedState
{
public:
  ControlState ()
  {

  }
  virtual State* Update()
  {
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("Control: ");
    TheApp->lcd.print(TheApp->MoistureSensor.GetValue());
    TheApp->lcd.setCursor(0, 1);
    TheApp->lcd.print("Last: ");
    TheApp->lcd.setCursor(8, 1);
    TheApp->lcd.print(GetDuration() / 1000 / 60);
        
    if (TheApp->MoistureSensor.GetValue() >= TheApp->Settings.GetMoistureStartTrigger() )
      if (GetDuration() > TheApp->Settings.GetMinWateringInterval())
        return TheApp->GetWateringState();

    if (TheApp->CButton.CheckPressedAndReset())
      return TheApp->GetSettingsState();

    return this;
  }
};

class WateringState : public TimedState
{
public:
  virtual State* Update()
  {
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("!!Pump Active!!");
    TheApp->lcd.setCursor(0, 1);
    TheApp->lcd.print(TheApp->MoistureSensor.GetValue());
    TheApp->lcd.print("   ");
    TheApp->lcd.setCursor(8, 1);
    TheApp->lcd.print(TheApp->Settings.GetMoistureStopTrigger());
    TheApp->lcd.print("   ");

    if (GetDuration() >= TheApp->Settings.GetMaxWateringTime())
      return TheApp->GetControlState();

    if(TheApp->LButton.CheckPressedAndReset() || TheApp->CButton.CheckPressedAndReset() || TheApp->RButton.CheckPressedAndReset())
      return TheApp->GetControlState();

    if (GetDuration() <= TheApp->Settings.GetMinWateringInterval())
      return this;

    if (TheApp->MoistureSensor.GetValue() <= TheApp->Settings.GetMoistureStopTrigger())
      return TheApp->GetControlState();
  }
  virtual void OnEnter()
  {
    TimedState::OnEnter();
    TheApp->Pump.Set(true);
  }
  virtual void OnExit()
  {
    TheApp->Pump.Set(false);
    TimedState::OnExit();
  }
};

class SettingsPageBase : public State
{
public:
  virtual const char * GetMessage() const = 0;
};

template <const char* TMessage, typename T, T TMin, T TMax, T TStep, T (SettingsContainer::*TGetter)() const, void (SettingsContainer::*TSetter)(const T&)>
class NumberSettingPage : public SettingsPageBase
{
  long _lastActionTime;
  T _current;
public:

  virtual State* Update()
  {
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print(TMessage);
    TheApp->lcd.setCursor(0, 1);
    TheApp->lcd.print(_current);
    TheApp->lcd.print("   ");

    if (TheApp->CButton.CheckPressedAndReset()) return TheApp->GetSettingsState();
    else if (TheApp->LButton.CheckPressedAndReset())
    { 
      _current -= TStep;
      if (_current < TMin) _current = TMin;
    }
    else if (TheApp->RButton.CheckPressedAndReset())
    { 
      _current += TStep;
      if (_current > TMax) _current = TMax;
    }
  }

  virtual void OnEnter()
  {
    _current = (TheApp->Settings.*TGetter)();
  }

  virtual void OnExit()
  {
    (TheApp->Settings.*TSetter)(_current);
  }

  virtual const char * GetMessage() const
  {
    return TMessage;
  }
};

char StartThresholdName[] = "Start Moisture  ";
char StopThresholdName[]  = "Stop Moisture  ";

class SettingsMainPageState : public State
{
  SettingsPageBase** _settings;
  int _maxSettings;
  int _currentSelection;

public:

  SettingsMainPageState()
  {
    _maxSettings = 2;
    _settings = new SettingsPageBase*[_maxSettings];

    _settings[0] = new NumberSettingPage<StartThresholdName, int, 0, 1023, 4, &SettingsContainer::GetMoistureStartTrigger, &SettingsContainer::SetMoistureStartTrigger>();
    _settings[1] = new NumberSettingPage<StopThresholdName,  int, 0, 1023, 4, &SettingsContainer::GetMoistureStopTrigger, &SettingsContainer::SetMoistureStopTrigger>();
  }

  virtual State* Update()
  {
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("ABCDEF         S");
    
    TheApp->lcd.setCursor(0, 1);

    if (_currentSelection >= 0 && _currentSelection < _maxSettings)
    {
      TheApp->lcd.print(_settings[_currentSelection]->GetMessage());
      if (TheApp->CButton.CheckPressedAndReset()) return _settings[_currentSelection];
      TheApp->lcd.setCursor(_currentSelection, 0);

    }
    else
    {
      TheApp->lcd.print("Save          ");
      // TODO: Call Save to EEProm
      if (TheApp->CButton.CheckPressedAndReset()) return TheApp->GetControlState();
      TheApp->lcd.setCursor(15, 0);
    }

    if (TheApp->LButton.CheckPressedAndReset())
    {
      --_currentSelection;
      if (_currentSelection < 0) _currentSelection = _maxSettings;
    }
    else if (TheApp->RButton.CheckPressedAndReset())
    {
      ++_currentSelection;
      if (_currentSelection > _maxSettings) _currentSelection = 0; 
    }

    return this;
  }
  virtual void OnEnter()
  {
    _currentSelection = 0;
    TheApp->lcd.blink();
  }

  virtual void OnExit()
  {
    TheApp->lcd.noBlink();
  }
};

inline Application::Application() : 
  _wateringState(new WateringState()),
  _controlState (new ControlState()),
  _settingsState(new SettingsMainPageState()),
  lcd(9, 11, 2, 3, 4, 5)
{
  lcd.begin(16, 2);
  _current = _controlState;
}

void setup() {
  TheApp = new Application();
}

void loop() {
  TheApp->Update();
}

