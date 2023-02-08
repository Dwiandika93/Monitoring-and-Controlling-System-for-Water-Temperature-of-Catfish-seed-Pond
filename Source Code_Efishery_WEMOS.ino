#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseESP8266.h> //harus download lib ArduinoJson-5.13.5
#include <NTPClient.h>
#include <WiFiUdp.h>

#define BAUD_RATE (115200)
#define WIFI_SSID "Kolam" //nama wifi
#define WIFI_PASSWORD "1102170095" //password wifi
#define FIREBASE_HOST "e-fishery-monitoring-1c51b-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "eAZIbNlqtpKmeRWB8aKTQS7jsgNUr3zWRXR1j7Kk"

SoftwareSerial mySerial(D7, D6); // RX, TX
WiFiClientSecure client;
FirebaseData fbdo;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7UL*60UL*60UL);

//https://docs.google.com/spreadsheets/d/1cvyA6ia28rki95puxUXHlBh67el_dvr89ZVHLrRvf3M/edit?usp=sharing
//https://script.google.com/macros/s/AKfycbyrVcmScKqKT9fVh9LFVOdVJyHibvcflcfLgd5WP7UtLieBnJjh67jFq0HkEmTynz2PKQ/exec
//AKfycbyrVcmScKqKT9fVh9LFVOdVJyHibvcflcfLgd5WP7UtLieBnJjh67jFq0HkEmTynz2PKQ
//Last edit and deployment 27-07-2021 -- Mantap
const String host = "script.google.com";
const int httpsPort = 443; //port SSL koneksi website
const String GAS_ID = "AKfycbyrVcmScKqKT9fVh9LFVOdVJyHibvcflcfLgd5WP7UtLieBnJjh67jFq0HkEmTynz2PKQ"; //sesuaikan dengan ID script spreadsheet yang dituju

const int jumlahSensor= 4;
float dataSensor[jumlahSensor];
const String namaSensor[jumlahSensor] = {"Kekeruhan", "Oksigen", "Suhu", "pH"}; //sesuaikan dengan penamaan sensor yang digunakan untuk diunggah atau diunduh sebanyak "jumlahSensor" elemen
String dataSerial;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(BAUD_RATE);
  Serial.begin(BAUD_RATE);
  wifiMulai();
  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  terima();
  kirimFirebase();
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
    String url;
    if(client.connect(host, httpsPort)){
      url = "/macros/s/" + GAS_ID + "/exec?";
      for (int i=0; i < jumlahSensor; i++){
        url += namaSensor[i]+"="+String(dataSensor[i]);
        if(i != jumlahSensor-1){
          url += "&";
        }
      }
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
             "User-Agent: BuildFailureDetectorESP8266\r\n" +
             "Connection: close\r\n\r\n");
    }
    client.stop();
  }
  return;
}

void kirimFirebase(){
  if(dataSerial != ""){
    for(int i=0; i < jumlahSensor; i++){
      if(Firebase.setFloat(fbdo, "Parameter/"+namaSensor[i], dataSensor[i])){
        Serial.println("Berhasil mengirim ke firebase");
      }
      else{
        Serial.print("Eror saat mengirim data ke Firebase, ");
        Serial.println(fbdo.errorReason());
      }
    }
    timeClient.update();
    if(Firebase.setString(fbdo, "Parameter/Diperbarui", timeClient.getFormattedTime())){
      Serial.println("Berhasil mengirim ke firebase");
    }
    else{
      Serial.print("Eror saat mengirim data ke Firebase, ");
      Serial.println(fbdo.errorReason());
    }
    Serial.println("");
  }
  return;
}

void wifiMulai(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //inisialisasi nama dan password jaringan
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500); //tunggu sampai terhubung ke jaringan
  }
  Serial.println("");
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  client.setInsecure();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //inisialisasi host dan kode "secret" firebase
  Firebase.reconnectWiFi(true); //hubungkan ulang ke jaringan apabila jaringan terputus
  Firebase.setFloatDigits(2);
  return;
}
