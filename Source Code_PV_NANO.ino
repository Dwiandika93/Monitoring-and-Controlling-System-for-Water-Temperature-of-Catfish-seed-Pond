#include <SoftwareSerial.h>

#define BAUD_RATE (115200)

SoftwareSerial mySerial(6, 7); // RX, TX

unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;
unsigned long interval =  30UL * 60UL * 1000UL; //30 menit
unsigned long interval2 = 100;
//unsigned long interval = 10 * 1000UL;

const int jumlahSensor= 4; //"Current PV", "Current Load", "Volt PV", "Volt Load"
float dataSensor[jumlahSensor + 2];
const int sampelData = 10;

/*
 * ARUS_PV_PIN A0 14
 * ARUS_LOAD_PIN A1 15
 * VOLT_PV_PIN A2 16
 * VOLT_LOAD_PIN A3 17
 */

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(BAUD_RATE);
  Serial.begin(BAUD_RATE);
  for(int i= 14; i <= 17; i++){
    pinMode(i, INPUT);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    kirim();
    previousMillis = currentMillis;
  }
  if(currentMillis - previousMillis2 >= interval2){
    sensorArus();
    sensorTegangan();
    energi();
    previousMillis2 = currentMillis;
  }
}

void sensorArus(){
  const float multiplier = 0.1;
  const float voltOffset = 2.31;
  float sensorRead, current, currentReg, VCC;
  for(int ARUS_PIN= 14; ARUS_PIN <= 15; ARUS_PIN++){
    sensorRead = 0.0;
    for(int i= 0; i < sampelData; i++){
      VCC = readVCC() / 1000.0;
      sensorRead += analogRead(ARUS_PIN) * (VCC / 1024.0);
      delay(10);
    }
    current = abs(((sensorRead / (float)sampelData) - voltOffset) / multiplier);
    currentReg = 1.0099386 * current;
    dataSensor[ARUS_PIN - 14] = currentReg;
  }
  return;
}

void sensorTegangan(){
  const float R1 = 30000.0;
  const float R2 = 7500.0;
  float sensorRead, voltage, voltageReg, VCC;
  for(int VOLT_PIN= 16; VOLT_PIN <= 17; VOLT_PIN++){
    sensorRead = 0.0;
    for(int i= 0; i < sampelData; i++){
      VCC = readVCC() / 1000.0;
      sensorRead += analogRead(VOLT_PIN) * (VCC / 1024.0);
      delay(10);
    }
    voltage = (sensorRead / (float)sampelData) / (R2 / (R1 + R2));
    voltageReg = 0.99014 * voltage;
    dataSensor[VOLT_PIN - 14] = voltageReg;
  }
  return;
}

void energi(){
  dataSensor[4] = dataSensor[4] + dataSensor[0] * dataSensor[2] * 0.5;
  dataSensor[5] = dataSensor[5] + dataSensor[1] * dataSensor[3] * 0.5;
}

void kirim(){
  String dataKirim = "";
  for(int i=0; i < jumlahSensor; i++){
    dataKirim += String(dataSensor[i],2); //mengubah float dengan 2 angka dibelakang koma menjadi string
    //menambahkan separator #
    if(i != jumlahSensor-1){
      //menambahkan separator #
      dataKirim += '#';
    }
  }
  mySerial.print(dataKirim); //mengirim data
  mySerial.flush(); //tunggu hingga pin TX selesai mengirim data serial komunikasi
  return;
}

long readVCC(){
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}
