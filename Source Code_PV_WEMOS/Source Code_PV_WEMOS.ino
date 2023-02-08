#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define BAUD_RATE (115200)
#define WIFI_SSID "ROSELIN" //nama wifi
#define WIFI_PASSWORD "shukchen75" //password wifi
#define FIREBASE_HOST "e-fishery-monitoring-1c51b-default-rtdb.firebaseio.com" //alamat firebase hapus https://
#define FIREBASE_AUTH "eAZIbNlqtpKmeRWB8aKTQS7jsgNUr3zWRXR1j7Kk" //secret code firebase

SoftwareSerial mySerial(D7, D6); //Wemos D1 mini RX pin D7, TX pin D6 (Hubungkan 6 -> D6, 7 -> D7)
WiFiClientSecure client;

//https://docs.google.com/spreadsheets/d/1bZq5IeDESk9_nmQrAeWQvxbEfW0TauqnkG56HOGGBaQ/edit#gid=0
//https://script.google.com/macros/s/AKfycbxznjvQBaZtZjeeLImyvoO5Wd98Kl2JKxqrBaer-kJmXPH0qyf79kspmgwpDxpg2ctMWw/exec
//AKfycbxznjvQBaZtZjeeLImyvoO5Wd98Kl2JKxqrBaer-kJmXPH0qyf79kspmgwpDxpg2ctMWw
// Last edit and deployment 21-08-2021 -- Mantap
const String host = "script.google.com";
const int httpsPort = 443; //port SSL koneksi website
const String GAS_ID = "AKfycbxptyAvO54TmL6i2jqC-hkVAiawhRT9I6dGyLFa70Zh9dMRXe74zXmsxr-hwijyNuYXvA"; //sesuaikan dengan ID deployment spreadsheet yang dituju

const int jumlahSensor= 4; 
float dataSensor[jumlahSensor];
String dataSerial;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(BAUD_RATE);
  Serial.begin(BAUD_RATE);
  wifiMulai();
}

void loop() {
  // put your main code here, to run repeatedly:
  terima();
  dataLog();
}

void terima(){
  dataSerial = ""; //mengosongkan variabel untuk data serial komunikasi
  while(mySerial.available()){
    dataSerial += char(mySerial.read());
  }
  if(dataSerial != ""){
    Serial.print("Terima dari Nano = ");
    Serial.println(dataSerial);
    char temp[dataSerial.length()+1]; //+1 untuk null termination
    dataSerial.toCharArray(temp, dataSerial.length()+1); //konversi string dataSerial ke array of char temp
    char* value = strtok(temp, "#"); //memisah simbol # dari awal karakter menggunakan null termination
    for(int i=0; i < jumlahSensor; i++){
      if(value != NULL){
        dataSensor[i] = atof(value); //mengubah tipe data char* ke float
        value = strtok(NULL, "#"); //memisah simbol # dari null termination menggunakan null termination
      }
    }
  }
  return;
}

void dataLog(){
  if(dataSerial != ""){
    const String namaSensor[jumlahSensor] = {"CurrentPV", "CurrentLoad", "VoltPV", "VoltLoad", "EnergiPV", "EnergiLoad"};
    String url;
    if(client.connect(host, httpsPort)){
      url = "/macros/s/" + GAS_ID + "/exec?";
      for (int i=0; i < jumlahSensor; i++){
        url += namaSensor[i]+"="+String(dataSensor[i]);
        if(i != jumlahSensor-1){
          url += "&";
        }
      }
      Serial.println(url);
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "User-Agent: BuildFailureDetectorESP8266\r\n" +
             "Connection: close\r\n\r\n");
    }
    client.stop();
  }
  return;
}

void wifiMulai(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //inisialisasi nama dan password jaringan
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500); //tunggu sampai terhubung ke jaringan
  }
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true); 
  client.setInsecure();
  return;
}
