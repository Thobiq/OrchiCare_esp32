#include <Adafruit_Sensor.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>
#include <DHT_U.h>

#define DHTPIN1 5
#define DHTPIN2 19
#define DHTTYPE DHT11
#define SOIL_PIN 34

DHT_Unified dht1(DHTPIN1, DHTTYPE);
DHT_Unified dht2(DHTPIN2, DHTTYPE);

uint8_t broadcastAddress[] = {0x88, 0x13, 0xBF, 0x0B, 0x94, 0xE8};
constexpr char WIFI_SSID[] = "r-esp";

typedef struct struct_message {
    float suhu1;
    float suhu2;
    float kelembaban1;
    float kelembaban2;
    int humMediaTanam;
} struct_message;

struct_message dataTerkirim;

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Status pengiriman: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sukses" : "Gagal");
  if(status == ESP_NOW_SEND_SUCCESS){
    digitalWrite(2, HIGH);
    digitalWrite(4, LOW);
  }else{
    digitalWrite(4, HIGH);
    digitalWrite(2, LOW);
  }
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void setup() {
  Serial.begin(9600);

  dht1.begin();
  dht2.begin();

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);

  WiFi.mode(WIFI_STA);
  // Serial.println(WiFi.macAddress());
  int32_t channel = getWiFiChannel(WIFI_SSID);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init gagal");
    return;
  }

  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Gagal menambahkan peer");
    return;
  }
}

void loop() {
  sensors_event_t event;
  dht1.temperature().getEvent(&event);
  if(isnan(event.temperature)){
    Serial.println("Temperature1 error!");
  }else{
    dataTerkirim.suhu1 = event.temperature;
    Serial.print("suhu1 : ");
    Serial.println(dataTerkirim.suhu1);
  }

  dht2.temperature().getEvent(&event);
  if(isnan(event.temperature)){
    Serial.println("Temperature2 error!");
  }else{
    dataTerkirim.suhu2 = event.temperature;
    Serial.print("suhu2 : ");
    Serial.println(dataTerkirim.suhu2);
  }

  dht1.humidity().getEvent(&event);
  if(isnan(event.relative_humidity)){
    Serial.println("Kelembaban1 error!");
  }else{
    dataTerkirim.kelembaban1 = event.relative_humidity;
    Serial.print("kelembaban1 : ");
    Serial.println(dataTerkirim.kelembaban1);
    Serial.print("%");
  }

  dht2.humidity().getEvent(&event);
  if(isnan(event.relative_humidity)){
    Serial.println("Kelembaban2 error!");
  }else{
    dataTerkirim.kelembaban2 = event.relative_humidity;
    Serial.print("kelembaban2 : ");
    Serial.println(dataTerkirim.kelembaban2);
    Serial.print("%");
  }

  int nilaiSensor = analogRead(SOIL_PIN);
  Serial.print("Nilai Soil Moisture: ");
  Serial.println(nilaiSensor);

  int kelembabanPersen = map(nilaiSensor, 2500, 2100, 0, 100);
  kelembabanPersen = constrain(kelembabanPersen, 0, 100);
  dataTerkirim.humMediaTanam = kelembabanPersen;
  // dataTerkirim.humMediaTanam = nilaiSensor;

  Serial.print("Kelembaban Tanah: ");
  Serial.print(kelembabanPersen);
  Serial.println(" %");
  
  esp_now_send(broadcastAddress, (uint8_t *) &dataTerkirim, sizeof(dataTerkirim));
  Serial.println("Data dikirim!");
  delay(2000);
}
