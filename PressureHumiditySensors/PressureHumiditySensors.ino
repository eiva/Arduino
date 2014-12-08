#include <math.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <LiquidCrystal.h>

#ifndef bool
#define bool boolean
#endif

#define MAXTIMINGS 85

#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

class DHT{
private:
    uint8_t data[6];
    uint8_t _pin, _type, _count;
    unsigned long _lastreadtime;
    boolean firstreading;

    boolean read(void)
    {
      uint8_t laststate = HIGH;
      uint8_t counter = 0;
      uint8_t j = 0, i;
      unsigned long currenttime;

      // pull the pin high and wait 250 milliseconds
      digitalWrite(_pin, HIGH);
      delay(350);

      currenttime = millis();
      if (currenttime < _lastreadtime) {
        // ie there was a rollover
        _lastreadtime = 0;
      }
      if (!firstreading && ((currenttime - _lastreadtime) < 2000)) {
        return true; // return last correct measurement
        //delay(2000 - (currenttime - _lastreadtime));
      }
      firstreading = false;
    
      _lastreadtime = millis();

      data[0] = data[1] = data[2] = data[3] = data[4] = 0;
  
      // now pull it low for ~20 milliseconds
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, LOW);
      delay(10);
      cli();
      digitalWrite(_pin, HIGH);
      delayMicroseconds(20);
      pinMode(_pin, INPUT);

      // read in timings
      for ( i=0; i< MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(_pin) == laststate) {
          counter++;
          delayMicroseconds(1);
          if (counter == 255) {
            break;
          }
        }
        laststate = digitalRead(_pin);
    
        if (counter == 255) break;
    
        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) 
        {
          // shove each bit into the storage bytes
          data[j/8] <<= 1;
          if (counter > _count)
            data[j/8] |= 1;
          j++;
        }
      }

      sei();
    
      // check we read 40 bits and that the checksum matches
      if ((j >= 40) && 
          (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
        return true;
      }
      return false;
    }
public:
    DHT(uint8_t pin, uint8_t type, uint8_t count=6)
    {
      _pin = pin;
      _type = type;
      _count = count;
      firstreading = true;
    }
    void begin(void)
    {
      // set up the pins!
      pinMode(_pin, INPUT);
      digitalWrite(_pin, HIGH);
      _lastreadtime = 0;
    }
    //boolean S == Scale.  True == Farenheit; False == Celcius
    float readTemperature(bool S=false)
    {
      float f;

      if (read()) 
      {
        switch (_type) 
        {
        case DHT11:
          f = data[2];
          if(S)
            f = convertCtoF(f);
        return f;
        case DHT22:
        case DHT21:
          f = data[2] & 0x7F;
          f *= 256;
          f += data[3];
          f /= 10;
          if (data[2] & 0x80)
            f *= -1;
          if(S)
            f = convertCtoF(f);
          return f;
        }
      }
      return NAN;
    }
    float convertCtoF(float c)  {	return c * 9 / 5 + 32; }
    float readHumidity(void)
    {
      float f;
      if (read()) {
        switch (_type) {
        case DHT11:
          f = data[0];
          return f;
        case DHT22:
        case DHT21:
          f = data[0];
          f *= 256;
          f += data[1];
          f /= 10;
          return f;
        }
      }
      return NAN;
    }
};

class HumidityMeasures{
  unsigned char _chart[40];
  unsigned char _measure;
  unsigned char _maxMeasure;
  double _mean;
public:
  HumidityMeasures(unsigned char maxMeasure)
  : _maxMeasure(maxMeasure)
  , _measure(0)
  , _mean(0)
  {
    for (unsigned char i = 0; i < 40; ++i){
      _chart[i] = 0;
    }
  }
  /// Process new measure
  bool Measure(double currentVal){
    bool shifted = false;
    if (_measure >= _maxMeasure){
      shift();
      shifted = true;
    }

    if(_measure == 0){
      _mean = currentVal;
    }
    else{
      _mean = ((double)_measure*_mean + currentVal) / ((double)_measure + 1);
    }

    ++_measure;

    unsigned char newPoint = convert(_mean);
    if (newPoint != _chart[0]){
      _chart[0] = newPoint;
      return true;
    }
    return shifted;
  }
  // Gets binary symbol for LCD.
  void FillSymbol(unsigned char index, byte charecterMask[8]) const{
    for (unsigned char i = 0; i < 5; ++i){
      for (unsigned char j = 0; j < 8; ++j){
        if (j < _chart[index*5 + i]){
          charecterMask[j] |= 1 << i;
        }
        else{
          charecterMask[j] &= ~(1 << i);
        }
      }
    }
  }
private:
  unsigned char convert(double val) const{
    return round(8.0 * val / 100.0);
  }
  void shift(){
    for (unsigned char i = 38; i >= 0; --i){
      _chart[i+1] = _chart[i];
    }
    _chart[0] = 0;
  }
};

const float pas_to_mm = 760.0 / 101325.0;

DHT dht(A0, DHT22,3);
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);

Adafruit_BMP085 bmp;
//const int analogInPin = A2;
  
void setup() {
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
    lcd.print("Hello!");
  if (!bmp.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("Error :(");
    while (1) {}
  }
  dht.begin();
}
  
void loop() {
    float t1 = bmp.readTemperature();
    float t2 = dht.readTemperature();
    float temperature = (t1+t2)/2.0f;
    float pressure = bmp.readPressure() * pas_to_mm;
    float humidity = dht.readHumidity();

    lcd.clear();
    lcd.print("T=");
    lcd.print(temperature);
    lcd.print("C");
    lcd.setCursor(10, 0);    
    //lcd.print("L=");
    //lcd.print(lightness);
    //lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("P=");
    lcd.print(pressure);
    lcd.setCursor(10, 1);    
    lcd.print("H=");
    lcd.print(humidity);
    lcd.print("%");

   delay(500);
}
