#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "wifi_connect.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "cert.h"

//Mic recording definitions
#define V_REF 3.3
#define SAMPLE_RATE 16000 //Samples per second
#define BYTES_PER_RAW_SAMPLE 4
#define BYTES_PER_FULL_SAMPLE 3
#define CHUNK_SECONDS 2
#define SAMPLES_PER_CHUNK (SAMPLE_RATE*CHUNK_SECONDS)
#define RAW_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_RAW_SAMPLE)
#define FULL_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_FULL_SAMPLE)
#define VOLUME_GAIN 0

//Wifi/Database keys
const char* ssid = "SamuelF"; //TP-LINK_AB77 //BYU-WiFi //SamuelF
const char* password = "samb@r@y"; //21940521 //samb@r@y
const String url = "https://summit-innovative.duckdns.org";
const String training_ext = "/training/";
const String realtime_ext = "/realtime/";
const String access_key = "c18ec4ba23d30007f7b6a304d79762db3f6f69eecc7fbded89949a6c3ac5f24c";

HTTPClient home;
WiFiClientSecure secure_client;

QueueHandle_t send_queue;
QueueHandle_t free_queue;
QueueHandle_t config_queue;

int32_t* raw_buffer = NULL;
uint8_t * audio_buffer = NULL;

typedef enum {TRAINING, REALTIME, WAITING} UploadState;
UploadState upload_state = WAITING;

typedef struct {
  String endpoint;
  unsigned long file_num;
} UploadConfig;

UploadConfig upload_cnf;

typedef enum {DOWN=0, UP, PRESS, RELEASE} ButtonState;

typedef struct {
  int pin;
  ButtonState current_state = UP;
  bool last_reading = HIGH;
  unsigned long last_bounce = 0;
} ButtonTracker;

ButtonTracker rt_button = {RT_BUTTON_PIN};
ButtonTracker t_button = {T_BUTTON_PIN};

ButtonTracker get_bt_state(ButtonTracker button){
  //Look for button press
  const static unsigned long debounce_t = 50;
  bool reading = digitalRead(button.pin);
  // Serial.printf("Digital read: %d\n", reading);
  // Guard rails on button press
  if (reading != button.last_reading) {
    button.last_bounce = millis();
  }
  Serial.printf("Current State: %d\n", button.current_state);
  if (millis() - button.last_bounce >= debounce_t){
    switch(button.current_state){
      case DOWN:
        if (reading == HIGH) {button.current_state = RELEASE;}
        break;
      case UP:
        if (reading == LOW) {button.current_state = PRESS;}
        break;
      case PRESS:
        button.current_state = DOWN;
        break;
      case RELEASE:
        button.current_state = UP;
        break;
    } 
  }
  button.last_reading = reading;
  return button;
}

UploadState get_upld_state(ButtonState rt_button, ButtonState t_button, UploadState state_current) {
  if (rt_button == t_button || rt_button == PRESS || t_button == PRESS || rt_button == RELEASE || t_button == RELEASE) {
    return state_current;
  }
  else if (rt_button == DOWN) {
    return REALTIME;
  }
  else if (t_button == DOWN) {
    return TRAINING;
  }
  else {
    return state_current;
  }
}

UploadConfig cnf_fms(UploadState& upload_state) {
  static UploadConfig current_cnf;
  static int file_num = 0;
  // Get button states
  rt_button = get_bt_state(rt_button);
  // Serial.printf("Realtime Button State: %d\n", rt_button);
  t_button = get_bt_state(t_button);
  // Serial.printf("Training Button State: %d\n", t_button);
  upload_state = get_upld_state(rt_button.current_state, t_button.current_state, upload_state);
  // If T_BUTTON_PIN is low, turn T_LED_PIN on
  if (upload_state == REALTIME) {
    digitalWrite(T_LED_PIN, LOW);
    digitalWrite(RT_LED_PIN, HIGH);
    current_cnf.endpoint = realtime_ext;
    current_cnf.file_num = 0;
  }
  // If RT_BUTTON_PIN is low, turn RT_LED_PIN on
  else if(upload_state == TRAINING) {
    digitalWrite(RT_LED_PIN, LOW);
    digitalWrite(T_LED_PIN, HIGH);
    current_cnf.endpoint = training_ext;
    current_cnf.file_num = file_num;
    file_num++;
  }
}

void initProperties(){
  //Configure GPIO pins
  pin_config();

  //Create PSRAM buffer
  // Serial.println((int) FULL_CHUNK_BYTES);
  // Serial.println((int) RAW_CHUNK_BYTES);
  // Serial.printf("PSRAM found: %s\n", psramFound() ? "yes" : "no");
  // Serial.printf("PSRAM total: %d, free: %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  audio_buffer = (uint8_t*)ps_malloc((int) FULL_CHUNK_BYTES);
  raw_buffer = (int32_t *)ps_malloc((int) RAW_CHUNK_BYTES);
  
  if (!audio_buffer || !raw_buffer) {
    Serial.println("PSRAM allocation failed");
    while (1);
  }

  //Connect to wifi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //Set up database socket
  secure_client.setCACert(ROOT_CA);
  home.setReuse(true);

  //Determine what state we want to be in (realtime or training uploads)
  // Write FSM machine

  // Set the file name variable so it uploads appropriately
  // If the state is realtime, set the filename to alternate between 0 and 1
  // If the state is training, query the server, find the oldest file number, and put the file name to the number after that
  digitalWrite(WAIT_LED_PIN, HIGH);
  while(upload_state == WAITING) {
    // Get button states
    rt_button = get_bt_state(rt_button);
    // Serial.printf("Realtime Button State: %d\n", rt_button);
    t_button = get_bt_state(t_button);
    // Serial.printf("Training Button State: %d\n", t_button);


    upload_state = get_upld_state(rt_button.current_state, t_button.current_state, upload_state);
    // If T_BUTTON_PIN is low, turn T_LED_PIN on
    if (upload_state == REALTIME) {
      digitalWrite(T_LED_PIN, LOW);
      digitalWrite(RT_LED_PIN, HIGH);
    }
    // If RT_BUTTON_PIN is low, turn RT_LED_PIN on
    else if(upload_state == TRAINING) {
      digitalWrite(RT_LED_PIN, LOW);
      digitalWrite(T_LED_PIN, HIGH);
    }
  }
  digitalWrite(WAIT_LED_PIN, LOW);
}

void amp_cov(int32_t* r_buffer, uint8_t* a_buffer) {
  int32_t raw32;
  for(size_t i = 0; i < SAMPLES_PER_CHUNK; i++) {
    // Serial.print("Raw 32 bit integer: ");
    // Serial.print(r_buffer[i], HEX);
    // Serial.println();
    raw32 = r_buffer[i] >> 8;
    raw32 <<= VOLUME_GAIN;
    a_buffer[(i*3)] = (uint8_t)((int32_t)raw32 & 0xFF);
    a_buffer[(i*3)+1] = (uint8_t)(((int32_t)raw32 >> 8) & 0xFF);
    a_buffer[(i*3)+2] = (uint8_t)(((int32_t)raw32 >> 16) & 0xFF);     
    // Serial.print("Full 24 bit integer: ");
    // Serial.print(a_buffer[(i*3)], HEX);
    // Serial.print(a_buffer[(i*3)+1], HEX);
    // Serial.print(a_buffer[(i*3)+2], HEX);
    // Serial.println();
  }
}

void record() {
  size_t bytes_read = 0;
  size_t bytes_collected = 0;
  do {
    i2s_channel_read(rx_handle, (uint8_t*) raw_buffer + bytes_collected, RAW_CHUNK_BYTES - bytes_collected, &bytes_read, portMAX_DELAY);
    bytes_collected += bytes_read;
  } while (bytes_collected < RAW_CHUNK_BYTES);
}
