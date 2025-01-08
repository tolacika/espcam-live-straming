#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_timer.h"
#include "esp_camera.h"
#include "esp_http_client.h"
#include "esp_websocket_client.h"
#include "esp_wifi.h"
#include "nvs_manager.h"
#include "wifi_manager.h"

#define BOARD_WROVER_KIT 1

#define WEBSOCKET_URI CONFIG_WEBSOCKET_URL

#define CAM_PIN_PWDN -1  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static const char *TAG = "espcam-websocket";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SVGA,   // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

esp_websocket_client_handle_t client;

void send_frame(camera_fb_t *fb)
{
  if (esp_websocket_client_is_connected(client))
  {
    esp_websocket_client_send_bin(client, (const char *)fb->buf, fb->len, portMAX_DELAY);
  }
}

static esp_err_t init_camera(void)
{
  // initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }

  return ESP_OK;
}

void app_main(void)
{
  static int64_t last_frame_time = 0;
  static int frame_count = 0;
  static float fps = 0.0;

  init_nvs_manager();

  // connect to wifi
  ESP_LOGI(TAG, "Connecting to wifi");
  wifi_connect_init();

  // Wait for Wi-Fi connection
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(TAG, "Connected to wifi");
  }
  vTaskDelay(1000 / portTICK_RATE_MS);

  if (ESP_OK != init_camera())
  {
    return;
  }

  esp_websocket_client_config_t websocket_cfg = {
      .uri = WEBSOCKET_URI,
  };

  ESP_LOGI(TAG, "Connecting to websocket");
  client = esp_websocket_client_init(&websocket_cfg);
  ESP_LOGI(TAG, "Starting websocket client");
  esp_websocket_client_start(client);

  while (true)
  {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb)
    {
      send_frame(fb);
      esp_camera_fb_return(fb);
    }

    int64_t current_time = esp_timer_get_time();
    frame_count++;

    if (current_time - last_frame_time >= 1000000) // 1 second
    {
      fps = frame_count;
      frame_count = 0;
      last_frame_time = current_time;
      ESP_LOGI(TAG, "Current FPS: %.2f", fps);
    }
    vTaskDelay(67 / portTICK_PERIOD_MS); // Adjust delay to force max 15 fps
  }
}
