# 1 "/tmp/tmpdtap7v3x"
#include <Arduino.h>
# 1 "/home/sambaray/crocker_fellowship/platformio/ripple_rev4.0/src/ripple_rev3.3.ino"


       

#include "thingProperties.h"
#include "wifi_connect.h"
# 19 "/home/sambaray/crocker_fellowship/platformio/ripple_rev4.0/src/ripple_rev3.3.ino"
void record_task(void* param);
void send_task(void* param);
void cfg_upload_task(void* param);
void queue_setup();
void setup();
void loop();
#line 19 "/home/sambaray/crocker_fellowship/platformio/ripple_rev4.0/src/ripple_rev3.3.ino"
void record_task(void* param) {
  int mil;
  while(true) {
    mil = millis();
    record();


    xQueueReceive(free_queue, &audio_buffer, portMAX_DELAY);

    amp_cov(raw_buffer, audio_buffer);




    xQueueSend(send_queue, &audio_buffer, portMAX_DELAY);



    Serial.print("Record task completed in: ");
    Serial.println(millis()-mil);
  }
}

void send_task(void* param) {
  int response;
  int mil;
  while(true) {
    mil = millis();
    xQueueReceive(send_queue, &audio_buffer, portMAX_DELAY);
    xQueuePeek(config_queue, &upload_state, 0);


    upload_cnf = cnf_fsm(upload_state);
    Serial.printf("Upload Config filenum: %d\n", upload_cnf.file_num);
    String final_url = url+upload_cnf.endpoint+((String) upload_cnf.file_num)+".wav";
    Serial.printf("URL: %s\n", final_url.c_str());
    response = upload_audio(home, secure_client, final_url, audio_buffer, FULL_CHUNK_BYTES, access_key);
    Serial.print("Server response: ");
    Serial.println(response);
    xQueueSend(free_queue, &audio_buffer, portMAX_DELAY);


    Serial.print("Send task completed in: ");
    Serial.println(millis()-mil);
  }
}

void cfg_upload_task(void* param) {
  while(true) {

    upload_state = upld_state_fsm(rt_button, t_button, upload_state);
    xQueueOverwrite(config_queue, &upload_state);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void queue_setup() {
  send_queue = xQueueCreate(1, sizeof(uint8_t*));
  free_queue = xQueueCreate(1, sizeof(uint8_t*));
  config_queue = xQueueCreate(1, sizeof(UploadState));
  xQueueSend(free_queue, &audio_buffer, 0);
  xQueueSend(config_queue, &upload_cnf, 0);
  xTaskCreatePinnedToCore(record_task, "Record", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(send_task, "Send", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(cfg_upload_task, "Configure Upload", 4096, NULL, 1, NULL, tskNO_AFFINITY);
}

void setup() {

  Serial.begin(115200);

  delay(1500);


  initProperties();

  queue_setup();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}