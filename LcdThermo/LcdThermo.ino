#include <LiquidCrystal.h>
#include <OneWire.h>

LiquidCrystal lcd(12, 11, 2, 3, 4, 5, 6, 7, 8, 9);
OneWire ds(10);  // on pin 10

byte i;
byte present = 0;
byte data[12];
byte addr[8];
int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;

void setup() {
  lcd.begin(16, 1);
  lcd.clear();
  
  while(true)
  {
      if ( !ds.search(addr)) {
        ds.reset_search();
        delay(500);
        continue;
      }

      if ( OneWire::crc8( addr, 7) != addr[7]) {
        delay(500);
        lcd.print("CRC");
        continue;
      }
      break;
  }
}

void loop() {
  lcd.setCursor(0, 0);

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;
  
  if (SignBit) // If its negative
  {
    lcd.print("-");
  }
  lcd.print(Whole);
  lcd.print(".");
  if (Fract < 10)
  {
     lcd.print("0");
  }
  lcd.print(Fract);
  lcd.print("   ");
}

