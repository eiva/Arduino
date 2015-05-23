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
#include <EEPROM.h>

//#define bool boolean

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

template<int TPin, bool TPressedLowLevel = false, int TThreshold = 20, int TRepeat = 150, int TRepCount = 3>
class Button
{
  long _switchTime;
  bool _isDown;
  bool _isUp;
  bool _isPressed;
  long _downTime;
public:
  Button() :
   _switchTime(0),
   _downTime(0),
   _isUp(false),
   _isDown(false),
   _isPressed(false)
   {
    pinMode(TPin, INPUT);
    digitalWrite(TPin, TPressedLowLevel ? HIGH : LOW);
   }
   void Update()
   {
     bool level = digitalRead(TPin) == HIGH ? true : false;
     if (TPressedLowLevel) level != level;

     const long time = millis();
     if (level && !_isDown && !_isUp)
     {
      // Transition to pressed
      _isDown = true;
      _downTime = time;
      _switchTime = time;
     }
     else if (!level && _isDown && !_isUp)
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
     if (_isDown && _isUp)
     {
       _isPressed = true;
     }
     if (level && _isDown && !_isUp && time - _switchTime > TThreshold + TRepeat * TRepCount && time - _downTime > TRepeat)
     {
       _isPressed = true;
       _downTime = time;
     }

   }

   bool CheckPressedAndReset()
   {
    if (_isPressed)
    {
      _isPressed = _isUp = false;
      return true;
    }
    return false;
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
  const int _currentVersion;
  int _minWateringTime;
  int _maxWateringTime;
  int _moistureStart;
  int _moistureStop;
  int _wateringInterwal;
  int _light;
public:
  SettingsContainer() :
   _minWateringTime(2000),
   _maxWateringTime(5000),
   _moistureStart(900),
   _moistureStop(400),
   _wateringInterwal(20000),
   _light(500),
   _currentVersion(1)
  {
    int magic = read(0);
    int ver = read(1);
    if (magic != 332 || ver != _currentVersion) return; // Bad EEProm state.
    _minWateringTime = read(2);
    _maxWateringTime = read(3);
    _moistureStart = read(4);
    _moistureStop = read(5);
    _wateringInterwal = read(6);
    _light = read(7);
  }

  int GetMinWateringTime() const { return _minWateringTime; }
  void SetMinWateringTime(const int& val){ _minWateringTime = val; }

  int GetMaxWateringTime() const { return _maxWateringTime; }
  void SetMaxWateringTime(const int& val){ _maxWateringTime = val;  }

  int GetMoistureStartTrigger() const { return _moistureStart; } // Maximum allowed resistance of sensor to start watering.
  void SetMoistureStartTrigger(const int& val){ _moistureStart = val; }

  int GetMoistureStopTrigger() const { return _moistureStop; }
  void SetMoistureStopTrigger(const int& val){ _moistureStop = val; }

  long GetMinWateringInterval() const { return _wateringInterwal; }
  void SetMinWateringInterval(const long& val){ _wateringInterwal = val; }

  int GetMutetModeLightingSensor() const { return _light; } // Resistance of light sensor exciding which whatering is forbidden.
  void SetMutetModeLightingSensor(const int& val){ _light = val; }

  void Save()
  {
    write(0, 332);
    write(1, _currentVersion);
    write(2, _minWateringTime);
    write(3, _maxWateringTime);
    write(4, _moistureStart);
    write(5, _moistureStop);
    write(6, _wateringInterwal);
    write(7, _light);
  }
private:
  void write(int p_address, int p_value)
  {
    p_address *= 2;
    byte lowByte = ((p_value >> 0) & 0xFF);
    byte highByte = ((p_value >> 8) & 0xFF);

    EEPROM.write(p_address, lowByte);
    EEPROM.write(p_address + 1, highByte);
   }

  //This function will read a 2 byte integer from the eeprom at the specified address and address + 1
  int read(int p_address)
  {
      p_address *= 2;
      int lowByte = EEPROM.read(p_address);
      int highByte = EEPROM.read(p_address + 1);

      unsigned int r = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
      return r;
   }

};

// Base class for state representation.
class State
{
public:
  State(){};
  virtual ~State(){};
  virtual void OnEnter();
  virtual void OnExit();
  virtual State* Update() = 0;
};

class Application
{
  State* _current; // Currently active state.

  State *_controlState;
  State *_wateringState;
  State *_settingsState;

public:
  Application();
  ~Application()
  {
    delete _controlState;
    delete _wateringState;
    delete _settingsState;
  }

  void Update()
  {
    MoistureSensor.Update();
    LButton.Update();
    CButton.Update();
    RButton.Update();
    
    if (_current == 0)
    {
      _current = _controlState;
      _current->OnEnter();
    }

    State* newState = _current->Update();
    if (newState != _current)
    {
      _current->OnExit();
      newState->OnEnter();
      _current = newState;
    }
  }

  State *GetControlState()  const { return _controlState; }
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
    State::OnEnter();
    _enteringTime = millis();
  }
  virtual void OnExit()
  {
    _previousLeaveTime = millis();
    State::OnExit();
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
    
    
    TheApp->lcd.setCursor(8, 1);
    TheApp->lcd.print(GetDuration() / 1000);
    TheApp->lcd.print("   ");
    
    if (TheApp->MoistureSensor.GetValue() == 1023) // Cut off
    {
      TheApp->lcd.setCursor(8, 0);
      TheApp->lcd.print(" CUT OFF");
    }
    else
    {
      TheApp->lcd.setCursor(8, 0);
      TheApp->lcd.print(TheApp->MoistureSensor.GetValue());
      TheApp->lcd.print("   ");
       
      if (TheApp->MoistureSensor.GetValue() >= TheApp->Settings.GetMoistureStartTrigger() )
        if (GetDuration() > TheApp->Settings.GetMinWateringInterval())
          return TheApp->GetWateringState();
    }

    if (TheApp->CButton.CheckPressedAndReset())
      return TheApp->GetSettingsState();

    return this;
  }
  virtual void OnEnter()
  {
    TimedState::OnEnter();
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("Control: ");
    TheApp->lcd.setCursor(0, 1);
    TheApp->lcd.print("Last: ");
  }
};

class WateringState : public TimedState
{
public:
  virtual State* Update()
  {
    TheApp->lcd.setCursor(0, 1);
    TheApp->lcd.print(TheApp->MoistureSensor.GetValue());
    TheApp->lcd.print("   ");
    TheApp->lcd.setCursor(8, 1);
    TheApp->lcd.print(TheApp->Settings.GetMoistureStopTrigger());
    TheApp->lcd.print("   ");
    
    if (TheApp->MoistureSensor.GetValue() == 1023) // Cut off
      return TheApp->GetControlState();

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
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("!!Pump Active!!");
    
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

template <typename T, T TMin, T TMax, T TStep, T (SettingsContainer::*TGetter)() const, void (SettingsContainer::*TSetter)(const T&)>
class NumberSettingPage : public SettingsPageBase
{
  long _lastActionTime;
  T _current;
  const char* _message;
public:

  NumberSettingPage(const char* message)
     : _message(message)
     , _current(TMin)
  { }
  virtual State* Update()
  {
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
    return this;
  }

  virtual void OnEnter()
  {
    SettingsPageBase::OnEnter();
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print(_message);
    _current = (TheApp->Settings.*TGetter)();
  }

  virtual void OnExit()
  {
    (TheApp->Settings.*TSetter)(_current);
    SettingsPageBase::OnExit();
  }

  virtual const char * GetMessage() const
  {
    return _message;
  }
};

class SettingsMainPageState : public State
{
  SettingsPageBase** _settings;
  const int _maxSettings;
  int _currentSelection;

public:

  SettingsMainPageState() 
    : _currentSelection(-1)
    , _maxSettings(6)
  {
    _settings = new SettingsPageBase*[_maxSettings];

    _settings[0] = new NumberSettingPage<int, 0, 1022, 4, &SettingsContainer::GetMoistureStartTrigger, &SettingsContainer::SetMoistureStartTrigger>("Start Watering");
    _settings[1] = new NumberSettingPage<int, 0, 1022, 4, &SettingsContainer::GetMoistureStopTrigger, &SettingsContainer::SetMoistureStopTrigger>("Stop Watering");
    
    _settings[2] = new NumberSettingPage<long, 10000, 40000, 5000, &SettingsContainer::GetMinWateringInterval, &SettingsContainer::SetMinWateringInterval>("Watering Interval");

    _settings[3] = new NumberSettingPage<int, 1000, 4000, 500, &SettingsContainer::GetMinWateringTime, &SettingsContainer::SetMinWateringTime>("Min Watering T");
    _settings[4] = new NumberSettingPage<int, 2000, 1000, 500, &SettingsContainer::GetMaxWateringTime, &SettingsContainer::SetMaxWateringTime>("Max Watering T");

    _settings[5] = new NumberSettingPage<int, 0, 1023, 4, &SettingsContainer::GetMutetModeLightingSensor, &SettingsContainer::SetMutetModeLightingSensor>("Light sensor");
  }

  virtual ~SettingsMainPageState()
  {
    for (int i = 0; i < _maxSettings; ++i)
    {
      delete _settings[i];
    }
    delete[] _settings;
  }

  virtual State* Update()
  {
    bool changed = false;
    if (_currentSelection < 0)
    {
      _currentSelection = _maxSettings + 1;
      changed = true;
    }
    if (TheApp->LButton.CheckPressedAndReset())
    {
      --_currentSelection;
      if (_currentSelection < 0) _currentSelection = _maxSettings + 1;
      changed = true;
    }
    else if (TheApp->RButton.CheckPressedAndReset())
    {
      ++_currentSelection;
      if (_currentSelection > _maxSettings + 1) _currentSelection = 0; 
      changed = true;
    }

    if (changed)
    {
      TheApp->lcd.setCursor(0, 1);
      if (_currentSelection >= 0 && _currentSelection < _maxSettings)
      {
        TheApp->lcd.print(_settings[_currentSelection]->GetMessage());
        TheApp->lcd.setCursor(_currentSelection, 0);
        TheApp->lcd.blink();
      }
      else if (_currentSelection == _maxSettings)
      {
        TheApp->lcd.print("Save          ");
        TheApp->lcd.setCursor(14, 0);
        TheApp->lcd.blink();
      }
      else if(_currentSelection == _maxSettings + 1)
      {
        TheApp->lcd.print("Exit          ");
        TheApp->lcd.setCursor(15, 0);
        TheApp->lcd.blink();
      }
    }
    
    if (TheApp->CButton.CheckPressedAndReset())
    {
      if (_currentSelection >= 0 && _currentSelection < _maxSettings)
      {
         return _settings[_currentSelection];
      }
      else
      {
        if (_currentSelection == _maxSettings)
        {
          TheApp->lcd.setCursor(0, 1);
          TheApp->lcd.print("Saving to EEPROM...");
          TheApp->Settings.Save();
        }
        else
        {
          TheApp->lcd.setCursor(0, 1);
          TheApp->lcd.print("Exiting...");
          delay(100);
        }
        // Reset buttons.
        TheApp->CButton.Update();
        TheApp->CButton.CheckPressedAndReset();
        return TheApp->GetControlState();
      }
    }
    
    return this;
  }
  virtual void OnEnter()
  {
    State::OnEnter();
    TheApp->lcd.setCursor(0, 0);
    TheApp->lcd.print("ABCDEF        SX");
    _currentSelection = -1;
  }

  virtual void OnExit()
  {
    TheApp->lcd.noBlink();
    State::OnExit();
  }
};

inline Application::Application() : 
  _wateringState(new WateringState()),
  _controlState (new ControlState()),
  _settingsState(new SettingsMainPageState()),
  _current(0),
  lcd(9, 11, 2, 3, 4, 5)
{
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Starting ....");
  delay(1000);
}

void State::OnEnter()
{

}
void State::OnExit()
{
  TheApp->lcd.clear();
  TheApp->lcd.noCursor();
  TheApp->lcd.noBlink();
}

void setup() {
  TheApp = new Application();
}

void loop() {
  TheApp->Update();
}

