#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>

#include "pins.h"
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
const int led_pin = D5;
const int rotary_clk_pin = D3;
const int rotary_dt_pin = D2;
const int rotary_sw_pin = D1;

// Button values
uint8_t button_state;
uint8_t previous_button_state = LOW;

// Rotary encoder values
const int positions_per_rotation = 40; // Estimate
int current_clk_state;
int prev_clk_state;
int position = 0;
int previous_position = 0;
float speed;

// Time
unsigned long current_time, previous_time, delta_time;

// Light values
const int brightness_change_interval = 100;
float brightnes = 0;
float sensitivity = 3.0;
float decay_rate = 0.4; // Percentage per second

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
  pinMode(led_pin, OUTPUT);

  pinMode(rotary_clk_pin, INPUT);
  pinMode(rotary_dt_pin, INPUT);
  pinMode(rotary_sw_pin, INPUT_PULLUP);

  prev_clk_state = digitalRead(rotary_clk_pin);
  previous_time = millis();
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

  float speed;
  sscanf(&payload[1], "%f", &speed);

  brightness += speed * sensitivity;
}

void update_brightness() {
  brightness -= decay_rate;
  if (brightness > 255) brightness = 255;
  if (brightness < 0) brightness = 0;
  analogWrite(LED_BUILTIN, brightness);
}

void update_connection() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

  if (!mqtt_client.connected()) {
      connectToMQTTBroker();
  }
  mqtt_client.loop();
}

void button_action() {
  button_state = digitalRead(button_pin);
  if (button_state != previous_button_state) {
    // Do button stuff (change mode)

    // char state_to_send = button_state == HIGH ? '1' : '0';
    // char payload[3] = {single_char_id, state_to_send, '\0'};
    // mqtt_client.publish(mqtt_topic, payload);
    // previous_button_state = button_state;
  }
}

void loop() {
  update_connection();

  

  // Update time
  current_time = millis();
  delta_time = current_time - previous_time
  if (delta_time < brightness_change_interval) {
    return
  }
  previous_time = current_time;

  // Rotary encoder
  // Read the current state of CLK
  current_clk_state = digitalRead(rotary_clk_pin);

  // Check if the CLK pin state has changed
  if (current_clk_state == prev_clk_state) {
    return;
  }
  position++;

  float delta_rotation = float(position - previous_position) / float(positions_per_rotation)
  speed = delta_roation / float(delta_time) * 1000; // Rotation per second (1000ms)
  speed = ((float)(position - previous_position) / (float)delta_time) * 10;

  char payload[3] = {single_char_id, state_to_send, '\0'};
  mqtt_client.publish(mqtt_topic, payload);
  previous_button_state = button_state;

  prev_clk_state = current_clk_state;
  previous_position = position;
  // Update the lastStateCLK with the current state
  

  // // Check if the button is pressed
  // if (digitalRead(SW) == LOW) {
  //   Serial.println("Button pressed");
  //   delay(200);  // Debouncing
  // }
}
