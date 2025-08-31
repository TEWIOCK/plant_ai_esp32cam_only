#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>

#define WIFI_SSID   "PHONE_HOTSPOT_SSID"
#define WIFI_PASS   "PHONE_HOTSPOT_PASSWORD"

#define SERVER_IP   "192.168.43.100"  // Change to server IP
#define SERVER_PORT 8000
#define POST_PATH   "/predict"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static const char *BOUNDARY = "----ESP32CAMFormBoundary";

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count     = 2;
  return (esp_camera_init(&config) == ESP_OK);
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting WiFi %s", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300); Serial.print(".");
  }
  Serial.println();
}

bool postImage(const uint8_t* data, size_t len) {
  WiFiClient client;
  if (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Connect server failed");
    return false;
  }
  String head = String("--") + BOUNDARY + "\r\n"
                "Content-Disposition: form-data; name=\"file\"; filename=\"frame.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = String("\r\n--") + BOUNDARY + "--\r\n";
  size_t contentLength = head.length() + len + tail.length();
  client.printf("POST %s HTTP/1.1\r\n", POST_PATH);
  client.printf("Host: %s:%d\r\n", SERVER_IP, SERVER_PORT);
  client.println("Connection: close");
  client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", BOUNDARY);
  client.printf("Content-Length: %u\r\n\r\n", (unsigned)contentLength);
  client.print(head);
  client.write(data, len);
  client.print(tail);
  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("{")) Serial.println("[AI RESULT] " + line);
  }
  client.stop();
  return true;
}

void setup() {
  Serial.begin(115200);
  if (!initCamera()) { Serial.println("Camera init failed"); while(1); }
  ensureWiFi();
}

void loop() {
  ensureWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb && fb->format == PIXFORMAT_JPEG) {
      postImage(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
  delay(5000);
}
