#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>

#include "secrets.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const char *mqtt_broker = BROOKER_URL;
const char *mqtt_username = AUTH_USERNAME;
const char *mqtt_password = AUTH_PASSWORD;
const char *mqtt_topic = TOPIC;
const int mqtt_port = PORT;

const char *ca_cert = CA_CERTIFICATE;

BearSSL::WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// NTP Server settings
const char *ntp_server = "pool.ntp.org";
const long gmt_offset_sec = 7200;            // GMT offset in seconds (adjust for your time zone)
const int daylight_offset_sec = 0;  

void connectToWifi();
void connectToMQTTBroker();
void syncTime();
void mqttCallback(char *topic, byte *payload, unsigned int length);



void setup() {
  Serial.begin(115200);
  connectToWifi();
  syncTime();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
}

void connectToWifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to the WiFi network");
}

void syncTime() {
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    Serial.print("Waiting for NTP time sync: ");
    while (time(nullptr) < 8 * 3600 * 2) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Time synchronized");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Current time: ");
        Serial.println(asctime(&timeinfo));
    } else {
        Serial.println("Failed to obtain local time");
    }
}

void connectToMQTTBroker() {
  BearSSL::X509List serverTrustedCA(ca_cert);
  esp_client.setTrustAnchors(&serverTrustedCA);

  while (!mqtt_client.connected()) {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);
      mqtt_client.publish(mqtt_topic, "IS ANYONE THERE??");
    } else {
      char err_buf[128];
      esp_client.getLastSSLError(err_buf, sizeof(err_buf));
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.println(mqtt_client.state());
      Serial.print("SSL error: ");
      Serial.println(err_buf);
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop() {
  if (!mqtt_client.connected()) {
      connectToMQTTBroker();
  }
  mqtt_client.loop();
}
