#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_camera.h>
#include "camera_pins.h"

#define SSID "SSID"
#define PASSWORD "PASSWORD"

#define INDICATOR_LED 2

void initWiFi();
void initCamera();
void blinkThenRestart();
void startCameraServer();

void setup()
{
  pinMode(INDICATOR_LED, OUTPUT);
  digitalWrite(INDICATOR_LED, LOW);

  initWiFi();
  initCamera();
  startCameraServer();
  digitalWrite(INDICATOR_LED, HIGH);
}

void loop() {}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  if (!MDNS.begin("esp32"))
  {
    blinkThenRestart();
  }
}

void initCamera()
{
  camera_config_t config = {
      .pin_pwdn = PWDN_GPIO_NUM,
      .pin_reset = RESET_GPIO_NUM,
      .pin_xclk = XCLK_GPIO_NUM,
      .pin_sccb_sda = SIOD_GPIO_NUM,
      .pin_sccb_scl = SIOC_GPIO_NUM,
      .pin_d7 = Y9_GPIO_NUM,
      .pin_d6 = Y8_GPIO_NUM,
      .pin_d5 = Y7_GPIO_NUM,
      .pin_d4 = Y6_GPIO_NUM,
      .pin_d3 = Y5_GPIO_NUM,
      .pin_d2 = Y4_GPIO_NUM,
      .pin_d1 = Y3_GPIO_NUM,
      .pin_d0 = Y2_GPIO_NUM,
      .pin_vsync = VSYNC_GPIO_NUM,
      .pin_href = HREF_GPIO_NUM,
      .pin_pclk = PCLK_GPIO_NUM,

      .xclk_freq_hz = 26000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,
      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_240X240,
      .jpeg_quality = 12,
      .fb_count = 2,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_LATEST,
  };

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    blinkThenRestart();
  }
}

void blinkThenRestart()
{
  digitalWrite(INDICATOR_LED, LOW);
  for (uint8_t i = 0; i < 10; i++)
  {
    delay(500);
    digitalWrite(INDICATOR_LED, !digitalRead(INDICATOR_LED));
  }
  ESP.restart();
}