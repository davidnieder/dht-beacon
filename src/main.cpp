#include <Arduino.h>
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <DHT_U.h>

#include "config.h"

ADC_MODE(ADC_VCC);


#define SLEEP_DURATION 1200  // in seconds

#define PIN_ESP_LED 2 // ESP-12 Board
#define PIN_DHT_PWR 4
#define PIN_DHT_DATA 5

#define SENSOR_COUNT 1

struct sensor {
  unsigned id;
  unsigned pin;
  unsigned type;
  float temperature;
  float humidity;
};

struct sensor sensors[SENSOR_COUNT] = {
  { .id = 2001, .pin = PIN_DHT_DATA, .type = DHT22 }
};

#define SENSOR_BUFSIZE 512
char sensor_buf[SENSOR_BUFSIZE];

const char *const sensor_format = "{"
  "\"id\": %d, "
  "\"temperature\": {\"value\": %.2f, \"error\": %s}, "
  "\"humidity\": {\"value\": %.2f, \"error\": %s}"
"}";

#define MSG_BUFSIZE 512
char msg_buf[MSG_BUFSIZE];

const char *msg_format = "{"
  "\"sensors\": [%s], "
  "\"adc\": {\"value\": %d, \"voltage\": %.2f}"
"}";

const char *json_true = "true";
const char *json_false = "false";


void read_sensors() {
  char buf[256];

  for (struct sensor *sensor = sensors; sensor != sensors+SENSOR_COUNT; sensor++) {
    DHT dht(sensor->pin, sensor->type);

    dht.begin();
    sensor->temperature = dht.readTemperature();
    sensor->humidity = dht.readHumidity();

    snprintf(buf, 256, sensor_format,
      sensor->id,
      isnan(sensor->temperature) ? -275 : sensor->temperature,
      isnan(sensor->temperature) ? json_true : json_false,
      isnan(sensor->humidity) ? -1 : sensor->humidity,
      isnan(sensor->humidity) ? json_true : json_false
    );

    strncat(sensor_buf, buf, SENSOR_BUFSIZE-1);

    if (sensor+1 != sensors+SENSOR_COUNT)
      strncat(sensor_buf, ",", SENSOR_BUFSIZE-1);
  }
}

void setup() {

  unsigned adc;
  float voltage;
  unsigned time_start;
  WiFiClient client;
  HTTPClient http;

  time_start = millis();

  pinMode(PIN_ESP_LED, OUTPUT);
  pinMode(PIN_DHT_PWR, OUTPUT);
  digitalWrite(PIN_ESP_LED, LOW);
  digitalWrite(PIN_DHT_PWR, HIGH);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Startup");

  delay(1500);
  read_sensors();

  adc = ESP.getVcc();
  voltage = adc/1024.0;

  snprintf(msg_buf, MSG_BUFSIZE, msg_format, sensor_buf, adc, voltage);

  WiFi.forceSleepWake(); delay(10);
  WiFi.persistent(false);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(SSID, PSK);

  int tries = 100;
  while (WiFi.status() != WL_CONNECTED && tries--) {
    delay(100);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Could not connect to wifi. Going to sleep.");
    ESP.deepSleep(SLEEP_DURATION*1E6, WAKE_RF_DISABLED);
  }

  Serial.println();
  Serial.print("Connected to wifi, IP address: ");
  Serial.println(WiFi.localIP());

  http.setReuse(false);
  http.setUserAgent(HOSTNAME);
  http.addHeader("Content-Type", "application/json");

  int ret = http.begin(client, REMOTE_HOST, REMOTE_PORT, REMOTE_ENDPOINT);
  if (ret) {
    Serial.println("Connected to server.");

    ret = http.POST(msg_buf);
    if (ret > 0) {
      Serial.print("Sent request, response status: ");
      Serial.println(ret);
    } else {
      Serial.println("Error: Could not send request.");
    }
  } else {
    Serial.println("Error: Could not connect to server.");
  }
  http.end();

  Serial.print("Going to sleep (uptime: ");
  Serial.print(millis()-time_start);
  Serial.println("ms).");
  Serial.flush();

  WiFi.disconnect();
  digitalWrite(PIN_ESP_LED, HIGH);
  digitalWrite(PIN_DHT_PWR, LOW);
  delay(10);

  ESP.deepSleep(SLEEP_DURATION*1E6, WAKE_RF_DISABLED);
}

void loop() {}
