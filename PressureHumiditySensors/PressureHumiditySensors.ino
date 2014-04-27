#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <LiquidCrystal.h>
#include <DHT.h>

DHT dht(A0, DHT22);
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2, 4, 9, 10, 11, 12);

Adafruit_BMP085 bmp;
const int analogInPin = A2;
  
void setup() {
  dht.begin();
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
    lcd.print("Hello!");
  if (!bmp.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("Error :(");
    while (1) {}
  }
}
  
void loop() {
    float t1 = bmp.readTemperature();
    float t2 = dht.readTemperature();
    float temperature = (t1+t2)/2.0f;
    float pressure = bmp.readPressure();
    float humidity = dht.readHumidity();
    int lightness = 100 - map(analogRead(analogInPin), 0, 1023, 0, 100);
    lcd.clear();
    lcd.print("T=");
    lcd.print(temperature);
    lcd.print("C");
    lcd.setCursor(10, 0);    
    lcd.print("L=");
    lcd.print(lightness);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("P=");
    lcd.print(pressure/100);
    lcd.setCursor(10, 1);    
    lcd.print("H=");
    lcd.print(humidity);
    lcd.print("%");

   delay(1000);
}
