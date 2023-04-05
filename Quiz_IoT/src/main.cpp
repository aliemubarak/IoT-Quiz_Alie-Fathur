#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_GREEN  18
#define LED_YELLOW 2
#define LED_RED    18
#define DHT_PIN 19
#define BHT_SDA 22
#define BHT_SCL 23

#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "wifiwifi"
#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883
unsigned long lastTempPublish = 0;
unsigned long lastHumidityPublish = 0;
unsigned long lastLightPublish = 0;


int g_nCount=0;
DHTesp dht;
BH1750 lightMeter;
WiFiClient espClient;
PubSubClient client(espClient);

void taskControlLED(void *pvParameters) {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);

  while (1) {
    float fHumidity = dht.getHumidity();
    float fTemperature = dht.getTemperature();
    Serial.printf("Humidity: %.2f, Temperature: %.2f\n", fHumidity, fTemperature);
    if (fTemperature > 28 && fHumidity > 80) {
      digitalWrite(LED_RED, HIGH);
    } else if (fTemperature > 28 && fHumidity > 60 && fHumidity <= 80) {
      digitalWrite(LED_YELLOW, HIGH);
    } else if (fTemperature <= 28 && fHumidity <= 60) {
      digitalWrite(LED_GREEN, HIGH);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}


void taskSendWarning(void *pvParameters) {
  while (1) {
    float lux = lightMeter.readLightLevel();
    Serial.printf("light: %.2f",lux);
    if (lux > 400) {
      client.publish("ESP_Quiz/doorstate", "Door open");
    } else if (lux <= 400) {
      client.publish("ESP_Quiz/doorstate", "Door closed");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(BHT_SDA, BHT_SCL);
  lightMeter.begin();
  dht.setup(DHT_PIN, DHTesp::DHT11);
    // init Leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  xTaskCreatePinnedToCore(taskSendWarning, "taskSendWarning", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskControlLED, "taskControlLED", 2048, NULL, 1, NULL, 0);\
  WiFi.begin(wifi, wifiwiwfi);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  client.setServer(MQTT_SERVER, MQTT_PORT);
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      delay(5000);
      Serial.println("Retrying MQTT connection...");
    }
  }
  Serial.println("System ready");
}

void loop() {
  
  if (millis() - lastTempPublish > 5000) {
    float fTemperature = dht.getTemperature();
    char tempStr[8];
    dtostrf(fTemperature, 4, 2, tempStr);
    client.publish("ESP_Quiz/temperature", tempStr);
    Serial.println("sending temp");
    lastTempPublish = millis();
  }

  if (millis() - lastHumidityPublish > 6000) {
    float fHumidity = dht.getHumidity();
    char humidityStr[8];
    dtostrf(fHumidity, 4, 2, humidityStr);
    client.publish("ESP_Quiz/humidity", humidityStr);
    Serial.println("sending humd");
    lastHumidityPublish = millis();
  }

  if (millis() - lastLightPublish > 4000) {
    float lux = lightMeter.readLightLevel();
    char lightStr[8];
    dtostrf(lux, 4, 2, lightStr);
    client.publish("ESP_Quiz/light", lightStr);
    Serial.println("sending light");
    lastLightPublish = millis();
  }
  client.loop();
}