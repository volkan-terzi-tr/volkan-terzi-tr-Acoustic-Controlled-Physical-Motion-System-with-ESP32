// Include required libraries
#include <WiFi.h>               // For managing Wi-Fi connectivity
#include <WebServer.h>          // For creating a simple web server
#include <ESP32Servo.h>         // For controlling a servo motor
#include <driver/i2s.h>         // For using I2S protocol to read audio from INMP441 microphone

// Define pin connections for the INMP441 microphone
#define I2S_WS  15              // Word Select (also called LRCL or WS)
#define I2S_SCK 14              // Serial Clock (BCLK)
#define I2S_SD  33              // Serial Data Input (DOUT from mic)

// Audio sampling settings
#define SAMPLE_RATE     16000   // Sampling rate (16 kHz)
#define BUFFER_SIZE     1024    // Number of samples to read per loop
#define THRESHOLD       200     // Sound level threshold to trigger the servo

Servo servo;                    // Create a Servo object
bool is_open = false;           // Tracks whether the servo is open (90°) or closed (0°)
unsigned long last_command_time = 0; // To prevent rapid toggling (debouncing)
long average = 0;               // Stores the average sound level

// Wi-Fi Access Point credentials
const char* ssid = "ESP32-SOUND";  // SSID for ESP32 AP
const char* password = "";         // No password (open network)

// Create a web server on port 80
WebServer server(80);

// Web page HTML content, dynamically updated with sound and servo state
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Sound Control</title>
  <meta http-equiv="refresh" content="1"> <!-- Auto-refresh every 1 second -->
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

// Handle root page: insert live data into HTML and send to client
void handleRoot() {
  String html = index_html;
  html.replace("%SOUND%", String(average));           // Insert current average sound level
  html.replace("%STATE%", is_open ? "ON" : "OFF");    // Insert current servo status
  server.send(200, "text/html", html);                // Send the updated HTML to browser
}

// Initialize I2S interface to read audio data from the microphone
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),    // ESP32 as master and receiving
    .sample_rate = SAMPLE_RATE,                             // Set sample rate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,           // 16-bit samples
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,            // Use only left channel
    .communication_format = I2S_COMM_FORMAT_I2S,            // Standard I2S format
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,                                     // Number of DMA buffers
    .dma_buf_len = 512,                                     // Size of each DMA buffer
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  // Define microphone pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,            // BCLK pin
    .ws_io_num = I2S_WS,              // LRCL/WS pin
    .data_out_num = I2S_PIN_NO_CHANGE,// No data output (receive-only)
    .data_in_num = I2S_SD             // Microphone output pin (DOUT)
  };

  // Install and initialize the I2S driver with above configurations
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);     // Clear the DMA buffer before starting
}

void setup() {
  Serial.begin(115200);       // Start serial monitor for debugging
  setupI2S();                 // Initialize the I2S microphone
  servo.attach(18);           // Attach servo to GPIO 18
  servo.write(0);             // Start with servo closed (0 degrees)

  // Start ESP32 in Access Point mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.softAPIP()); // Print AP IP address

  // Define root route for web server
  server.on("/", handleRoot);
  server.begin();             // Start the web server
}

void loop() {
  server.handleClient();      // Handle any incoming HTTP requests

  // Read audio samples from the microphone
  int16_t samples[BUFFER_SIZE];       // Buffer to hold 16-bit audio samples
  size_t bytes_read;
  i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

  // Calculate the average sound level
  long sum = 0;
  int sample_count = bytes_read / 2;  // Each sample is 2 bytes (16 bits)
  for (int i = 0; i < sample_count; i++) {
    sum += abs(samples[i]);           // Use absolute value to get sound amplitude
  }
  average = sum / sample_count;       // Compute average amplitude

  // If average sound level exceeds threshold and debounce time passed
  if (average > THRESHOLD && millis() - last_command_time > 2000) {
    is_open = !is_open;               // Toggle the servo state
    servo.write(is_open ? 90 : 0);    // Move servo accordingly
    Serial.println(is_open ? "OPENED" : "CLOSED"); // Print status to Serial Monitor
    last_command_time = millis();     // Update debounce timer
  }

  delay(100); // Small delay to reduce CPU usage
}