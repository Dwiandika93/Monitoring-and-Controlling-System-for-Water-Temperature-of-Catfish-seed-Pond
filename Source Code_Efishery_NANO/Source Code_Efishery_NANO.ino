#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BAUD_RATE (115200)
#define ONE_WIRE_BUS 2
#define TURBIDITY_PIN A0
#define DO_PIN A1
#define PH_PIN A2

SoftwareSerial mySerial(6, 7); // RX, TX
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Main Millis
unsigned long previousMillis = 0;
//const unsigned long interval = 15UL*60UL*1000UL; //15 menit
const unsigned long interval = 30UL*60UL*1000UL; //30 menit
//const unsigned long interval = 2000UL; //2 Detik
//relay state
int A = 0; //Kekeruhan (Pompa)
int B = 0; //DO (Aerator + Pompa)
int C = 0; //Suhu (kalo dingin Heater)
int D = 0; //Suhu (kalo panas Pompa)
int E = 0; //PH UP
int F = 0; //PH DOWN
//Terminating-Millis
unsigned long T_interval_A, T_interval_B, T_interval_C, T_interval_D, T_interval_E, T_interval_F;

const int jumlahSensor = 4;
float dataSensor[jumlahSensor];
const int jumlahRelay = 5;
const int relay[jumlahRelay] = {8, 9, 10, 11, 12}; // Pompa, Aerator, Heater, Pompa Up, Pompa Down
const int sampelData = 10;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(BAUD_RATE);
  lcd.begin();
  lcd.backlight();
  sensors.begin();
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(DO_PIN, INPUT);
  pinMode(PH_PIN, INPUT);
  for(int i=0; i < jumlahRelay; i++){
    pinMode(relay[i], OUTPUT);
    digitalWrite(relay[i], HIGH);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    kekeruhanSensor();
    suhuSensor();
    oksigenSensor();
    phSensor();
    kirim();
    infoLCD();
    aktuator();
    previousMillis = currentMillis;
  }
  if((currentMillis >= T_interval_A) && A == 1){
    //Kekeruhan (Pompa)
    if(B == 0 && D == 0){
      //Apabila DO(B) dan suhu(D) sudah terpenuhi, matikan pompa
      digitalWrite(relay[0], HIGH);
      kekeruhanSensor();
      infoLCD();
      kirim();
      A = 0;
    }
    else if(B == 1 || D == 1){
      //apabila DO(B) atau suhu(D) belum terpenuhi, pompa tetap menyala
      kekeruhanSensor();
      kirim();
      infoLCD();
      A = 0;
    }
  } 
  if((currentMillis >= T_interval_B) && B == 1){
    //DO (Aerator + Pompa)
    if(A == 0 && D == 0){
      //apabila kekeruhan(A) dan suhu(D) sudah terpenuhi, matikan pompa dan aerator
      digitalWrite(relay[0], HIGH);
      digitalWrite(relay[1], HIGH);
      suhuSensor();
      oksigenSensor();
      kirim();
      infoLCD();
      B = 0;
    }
    else if(A == 1 || D == 1){
      //apabila kekeruhan(A) atau suhu(D) belum terpenuhi, matikan aerator saja
      digitalWrite(relay[1], HIGH);
      suhuSensor();
      oksigenSensor();
      kirim();
      infoLCD();
      B = 0;
    }
  }
  if((currentMillis >= T_interval_C) && C == 1){
    //Suhu (Heater)
    digitalWrite(relay[2], HIGH);
    suhuSensor();
    kirim();
    infoLCD();
    C = 0;
  }
  if((currentMillis >= T_interval_D) && D == 1){
    //Suhu (Pompa)
    if(A == 0 && B == 0){
      //apabila kekeruhan(A) dan DO(B) sudah terpenuhi, matikan pompa
      digitalWrite(relay[0], HIGH);
      suhuSensor();
      kirim();
      infoLCD();
      D = 0;
    }
    else if(A == 1 || B == 1){
      //apabila kekeruhan(A) atau DO(B) belum terpenuhi, pompa tetap menyala
      suhuSensor();
      kirim();
      infoLCD();
      D = 0;
    }
  }
  if((currentMillis >= T_interval_E) && E == 1){
    //PH Up
    digitalWrite(relay[3], HIGH);
    phSensor();
    kirim();
    infoLCD();
    E = 0;
  }
  if((currentMillis >= T_interval_F) && F == 1){
    //PH Down
    digitalWrite(relay[4], HIGH);
    phSensor();
    kirim();
    infoLCD();
    F = 0;
  }
}

void kekeruhanSensor(){
  //dataSensor[0]
  //relay[0]
  float sensorRead, voltage, ntuReg, VCC;
  float offset = 0.65;
  sensorRead = 0.0;
  for(int i = 0; i < sampelData; i++){
    VCC = readVCC() / 1000.0;
    sensorRead += analogRead(TURBIDITY_PIN) * (VCC / 1024.0);
    delay(10);
  }
  voltage = (sensorRead / (float)sampelData) + offset;
  ntuReg = abs(-7.0691 * (sq(voltage) * voltage) - 226.3 * sq(voltage) + 1341.4 * voltage - 1124.5);
  dataSensor[0] = ntuReg;
  return;
}

void oksigenSensor(){
  //dataSensor[1]
  //relay[1] dan relay[0]
  uint32_t VCC, voltage, sensorRead, DO;
  uint16_t V_saturation;
  uint8_t suhuAir = round(dataSensor[2]);
  const uint32_t CAL1_V = 944;
  const uint32_t CAL1_T = 30;
  const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};
  sensorRead = 0;
  for(int i=0; i < sampelData; i++){
    VCC = readVCC();
    sensorRead += analogRead(DO_PIN) * (VCC / 1024);
    delay(10);
  }
  voltage = sensorRead / sampelData;
  V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * suhuAir - (uint32_t)CAL1_T * 35;
  DO = (voltage * DO_Table[suhuAir] / V_saturation);
  dataSensor[1] = DO / 1000.0;
  return;
}

void suhuSensor(){
  //dataSensor[2]
  //relay[2] atau relay[0]
  float sensorRead, celcius, celciusReg;
  sensorRead = 0.0;
  for(int i = 0; i < sampelData; i++){
    sensors.requestTemperatures();
    sensorRead += sensors.getTempCByIndex(0);
    delay(10);
  }
  celcius = sensorRead / (float)sampelData;
  celciusReg = 1.0105 * celcius + 0.4231;
  dataSensor[2] = celciusReg;
  return;
}

void phSensor(){
  //dataSensor[3]
  //relay[3] atau relay[4]
  int8_t array[10];
  uint8_t n = 10;
  for (uint8_t i=0; i< 10; i++){
    array[i] = analogRead(PH_PIN);
  }
  combSort11(array, 10);
}

void combSort11(uint8_t *ar, uint8_t n){ 
  float voltage, sensorRead, VCC, ph, x, y, t;
  sensorRead = 0.0;
  uint8_t i, j, gap, swapped = 1;
  uint8_t temp;
  gap = n;
  for(int i= 0; i < sampelData; i++){
    VCC = readVCC() / 1000.0;
    sensorRead += analogRead(PH_PIN) * (VCC / 1024.0);
    delay(10);
  }
  while (gap > 1 || swapped == 1)
  {
    gap = gap * 10 / 13;
    if (gap == 9 || gap == 10) gap = 11;
    if (gap < 1) gap = 1;
    swapped = 0;
    for (i = 0, j = gap; j < n; i++, j++)
    {
      if (ar[i] > ar[j])
      {
        temp = ar[i];
        ar[i] = ar[j];
        ar[j] = temp;
        swapped = 1;
      }
    }
  VCC = readVCC() / 1000.0;
  float voltage =((float)ar[i] * (VCC / 1024.0)/8.0);
  ph = (-5.2 * voltage) + 8.2;
  dataSensor[3] = ph;
  }
  return;
}

void aktuator(){
  const float SP_TBD = 100.0,
  SP_DO = 3.0,
  SP_TEMP_L = 27.0,
  SP_TEMP_H = 30.0,
  SP_PH_L = 6.5,
  SP_PH_H = 7.5;
  float x, y, t;
  unsigned long Time_A, Time_B, Time_C, Time_D, Time_E, Time_F;
  //Aktuator kekeruhan (Pompa filter)
  if(dataSensor[0] > SP_TBD){
    x = dataSensor[0] - SP_TBD;
    t = 0.22916 * x * 18.89645;
    Time_A = millis();
    T_interval_A = (round(t)*60UL*1000UL) + Time_A;
    digitalWrite(relay[0], LOW);
    A = 1;
  }
  //Aktuator DO (Aerator dan pompa)
  if(dataSensor[1] < SP_DO){
    x = SP_DO - dataSensor[1];
    t = 13.90262 * x;
    Time_B = millis();
    T_interval_B = (round(t)*60UL*1000UL) + Time_B + 9000UL;
    digitalWrite(relay[0], LOW);
    digitalWrite(relay[1], LOW);
    B = 1;
  }
  //Aktuator Suhu (Heater)
  if(dataSensor[2] < SP_TEMP_L){
    x = SP_TEMP_L - dataSensor[2];
    t = 12.51745 * x * 36.63333;
    Time_C = millis();
    T_interval_C = (round(t)*60UL*1000UL) + Time_C;
    digitalWrite(relay[2], LOW);
    C = 1;
  }
  //Aktuator Suhu (Pompa)
  else if(dataSensor[2] > SP_TEMP_H){
    x = dataSensor[2] - SP_TEMP_H;
    t = 25.4783 * x * 34.68094;
    Time_D = millis();
    T_interval_D = (round(t)*60UL*1000UL) + Time_D;
    digitalWrite(relay[0], LOW);
    D = 1;
  }
  //aktuator PH
  if(dataSensor[3] < SP_PH_L){
    //PH UP
    x = (SP_PH_L) - dataSensor[3];
    y = 14.632 + (93.45925 * x);
    t = 178.698 + (29.26563 * y);
    Time_E = millis();
    T_interval_E = (round(t)*4UL) + Time_E;
    digitalWrite(relay[3], LOW);
    digitalWrite(relay[4], HIGH);
    E = 1;
  }
  else if(dataSensor[3] > SP_PH_H){
    //PH DOWN
    x = dataSensor[3] - (SP_PH_H);
    y = 13.2 + (13.99295 * x);
    t = 178.698 + (29.26563 * y);
    Time_F = millis();
    T_interval_F = (round(t)*4UL) + Time_F;
    digitalWrite(relay[3], HIGH);
    digitalWrite(relay[4], LOW);
    F = 1;
  }
  return;
}

void kirim(){
  String dataKirim = "";
  for(int i=0; i < jumlahSensor; i++){
    dataKirim += String(dataSensor[i],2); //mengubah float dengan 2 angka dibelakang koma menjadi string
    if(i != jumlahSensor-1){
      //menambahkan separator #
      dataKirim += '#';
    }
  }
  mySerial.print(dataKirim); //mengirim data
  mySerial.flush(); //tunggu hingga pin TX selesai mengirim data serial komunikasi
  return;
}

void infoLCD(){
    lcd.clear();
    //print suhu
    lcd.setCursor(0,0);
    lcd.print(dataSensor[2],1);
    lcd.setCursor(4,0);
    lcd.print(char(223));
    lcd.print("C");
    //print pH
    lcd.setCursor(0,1);
    lcd.print(dataSensor[3],1);
    lcd.setCursor(4,1);
    lcd.print("pH");
    //print oksigen
    lcd.setCursor(8,0);
    lcd.print(dataSensor[1],1);
    lcd.setCursor(12,0);
    lcd.print("mg/l");
    //print kekeruhan
    lcd.setCursor(8,1);
    lcd.print(round(dataSensor[0]));
    lcd.setCursor(13,1);
    lcd.print("NTU");
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
