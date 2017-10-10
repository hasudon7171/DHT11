
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <DHT.h>
#include <Milkcocoa.h>

//
// 使用環境に合わせて必ず修正する定義
//
// Milkcocoa
#define MILKCOCOA_APP_ID       ""
//
//  WiFi
const char *WIFI_SSID =        "";
const char *WIFI_PASSWORD =    "";
//

//
// 必要に応じて修正する定義
//
// Deep Sleep
#define SLEEP_INTERVAL         2 // 2分ごとに実行
// Milkcocoa
#define MILKCOCOA_DATASTORE    "esp8266/dht11" // データストアを変更したいときは修正
//
// DHT 温度・湿度センサー
#define DHTPIN 2               // センサーの入力ピン番号
#define DHTTYPE DHT11          // DHT 11 を使用するときはコメントを外す
//#define DHTTYPE DHT22        // DHT 22 (AM2302), AM2321 を使用するときはコメントを外す
//#define DHTTYPE DHT21        // DHT 21 (AM2301) を使用するときはコメントを外す

//
// 修正する必要のない定義
//
// Milkcocoa
#define MILKCOCOA_SERVERPORT  1883
const char MQTT_SERVER[] PROGMEM    = MILKCOCOA_APP_ID ".mlkcca.com";
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ MILKCOCOA_APP_ID;
//
// NTP
const char *NTP_SERVER = "ntp.nict.jp";
const int TIME_OFFSET = 9 * 60 * 60;  // UTC+9h (JST)

//
// グローバル
//
//  DHT
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(74880);
  Serial.println("\r\nReset Reason: " + ESP.getResetReason());

  // Setup DHT
//  pinMode(15, OUTPUT);      // WIO NODE使用時は2行をコメントを外す
//  digitalWrite(15,HIGH);    // センサーに電源を供給
  dht.begin();

  // Setup WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Waiting for Wi-Fi connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nConnected!");

  // Setup NTP
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, NTP_SERVER, TIME_OFFSET);
  timeClient.begin();
  while (!timeClient.update()) {
    delay(500);
  }
  setTime(timeClient.getEpochTime());
}

void loop() {
  // DeepSleep
  int min = SLEEP_INTERVAL - minute() % SLEEP_INTERVAL;
  if (min != SLEEP_INTERVAL) {
    Serial.println(getDateTime() + " " + "Go to sleep for " + min+ " minutes.");
    ESP.deepSleep((min * 60 - second() + 10) * 1000 * 1000, WAKE_RF_DEFAULT);
  }
  
  // DHT
  delay(2000);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println(getDateTime() + " " + "Failed to read from DHT sensor!");
    return;
  }

  // Milkcocoa
  WiFiClient client;
  Milkcocoa milkcocoa = Milkcocoa(&client, MQTT_SERVER, MILKCOCOA_SERVERPORT, MILKCOCOA_APP_ID, MQTT_CLIENTID);
  milkcocoa.loop();
  DataElement elem = DataElement();
  char ch[20];
  String st = getDateTime();
  st.toCharArray(ch, 17);
  elem.setValue("datetime", ch);
  elem.setValue("temperature", (int)round(t));
  elem.setValue("humidity", (int)round(h));
  milkcocoa.push(MILKCOCOA_DATASTORE, &elem);
  Serial.println(getDateTime() + " " +
                            "DateTime:" + String(elem.getString("datetime")) +
                            " Temperature:" + String(elem.getInt("temperature")) +
                            "C Humidity:" + String(elem.getInt("humidity")) + "%");
  // Deep Sleep
  min = SLEEP_INTERVAL - minute() % SLEEP_INTERVAL;
  Serial.println(getDateTime() + " " + "Go to sleep for " + min+ " minutes.");
  ESP.deepSleep((min * 60 - second() + 10) * 1000 * 1000, WAKE_RF_DEFAULT);
}

//  getDateTime
String getDateTime() {
  char dt[20];
  sprintf(dt, "%04d/%02d/%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  return String(dt);
}

