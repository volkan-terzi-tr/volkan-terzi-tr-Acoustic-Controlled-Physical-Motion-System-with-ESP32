#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <driver/i2s.h>

// Mikrofon pinleri (INMP441)
#define I2S_WS  15
#define I2S_SCK 14
#define I2S_SD  33

#define SAMPLE_RATE     16000
#define BUFFER_SIZE     1024
#define THRESHOLD       200

Servo servo;
bool is_open = false;
unsigned long last_command_time = 0;
long average = 0;

// Wi-Fi Access Point bilgileri
const char* ssid = "ESP32-SOUND";
const char* password = "";

WebServer server(80);

// Web arayüz HTML içeriği
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Ses Kontrol</title>
  <meta http-equiv="refresh" content="1">
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    .box { border: 1px solid #ccc; padding: 20px; display: inline-block; border-radius: 10px; }
  </style>
</head>
<body>
  <div class="box">
    <h2>Acoustic-Controlled Physical Motion System with ESP32</h2>
    <p><strong>Sound Level:</strong> %SOUND%</p>
    <p><strong>Servo Status:</strong> %STATE%</p>
  </div>
</body>
</html>
)rawliteral";

// Web arayüz dinamik veri yerleştirme
void handleRoot() {
  String html = index_html;
  html.replace("%SOUND%", String(average));
  html.replace("%STATE%", is_open ? "ON" : "OFF");
  server.send(200, "text/html", html);
}

// I2S mikrofon başlatma
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  servo.attach(18);
  servo.write(0);

  // Access Point başlat
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("ESP32 IP Adresi: ");
  Serial.println(WiFi.softAPIP());

  // Web sunucu
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  int16_t samples[BUFFER_SIZE];
  size_t bytes_read;
  i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

  long sum = 0;
  int sample_count = bytes_read / 2;
  for (int i = 0; i < sample_count; i++) {
    sum += abs(samples[i]);
  }

  average = sum / sample_count;

  if (average > THRESHOLD && millis() - last_command_time > 2000) {
    is_open = !is_open;
    servo.write(is_open ? 90 : 0);
    Serial.println(is_open ? "AÇILDI" : "KAPANDI");
    last_command_time = millis();
  }

  delay(100);
}
