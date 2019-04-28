#include <TimerOne.h>
#include <Time.h>
#include <TimeLib.h>
#include <SevSeg.h>
#include <EEPROM.h>
#include "pitch.h"

boolean isValidData(char c, boolean onlyNumbers);
boolean isValidTime(String s);
boolean syncDelay(int ms);
void beeping(int pin, int times, int j);
void updateTime();
void clearMemoryData();
void loadMemoryData();
void saveMemoryData();
void turnDisplay();
void checkButton();
void checkStatus();
void set_clock();
void nyanCat();
int setAlarm();

//void flashMessage(String msg);
 
const int BUZZ_Pin = A0;
const int BUTTON_Pin = A1;
const int BATT_Pin = A2;
const int BT_Pin = A3;

boolean button_pressed = false;
boolean button_released = false;

byte numDigits = 4;
byte digitPins[] = {13, 12, 11, 10};
byte segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9};

int timer = -1;
SevSeg sevseg;
time_t t;

unsigned long batteryLife = 0;
boolean display_status = false;
boolean alarm_status = false;
byte h, m, s;
byte alarmHour = 9;


void setup() 
{
  pinMode(BT_Pin, OUTPUT);
  pinMode(BUZZ_Pin, OUTPUT);
  pinMode(BUTTON_Pin, INPUT);  
  digitalWrite(BT_Pin, HIGH);

  Serial.begin(57600);
  Serial.setTimeout(200);
  Timer1.initialize();

  loadMemoryData();
  turnDisplay();   // erase this shit

  Timer1.attachInterrupt(updateTime);
  Timer1.start();  
  delay(100);
  
}

int c;
//
void loop() 
{  
  if (display_status) 
    sevseg.refreshDisplay();
}


void updateTime() {
  
  t = now();
  h = hour(t);
  m = minute(t);
  s = second(t);

  if (display_status) 
    sevseg.setNumber(h * 100 + m, 2);
    
  if (alarmHour == h && s==0 && alarm_status)
    beeping(BUZZ_Pin, 4, 3);
}


void serialEvent() 
{  
  char serial_data[20] = {}; 
  Serial.readBytes(serial_data, 20);
    
  if (compareData(serial_data, "CMD")) {
    Serial.println();
    Serial.println(F("Commands:"));
    Serial.println(F("-SETTIME0000"));
    Serial.println(F("-SETALARM0000"));
    Serial.println(F("-SETBRIGHTNESS000"));
    Serial.println(F("-TOOGLEALARM"));
    Serial.println(F("-TOOGLEDISPLAY"));
    Serial.println(F("-CLEARSAVEDDATA"));
    Serial.println(F("-BATTERYLIFE"));
    Serial.println(F("-NYANCAT"));
  }
  else if (compareData(serial_data, "SETTIME")) {
    String inputTime = String(serial_data).substring(7,11);
    if(isValidTime(inputTime)) {
      setTime((int)((inputTime[0]-48)*10)+(inputTime[1]-48), (int)((inputTime[2]-48)*10)+(inputTime[3]-48), 0, 0, 0, 0);
      Serial.println();
      Serial.println(F("Done"));
      saveMemoryData();
    }
    else {
      Serial.println();
      Serial.println(F("Invalid Time"));
    }
  }
  else if (compareData(serial_data, "SETALARM")) {
    String inputTime = String(serial_data).substring(8,12);
    if(isValidTime(inputTime)) {
      alarmHour = inputTime.substring(0,2).toInt();
      alarm_status = true;
      Serial.println();
      Serial.println(F("Done"));
      saveMemoryData();
    }
    else {
      Serial.println();
      Serial.println(F("Invalid Time"));
    }
  }  else if (compareData(serial_data, "SETBRIGHTNESS")) {
    int input = (String(serial_data).substring(13,16)).toInt();
    if(input>0 && input<=100) {
      sevseg.setBrightness(input);
      Serial.println();
      Serial.println("Done");
      saveMemoryData();
    }
    else {
      Serial.println();
      Serial.println(F("Invalid parameter."));
      Serial.println(F("Brightness values range between 1 and 100."));
    }
  }
  else if (compareData(serial_data, "TOOGLEALARM")) {
    alarm_status = !alarm_status;
    if (alarm_status == true) {
      Serial.println();
      Serial.println(F("Alarm ON"));
    }
    else {
      Serial.println();
      Serial.println(F("Alarm OFF"));
    }
    saveMemoryData();
  }
  else if (compareData(serial_data, "TOOGLEDISPLAY")) {     
    display_status = !display_status;
    if (display_status == true) {
      sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
      Serial.println();
      Serial.println("Display ON");
    }
    else {
      sevseg.blank();
      Serial.println();
      Serial.println(F("Display OFF"));
    }
    saveMemoryData();
  }
  else if (compareData(serial_data, "CLEARSAVEDDATA")) {     
    clearMemoryData();
    Serial.println();
    Serial.println(F("EEPROM data erased."));
  }
  else if (compareData(serial_data, "BATTERYLIFE")) {      
    batteryLife = 0;
    for (int i=0;i<3000;i++) {
      batteryLife+= analogRead(BATT_Pin);
    }
    batteryLife = map((batteryLife/3000), 0, 1023, 0, 500);
    Serial.println();
    Serial.print(F("Battery at ")); 
    Serial.println(String(batteryLife/100) + "." + String((batteryLife%100)/10) + String(batteryLife%10) + " v");
  }
  else if (compareData(serial_data, "NYANCAT")) {
    nyanCat();
  }
}


unsigned long prevTime = 0;

boolean syncDelay(int ms) {
  if (prevTime == 0) { 
    prevTime = millis();
    return false;
  }
  else if (millis() > prevTime+ms) {
    prevTime = 0;
    return true; 
  }
  else {
    return false;
  }
}

void turnDisplay() {
  if (display_status == true) 
    sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
}

boolean compareData(char data[], String t) {
  for (int i=0; i<(sizeof(t)/sizeof(t[0])+1); i++) {
    if (data[i] != t[i]) {
      return false;
    }
  } 
  return true;
}

boolean isValidData(char c, boolean onlyNumbers) {
  if (onlyNumbers) {
    if (c>='0' && c<='9')
      return true;
    else
      return false;
  }
  else {
    if (c>31 && c<126)
      return true;
    else
      return false;
  }
}



boolean isValidTime(String s) {
  if (s[0]<'0'||s[0]>'2') return false;
  if (s[0]=='2'&&s[1]>='4') return false;
  if (!isValidData(s[1], true)) return false;
  if (s[2]<'0'||s[2]>'5') return false;
  if (!isValidData(s[3], true)) return false;
  return true;
}


void saveMemoryData() {
  noInterrupts(); 
  EEPROM.update(0, h);
  EEPROM.update(1, m);
  EEPROM.update(2, s);
  EEPROM.update(3, alarmHour);
  EEPROM.update(4, (byte)display_status);
  EEPROM.update(5, (byte)alarm_status);
  interrupts();
}

void loadMemoryData() {
  noInterrupts();
  h = EEPROM.read(0);
  m = EEPROM.read(1);
  s = EEPROM.read(2);
  alarmHour = EEPROM.read(3);
  display_status = (boolean) EEPROM.read(4);
  alarm_status = (boolean) EEPROM.read(5);
  interrupts();
  setTime(h, m, s, 0, 0, 0);
}

void clearMemoryData() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}


void checkButton() {

  int button_status = digitalRead(BUTTON_Pin);
  button_released = LOW;
  
  if (button_status==HIGH) {
    button_pressed = true;
    if (timer == -1) timer = m*60+s;
  }
  else if (button_status==LOW && button_pressed) {
    button_released = true;
    button_pressed = false;
    timer = -1;
  }
  else {
    button_pressed = false; 
  }
    
  if (button_released)
    checkStatus();
    
  if (timer!=-1 &&((m*60+s)-timer) > 3) {
    setAlarm();
    timer = -1;
    button_pressed = false;
    button_released = false;
  }
  
  if(h==alarmHour && alarm_status) {
    if (s == 0) {
      beeping(BUZZ_Pin, 5, 3);
    }
  }
}



void beeping(int pin, int times, int j) {
  for (int n = 0; n < times; n++) {
    for (int i = 0; i < j; i++) {
      digitalWrite(pin, HIGH);
      delay(100);
      digitalWrite(pin, LOW);
      delay(100);
    }
  delay(200*j);
  }
}

/*

void checkStatus() {
  
  long temp;
  if (alarm_status == true) {
    alarm_status = false;
    temp = millis();
    while(millis()-temp < 400) {
      sevseg.setChars(" Off");
      sevseg.refreshDisplay(); 
    }
  }
  else {
    alarm_status = true;
    temp = millis();
    digitalWrite(BUZZ_Pin, HIGH);
    while(millis()-temp < 100) {
      sevseg.setChars(" On ");
      sevseg.refreshDisplay(); 
    }
   digitalWrite(BUZZ_Pin, LOW);
   temp = millis();
   while(millis()-temp < 300) {
     sevseg.setChars(" On ");
     sevseg.refreshDisplay(); 
    }
  }
}
*/

/*
void set_clock() {
  
  int process = 0;
  int hours = 0;
  int minutes = 0;
  int msAtStart;
  int count = 0;
  String value;
  char formatted[5];
  boolean pressed = false;
  boolean released = false;

  while(!released) {
    int n = digitalRead(BUTTON_Pin);
    released = false;
    if (n==1) pressed = true;
    else if (n==0 && pressed) {
      released = true;
      pressed = false;
    }
    else {
      pressed = false; 
    }
    sevseg.setChars("Set ");
    sevseg.refreshDisplay(); 
  }
  
  msAtStart = millis();
  while(process != 2) {
    int n = digitalRead(BUTTON_Pin);
    released = false;
    if (n==1) pressed = true;
    else if (n==0 && pressed) {
      released = true;
      pressed = false;
    }
    else {
      pressed = false; 
    }
    count = millis() - msAtStart;
    if (process==0 && released) {
      hours++;
      msAtStart = millis();
      if (hours == 24) hours = 0;
    }
    else if (process==1 && released) {
      minutes++;
      msAtStart = millis();
      if (minutes == 60) minutes = 0; 
    }
    if (count>3000) {
      process++;
      msAtStart = millis();
    }
    value = String(hours/10,DEC)+String(hours%10,DEC)+String(minutes/10,DEC)+String(minutes%10,DEC);
    value.toCharArray(formatted,5);
    if (formatted[0]=='0') formatted[0]=' ';
    if (count%1000 > 500) {
     if (process == 0) {
      formatted[0] = ' ';
      formatted[1] = ' ';
     }
     else if (process == 1) {
      formatted[2] = ' ';
      formatted[3] = ' ';
     }
    }
    sevseg.setChars(formatted);
    sevseg.refreshDisplay(); 
    }
  setTime(hours, minutes, 0, 0, 0, 0);
}
*/

/*
int setAlarm() {
  
  int process = 0;
  int hours = 0;
  long msAtStart;
  int count = 0;
  String value;
  char formatted[5];
  boolean pressed = false;
  boolean released = false;
  boolean stuck = true;

  while(!released) {
    int n = digitalRead(BUTTON_Pin);
    if (n==0) stuck = false;
    if (!stuck) {
    released = false;
    if (n==1) pressed = true;
    else if (n==0 && pressed) {
      released = true;
      pressed = false;
    }
    else {
      pressed = false; 
    }
    }
    sevseg.setChars("Set ");
    sevseg.refreshDisplay(); 
  }
  
  msAtStart = millis();
  while(process != 1) {
    int n = digitalRead(BUTTON_Pin);
    released = false;
    if (n==1) pressed = true;
    else if (n==0 && pressed) {
      released = true;
      pressed = false;
    }
    else {
      pressed = false; 
    }
    count = millis() - msAtStart;
    if (process==0 && released) {
      hours++;
      msAtStart = millis();
      if (hours == 24) hours = 0;
    }
    if (count>5000) {
      process++;
      msAtStart = millis();
    }
    value = String(hours/10,DEC)+String(hours%10,DEC)+String(0,DEC)+String(0,DEC);
    value.toCharArray(formatted,5);
    if (formatted[0]=='0') formatted[0]=' ';
    if (count%1000 > 500) {
     if (process == 0) {
      formatted[0] = ' ';
      formatted[1] = ' ';
      formatted[2] = ' ';
      formatted[3] = ' ';
     }
    }
    sevseg.setChars(formatted);
    sevseg.refreshDisplay(); 
    }
  alarmHour = hours;
}
*/

void nyanCat() {  
  int tempo = 700;
  int melody_A[] = { 
    NOTE_FS6, NOTE_GS6, NOTE_DS6, NOTE_DS6, 0, NOTE_B5, NOTE_D6, NOTE_CS6, NOTE_B5, 0, NOTE_B5, NOTE_CS6,
    NOTE_D6, NOTE_D6, NOTE_CS6, NOTE_B5, NOTE_CS6, NOTE_DS6, NOTE_FS6, NOTE_GS6, NOTE_DS6, NOTE_FS6, NOTE_CS6, NOTE_DS6, NOTE_B5, NOTE_CS6, NOTE_B5,
    NOTE_DS6, NOTE_FS6, NOTE_GS6, NOTE_DS6, NOTE_FS6, NOTE_CS6, NOTE_DS6, NOTE_B5, NOTE_D6, NOTE_DS6, NOTE_D6, NOTE_CS6, NOTE_B5, NOTE_CS6,
    NOTE_D6, NOTE_B5, NOTE_CS6, NOTE_DS6, NOTE_FS6, NOTE_CS6, NOTE_DS6, NOTE_CS6, NOTE_B5, NOTE_CS6, NOTE_B5, NOTE_CS6 
    };
  int rthm_A[] = { 
    4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4,
    4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    4, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4
    };
  int melody_B[] = {
    NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_CS6, NOTE_DS6, NOTE_B5, NOTE_E6, NOTE_DS6, NOTE_E6, NOTE_FS6, 
    NOTE_B5, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_FS5, NOTE_E6, NOTE_DS6, NOTE_CS6, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_E5, NOTE_FS5, 
    NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_B5, NOTE_CS6, NOTE_DS6, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_FS5
    };
  int rthm_B[] = { 
    4, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    4, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
    };
  int melody_B1[] = {
    NOTE_B5, NOTE_B5, NOTE_AS5, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_E6, NOTE_DS6, NOTE_E6, NOTE_FS6, NOTE_B5, NOTE_AS5
    };
  int melody_B2[] = {
    NOTE_B5, NOTE_B5, NOTE_AS5, NOTE_B5, NOTE_FS5, NOTE_GS5, NOTE_B5, NOTE_E6, NOTE_DS6, NOTE_E6, NOTE_FS6, NOTE_B5, NOTE_CS6
    };
  int rthm_B2[] = { 
    4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4,
    };

  for (int i = 0; i < (sizeof(melody_A)/sizeof(melody_A[0])); i++) {
    int rthm = tempo / rthm_A[i];
    tone(BUZZ_Pin, melody_A[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);
  }
  for (int i = 0; i < (sizeof(melody_A)/sizeof(melody_A[0])); i++) {
    int rthm = tempo / rthm_A[i];
    tone(BUZZ_Pin, melody_A[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);
  }
  for (int i = 0; i < (sizeof(melody_B)/sizeof(melody_B[0])); i++) {
    int rthm = tempo / rthm_B[i];
    tone(BUZZ_Pin, melody_B[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);
  }
  for (int i = 0; i < (sizeof(melody_B1)/sizeof(melody_B1[0])); i++) {
    int rthm = tempo / rthm_B2[i];
    tone(BUZZ_Pin, melody_B1[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);
  }
  for (int i = 0; i < (sizeof(melody_B)/sizeof(melody_B[0])); i++) {
    int rthm = tempo / rthm_B[i];
    tone(BUZZ_Pin, melody_B[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);
  }
  for (int i = 0; i < (sizeof(melody_B2)/sizeof(melody_B2[0])); i++) {
    int rthm = tempo / rthm_B2[i];
    tone(BUZZ_Pin, melody_B2[i], rthm);
    int pauseBetweenNotes = rthm * 1.30;
    delay(pauseBetweenNotes);
    noTone(8);  
  }
}

