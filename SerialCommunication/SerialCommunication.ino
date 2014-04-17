const int LED = 13;

void setup(){
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(LED, OUTPUT);
}

void loop(){
  if (Serial.available() < 2)
    return;
  int sync = Serial.read();
  if (sync != 'C'){
    flushCom();
    return;
  }
  int command = Serial.read();
  if ( command == 'P')
      printLineCommand();
  else if ( command == 'B')
      pageBreakCommand();
  else{
    Serial.write("WRONG COMMAND!");
    flushCom();
  }
  Serial.write("READY!"); 
}

void flushCom(){
  while(Serial.available() > 0)
    Serial.read();
  Serial.flush();
}

void printLineCommand(){
 digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  Serial.write("PrintOk!");
}

void pageBreakCommand(){
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
  Serial.write("BreakOK!");
}
