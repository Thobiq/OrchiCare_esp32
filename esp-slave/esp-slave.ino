#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//pin
#define FanPin 33
#define SprinklerPin 32

//wifi
const char *ssid = "r-esp";
const char *password = "eeeeeeee";

//broker
const char *mqtt_broker = "broker.tbqproject.my.id";
const char *topic = "tbq/broker";
const char *mqtt_username = "tbqmqtt";
const char *mqtt_password = "tbq2412";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

//interval publish data
unsigned long lastPublish = 0;
const long publishInterval = 2000;

void setup_wifi() {
  delay(100);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wifi Terhubung IP : " + String(WiFi.localIP()));
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker.");
      client.subscribe(topic); // optional, kalau mau terima pesan juga
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Struktur data yang sama
typedef struct struct_message {
    float suhu1;
    float suhu2;
    float kelembaban1;
    float kelembaban2;
    int humMediaTanam;
} struct_message;

struct_message dataDiterima;
StaticJsonDocument<256> doc;

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  Serial.print("Data diterima dari MAC: ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
           recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  Serial.println(macStr);

  memcpy(&dataDiterima, incomingData, sizeof(dataDiterima));

  // inisialisasi json
  doc["suhu"] = max(dataDiterima.suhu1, dataDiterima.suhu2);
  doc["kelembaban-gh"] = max(dataDiterima.kelembaban1, dataDiterima.kelembaban2);
  doc["kelembaban-mt"] = dataDiterima.humMediaTanam;

  // Serial.print("Suhu: ");
  // Serial.println(dataDiterima.suhu1);
  
  // Serial.print("Kelembaban: ");
  // Serial.println(dataDiterima.kelembaban1);
}


void setup() {
  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  reconnect();

  lcd.init();
  lcd.backlight();
  

  pinMode(2, OUTPUT);
  pinMode(FanPin, OUTPUT);
  pinMode(SprinklerPin, OUTPUT);

  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init gagal");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}



void loop() {
  // serializeJson(doc, buffer);
  // Serial.println(doc);
  // serializeJson(doc, Serial);
  // Serial.println();

  if(dataDiterima.suhu1 && dataDiterima.suhu2){
    digitalWrite(2, HIGH);
  }else{
    digitalWrite(2, LOW);
  }

  

  if(max(dataDiterima.suhu1, dataDiterima.suhu2) > 37 && max(dataDiterima.suhu1, dataDiterima.suhu2) != 0){
    digitalWrite(FanPin, LOW);
    doc["fanStatus"] = "ON";
  }else if(max(dataDiterima.suhu1, dataDiterima.suhu2) <= 37){
    digitalWrite(FanPin, HIGH);
    doc["fanStatus"] = "OFF";
  }else{
    digitalWrite(FanPin, HIGH);
    doc["fanStatus"] = "OFF";
  }

  if(dataDiterima.humMediaTanam < 60 && dataDiterima.humMediaTanam != 0){
    digitalWrite(SprinklerPin, LOW);
    doc["sprinklerStatus"] = "ON";
  }else if(dataDiterima.humMediaTanam >= 80){
    digitalWrite(SprinklerPin, HIGH);
    doc["sprinklerStatus"] = "OFF";
  }else{
    digitalWrite(SprinklerPin, HIGH);
    doc["sprinklerStatus"] = "OFF";
  }

  lcd.setCursor(0, 0);
  // lcd.print("s1=" + String(dataDiterima.suhu1));
  // lcd.print("s2=" + String(dataDiterima.suhu2));
  lcd.print("Temp = ");
  lcd.print(max(dataDiterima.suhu1, dataDiterima.suhu2));

  lcd.setCursor(0, 1);
  // lcd.print("H1=" + String(dataDiterima.kelembaban1));
  // lcd.print("H2=" + String(dataDiterima.kelembaban2));
  lcd.print("Hum = ");
  lcd.print(dataDiterima.humMediaTanam);

  if (!client.connected()) {
    reconnect();
  }  

  client.loop();

  if (millis() - lastPublish > publishInterval) {
    
    // String msgTemp1 = "Suhu1 : " + String(dataDiterima.suhu1);
    // String msgTemp2 = "Suhu2 : " + String(dataDiterima.suhu2);
    // String msgHum = "Kelembaban : " + String(dataDiterima.humMediaTanam);

    char buffer[256];
    serializeJson(doc, buffer);

    // client.publish(topic, msgTemp1.c_str());
    // client.publish(topic, msgTemp2.c_str());
    client.publish(topic, buffer);

    lastPublish = millis();
  }
}
