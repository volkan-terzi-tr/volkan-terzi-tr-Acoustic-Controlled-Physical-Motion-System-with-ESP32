# ESP32-S3 Acoustic Controlled Servo System

## 🔧 Project Overview
This project demonstrates a simple sound-triggered servo control system using an **ESP32-S3**, **INMP441 microphone module**, and a **Tower Pro SG90 servo motor**. When a loud sound (e.g., clap, voice, noise) is detected above a set threshold, the ESP32 activates the servo motor. A local web interface is also provided to monitor the system in real-time.

## 🧰 Hardware Used
- ESP32-S3 Development Board  
- INMP441 Digital MEMS Microphone  
- SG90 Micro Servo Motor  
- USB cable (for power & programming)  
- Breadboard and jumper wires  

## 🖥️ Software & Libraries
- [Arduino IDE](https://www.arduino.cc/en/software)
- ESP32 Board Support via Board Manager
- Libraries:
  - `ESP32Servo`
  - `WebServer`
  - `I2S` (built-in or ESP-IDF-based)

## 🚀 How to Use
1. Connect the hardware as shown in the circuit diagram.
2. Open the Arduino IDE and install required libraries.
3. Select the correct board: **ESP32-S3 Dev Module** and appropriate port.
4. Upload the code from `main.ino` to the ESP32-S3.
5. Once uploaded, the ESP32 will create a Wi-Fi Access Point.
6. Connect to the Wi-Fi (e.g., `ESP32-AUDIO`), and open a browser.
7. Navigate to `http://192.168.4.1` to view the live interface.
