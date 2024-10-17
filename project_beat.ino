#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>

#include "secrets.h"

// WIFI
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// Brooker connection
const char *mqtt_broker = BROOKER_URL;
const char *mqtt_username = AUTH_USERNAME;
const char *mqtt_password = AUTH_PASSWORD;
const char *mqtt_topic = TOPIC;
const int mqtt_port = PORT;
const char *ca_cert = CA_CERTIFICATE;

// Clients
BearSSL::WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// NTP Server settings
const char *ntp_server = "pool.ntp.org";
const long gmt_offset_sec = 7200; // GMT offset in seconds (adjust for your time zone)
const int daylight_offset_sec = 0;  

const char single_char_id = SINGLE_CHAR_ID;

// Hardware
const int button_pin = D6;

uint8_t led_state = LOW;

uint8_t button_state;
uint8_t previous_button_state = LOW;

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

  // Hardware setup
  pinMode(button_pin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // HIGH means off
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf("Wifi status: %d \n", WiFi.status());
  }
  Serial.println("Connected to the WiFi network");
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

  if ((char) payload[0] == single_char_id) {
    Serial.println("Own message xD");
    return;
  }

  if ((char) payload[1] == '1') {
    digitalWrite(LED_BUILTIN, LOW); // LOW means on
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

  if (!mqtt_client.connected()) {
      connectToMQTTBroker();
  }
  mqtt_client.loop();

  button_state = digitalRead(button_pin);

  if (button_state != previous_button_state) {
    char state_to_send = button_state == HIGH ? '1' : '0';
    char payload[3] = {single_char_id, state_to_send, '\0'};
    mqtt_client.publish(mqtt_topic, payload);
    previous_button_state = button_state;
  }
}
