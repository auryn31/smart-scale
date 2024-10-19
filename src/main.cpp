#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"                 // HX711 library for load cell weight sensor
#include <Adafruit_GFX.h>          // Graphics library for TFT display
#include <Adafruit_ILI9341.h>      // TFT library for ILI9341-based displays
#include <esp_sleep.h>             // Include for deep sleep functions

// ------------------------ Pin Definitions ------------------------ //
// TFT LCD (ILI9341) via SPI
#define TFT_CS   5   // GPIO5 for Chip Select (CS)
#define TFT_DC   6   // GPIO6 for Data/Command (DC)
#define TFT_RST  7   // GPIO7 for Reset (RST)
#define TFT_MOSI 8   // GPIO8 for MOSI
#define TFT_CLK  9   // GPIO9 for Clock (SCK)

// HX711 (Load Cell)
#define DOUT  3     // GPIO3 for HX711 DOUT pin
#define CLK   4     // GPIO4 for HX711 CLK pin

// Wake-Up Pin
#define WAKE_PIN 2  // GPIO2 for waking up from deep sleep

// ------------------------ Wi-Fi Credentials ------------------------ //
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ------------------------ Object Instantiations ------------------------ //
// Create HX711 and TFT display objects
HX711 scale;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Create web server object on port 80
WebServer server(80);

// ------------------------ Global Variables ------------------------ //
float weight = 0;
bool wifiConnected = false;  // Tracks WiFi connection status

// ------------------------ Function Declarations ------------------------ //

/**
 * @brief Reads the current weight from the HX711 load cell.
 * 
 * @return float The measured weight in kg.
 */
float getWeight() {
  if (scale.is_ready()) {
    return scale.get_units(10);  // Average of 10 readings for stability
  } else {
    return 0.0;
  }
}

/**
 * @brief Handles HTTP GET requests to the root URL and returns the current weight.
 */
void handleRoot() {
  weight = getWeight();
  String response = "Current Weight: " + String(weight, 2) + " kg";
  server.send(200, "text/plain", response);
}

/**
 * @brief Draws a simple Wi-Fi icon on the TFT LCD.
 * 
 * @param connected True if Wi-Fi is connected, False otherwise.
 */
void drawWiFiIcon(bool connected) {
  int iconSize = 20; // Size of the icon
  int x = tft.width() - iconSize - 10;  // X position for the top-right corner
  int y = 10;   // Y position
  
  // Clear previous Wi-Fi icon area
  tft.fillRect(x, y, iconSize, iconSize, ILI9341_BLACK); // Adjust width as needed

  // Define colors based on connection status
  uint16_t color = connected ? ILI9341_GREEN : ILI9341_RED;

  // Draw Wi-Fi icon (simplified version)
  // Draw three arcs and a dot to represent Wi-Fi signal
  // Since Adafruit_GFX doesn't have an arc function, we'll use lines to approximate

  // Bottom arc
  tft.drawLine(x + 5, y + 15, x + 10, y + 10, color);
  tft.drawLine(x + 10, y + 10, x + 15, y + 15, color);

  // Middle arc
  tft.drawLine(x + 5, y + 20, x + 10, y + 15, color);
  tft.drawLine(x + 10, y + 15, x + 15, y + 20, color);

  // Top arc
  tft.drawLine(x + 5, y + 25, x + 10, y + 20, color);
  tft.drawLine(x + 10, y + 20, x + 15, y + 25, color);

  // Dot
  tft.fillCircle(x + 10, y + 25, 2, color);
}

/**
 * @brief Sets up Wi-Fi connection and starts the web server.
 */
void setupWiFi() {
  Serial.println("Setup Wifi");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  int attempt = 0;
  const int maxAttempts = 20; // Total of 10 seconds (20 * 500ms)

  while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
    delay(500);
    Serial.print(".");
    drawWiFiIcon(false); // Display red Wi-Fi icon while trying to connect
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWi-Fi Connected!");
    drawWiFiIcon(true);  // Display green Wi-Fi icon when connected

    // Start web server
    server.on("/", handleRoot);  // Root URL to display weight
    server.begin();
    Serial.println("Web server started.");
  } else {
    wifiConnected = false;
    Serial.println("\nFailed to connect to Wi-Fi.");
    drawWiFiIcon(false); // Keep red Wi-Fi icon if connection failed
  }
}

/**
 * @brief Displays the current weight on the TFT LCD along with the Wi-Fi status icon.
 * 
 * @param weight The weight value to display.
 */
void displayWeight(float weight) {
  tft.fillScreen(ILI9341_RED);         // Clear the screen with black background
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // White text with black background
  tft.setTextSize(3);                     // Set text size (larger font)
  tft.setCursor(20, 50);                  // Set text starting position
  tft.print("Weight:");

  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK); // Yellow text with black background
  tft.setTextSize(4);                    // Larger text for weight value
  tft.setCursor(20, 100);                // Position for the weight value
  tft.print(weight, 2);                  // Display the weight with 2 decimal places
  tft.print(" kg");

  // Always display the Wi-Fi icon in the top right corner
  drawWiFiIcon(wifiConnected);
}

// ------------------------ Setup Function ------------------------ //
void setup() {
  // setup serial debuggin
  Serial.begin(9600);
  Serial.println("Setup");
  // Initialize the scale
  // scale.begin(DOUT, CLK);
  // scale.set_scale();  // Set scale factor, adjust based on calibration
  // scale.tare();       // Reset the scale to 0

  // Initialize TFT display
  tft.begin();
  tft.setRotation(1);  // Landscape mode, change if needed

  // If the ESP32 wakes up from deep sleep, it should check weight
  // if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
  //   weight = getWeight();
    displayWeight(10.2); // Display weight on TFT
    
  //   // Start WiFi and web server
  //   setupWiFi();
  // }

  // Enable GPIO wakeup on the WAKE_PIN (GPIO2) and configure the wakeup trigger
  // esp_sleep_enable_gpio_wakeup();  // Enable GPIO wakeup
  // gpio_set_direction((gpio_num_t)WAKE_PIN, GPIO_MODE_INPUT);  // Set pin as input
  // gpio_wakeup_enable((gpio_num_t)WAKE_PIN, GPIO_INTR_LOW_LEVEL);  // Wakeup on low level

  // delay(10000);  // Keep awake for 10 seconds before sleep
  // esp_deep_sleep_start();  // Enter deep sleep mode
}


// ------------------------ Loop Function ------------------------ //

void loop() {
  // Handle any HTTP requests if Wi-Fi is connected
  if (wifiConnected) {
    server.handleClient();
  }
  Serial.println("Test");

  // Optionally, you can add a small delay to prevent the loop from running too fast
  delay(10);
}