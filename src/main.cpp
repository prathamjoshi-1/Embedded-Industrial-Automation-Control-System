#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <config.h>

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

// IR Sensor
#define IR_SENSOR D7

// Machines
#define M1 D1
#define M2 D2
#define M3 D5
#define M4 D6

int count = 0;
long totalProduction = 0;
bool lastState = HIGH;

int target = 800;

// Time
unsigned long lastMinute = 0;
unsigned long lastReset = 0;
int minuteCount = 0;

unsigned long minuteInterval = 60000;
unsigned long dayInterval = 86400000;

// Speed
int m1=0,m2=0,m3=0,m4=0;

// State
bool s1=0,s2=0,s3=0,s4=0;

// 🔥 STRING STATUS
String st1, st2, st3, st4;

// 🔥 INTERLOCK FUNCTION
void updateMachines() {

  st1="OFF"; st2="OFF"; st3="OFF"; st4="OFF";

  // M1 OFF
  if(!s1){
    analogWrite(M1,0);
    analogWrite(M2,0);
    analogWrite(M3,0);
    analogWrite(M4,0);

    st1="OFF";
    st2 = s2 ? "WAIT" : "OFF";
    st3 = s3 ? "WAIT" : "OFF";
    st4 = s4 ? "WAIT" : "OFF";
  }

  // M2 OFF
  else if(s1 && !s2){
    analogWrite(M1,0);
    analogWrite(M2,0);
    analogWrite(M3,0);
    analogWrite(M4,0);

    st1="BLOCK";
    st2="OFF";
    st3 = s3 ? "WAIT" : "OFF";
    st4 = s4 ? "WAIT" : "OFF";
  }

  // M3 OFF
  else if(s1 && s2 && !s3){
    analogWrite(M1,0);
    analogWrite(M2,0);
    analogWrite(M3,0);
    analogWrite(M4,0);

    st1="BLOCK";
    st2="BLOCK";
    st3="OFF";
    st4 = s4 ? "WAIT" : "OFF";
  }

  // M4 OFF
  else if(s1 && s2 && s3 && !s4){
    analogWrite(M1,0);
    analogWrite(M2,0);
    analogWrite(M3,0);
    analogWrite(M4,0);

    st1="BLOCK";
    st2="BLOCK";
    st3="BLOCK";
    st4="OFF";
  }

  // ALL RUN
  else {
    analogWrite(M1,m1);
    analogWrite(M2,m2);
    analogWrite(M3,m3);
    analogWrite(M4,m4);

    st1="RUN";
    st2="RUN";
    st3="RUN";
    st4="RUN";
  }

  // Send to Blynk
  Blynk.virtualWrite(V21, st1);
  Blynk.virtualWrite(V22, st2);
  Blynk.virtualWrite(V23, st3);
  Blynk.virtualWrite(V24, st4);
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR, INPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);

  EEPROM.begin(512);
  EEPROM.get(0, count);
  EEPROM.get(10, totalProduction);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Blynk.virtualWrite(V5, count);
  Blynk.virtualWrite(V7, String(count)+"/"+String(target));
  Blynk.virtualWrite(V8, 0);
  Blynk.virtualWrite(V9, totalProduction);
}

void loop() {
  Blynk.run();

  bool currentState = digitalRead(IR_SENSOR);

  // Production Count
  if(currentState==LOW && lastState==HIGH){
    count++;
    minuteCount++;
    totalProduction++;

    EEPROM.put(0,count);
    EEPROM.put(10,totalProduction);
    EEPROM.commit();

    Blynk.virtualWrite(V5,count);
    Blynk.virtualWrite(V7,String(count)+"/"+String(target));
    Blynk.virtualWrite(V9,totalProduction);

    delay(100);
  }

  lastState = currentState;

  // Per minute
  if(millis()-lastMinute>=minuteInterval){
    Blynk.virtualWrite(V8,minuteCount);
    minuteCount=0;
    lastMinute=millis();
  }

  // Daily reset
  if(millis()-lastReset>dayInterval){
    count=0;
    minuteCount=0;

    EEPROM.put(0,count);
    EEPROM.commit();

    Blynk.virtualWrite(V5,count);
    Blynk.virtualWrite(V7,"0/"+String(target));

    lastReset=millis();
  }
}

// 🔹 SPEED CONTROL
BLYNK_WRITE(V1){ m1=param.asInt(); updateMachines(); }
BLYNK_WRITE(V2){ m2=param.asInt(); updateMachines(); }
BLYNK_WRITE(V3){ m3=param.asInt(); updateMachines(); }
BLYNK_WRITE(V4){ m4=param.asInt(); updateMachines(); }

// 🔹 ON/OFF CONTROL
BLYNK_WRITE(V11){ s1=param.asInt(); updateMachines(); }
BLYNK_WRITE(V12){ s2=param.asInt(); updateMachines(); }
BLYNK_WRITE(V13){ s3=param.asInt(); updateMachines(); }
BLYNK_WRITE(V14){ s4=param.asInt(); updateMachines(); }

// 🔹 RESET
BLYNK_WRITE(V6){
  if(param.asInt()==1){
    count=0;
    minuteCount=0;

    EEPROM.put(0,count);
    EEPROM.commit();

    Blynk.virtualWrite(V5,count);
    Blynk.virtualWrite(V7,"0/"+String(target));
  }
}