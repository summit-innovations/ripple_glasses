#pragma once

#include <driver/i2s.h>

#define I2S_WS 27
#define I2S_SCK 14
#define I2S_SD 32
#define I2S_PORT I2S_NUM_0


void config_i2s() {
  Serial.println("Configuring mic pins");
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,   // GPIO7 (D8)
    .ws_io_num = I2S_WS,     // GPIO8 (D9)
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD    // GPIO9 (D10)
  };
  esp_err_t err;
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  Serial.printf("i2s_driver_install: %d\n", err);
  err = i2s_set_pin(I2S_PORT, &pin_config);
  Serial.printf("i2s_set_pin: %d\n", err);
  // Serial.println("Mic pins configured correctly!!");
}

