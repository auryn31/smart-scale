#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"                 // HX711 library for load cell weight sensor
#include <Adafruit_GFX.h>          // Graphics library for TFT display
#include <Adafruit_ILI9341.h>      // TFT library for ILI9341-based displays

// Pin Definitions for TFT (SPI)
#define TFT_CS   5
#define TFT_DC   17
#define TFT_RST  16
#define TFT_MOSI 23
#define TFT_CLK  18

// Pin Definitions for HX711 (Weight sensor)
#define DOUT  26   // HX711 DOUT pin
#define CLK   25   // HX711 CLK pin
#define WAKE_PIN 33 // Pin to wake up from deep sleep (use a touch pin or external interrupt)

// Replace with your network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Create HX711 and TFT display objects
HX711 scale;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

WebServer server(80);  // Create web server object on port 80

// Global variable for storing weight
float weight = 0;
bool wifiConnected = false;  // Tracks WiFi connection status

// Function to read the current weight from HX711
float getWeight() {
  if (scale.is_ready()) {
    return scale.get_units(10);  // Average of 10 readings
  } else {
    return 0.0;
  }
}

// Function to handle HTTP GET request and return the current weight
void handleRoot() {
  weight = getWeight();
  String response = "Current Weight: " + String(weight, 2) + " kg";
  server.send(200, "text/plain", response);
}

// Function to draw the Wi-Fi icon
void drawWiFiIcon(bool connected) {
  int x = 200;  // X position for the top-right corner
  int y = 10;   // Y position
  
  // Clear previous Wi-Fi icon
  tft.fillRect(x, y, 40, 30, ILI9341_BLACK); // Erase the previous icon
  
  if (connected) {
    // Draw Wi-Fi icon in green
    tft.drawLine(x + 5, y + 20, x + 15, y + 10, ILI9341_GREEN);  // Bottom arc
    tft.drawLine(x + 15, y + 10, x + 25, y + 20, ILI9341_GREEN); // Bottom arc
    tft.drawLine(x + 8, y + 15, x + 15, y + 8, ILI9341_GREEN);   // Middle arc
    tft.drawLine(x + 15, y + 8, x + 22, y + 15, ILI9341_GREEN);  // Middle arc
    tft.fillCircle(x + 15, y + 22, 3, ILI9341_GREEN);            // Dot at the bottom
  } else {
    // Draw Wi-Fi icon in red
    tft.drawLine(x + 5, y + 20, x + 15, y + 10, ILI9341_RED);    // Bottom arc
    tft.drawLine(x + 15, y + 10, x + 25, y + 20, ILI9341_RED);   // Bottom arc
    tft.drawLine(x + 8, y + 15, x + 15, y + 8, ILI9341_RED);     // Middle arc
    tft.drawLine(x + 15, y + 8, x + 22, y + 15, ILI9341_RED);    // Middle arc
    tft.fillCircle(x + 15, y + 22, 3, ILI9341_RED);              // Dot at the bottom
  }
}

// Function to setup WiFi and web server
void setupWiFi() {
  WiFi.begin(ssid, password);
  int attempt = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    attempt++;
    drawWiFiIcon(false); // Display red Wi-Fi icon while trying to connect
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    drawWiFiIcon(true);  // Display green Wi-Fi icon when connected
  } else {
    wifiConnected = false;
    drawWiFiIcon(false); // Keep red Wi-Fi icon if connection failed
  }
  
  // Start web server
  if (wifiConnected) {
    server.on("/", handleRoot);  // Root URL to display weight
    server.begin();
  }
}

// Function to display weight on the TFT LCD
void displayWeight(float weight) {
  tft.fillScreen(ILI9341_BLACK);         // Clear the screen with black background
  tft.setTextColor(ILI9341_WHITE);       // Set text color to white
  tft.setTextSize(2);                    // Set text size (larger font)
  tft.setCursor(10, 50);                 // Set text starting position
  tft.print("Weight:");
  
  tft.setTextColor(ILI9341_YELLOW);      // Set text color to yellow for the weight
  tft.setCursor(10, 100);                // Position for the weight value
  tft.print(weight, 2);                  // Display the weight with 2 decimal places
  tft.print(" kg");
  
  // Always display the Wi-Fi icon in the top right corner
  drawWiFiIcon(wifiConnected);
}

// Setup function to initialize everything
void setup() {
  // Initialize the scale
  scale.begin(DOUT, CLK);
  scale.set_scale();  // Set scale factor, adjust based on calibration
  scale.tare();       // Reset the scale to 0

  // Initialize TFT display
  tft.begin();
  tft.setRotation(1);  // Landscape mode, change if needed

  // If the ESP32 wakes up from deep sleep, it should check weight
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    weight = getWeight();
    displayWeight(weight); // Display weight on TFT
    
    // Start WiFi and web server
    setupWiFi();
  }

  // Cast WAKE_PIN to gpio_num_t to avoid the type error
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 1);  // Use pin 33 to wake up

  delay(10000);  // Keep awake for 10 seconds before sleep
  esp_deep_sleep_start();  // Enter deep sleep mode
}

// Main loop
void loop() {
  // Handle any HTTP requests
  if (wifiConnected) {
    server.handleClient();
  }
}
