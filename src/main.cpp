#include <WiFi.h>
#include "wifi_bitmap.h"
#include "wifi_credentials.h"
#include <WebServer.h>
#include "HX711.h"           // HX711 library for load cell weight sensor
#include <Adafruit_GFX.h>    // Graphics library for TFT display
#include <Adafruit_ST7789.h> // TFT library for ILI9341-based displays

// Pin Definitions for TFT (SPI)
#define TFT_CS 5
#define TFT_DC 17
#define TFT_RST 16
#define TFT_MOSI 23
#define TFT_CLK 18

// Pin Definitions for HX711 (Weight sensor)
#define DOUT 32     // HX711 DOUT pin
#define CLK 33      // HX711 CLK pin
#define WAKE_PIN 23 // Pin to wake up from deep sleep (use a touch pin or external interrupt)

// Replace with your network credentials
const char *ssid = wifiSSID;
const char *password = wifiPassword;

#define ILI9341_TFTWIDTH 320  ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 240 ///< ILI9341 max TFT height

// Create HX711 and TFT display objects
HX711 scale;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

WebServer server(80); // Create web server object on port 80

// Global variable for storing weight
float weight = 0;
bool wifiConnected = false; // Tracks WiFi connection status
float calibration_factor = -29845.35;

// Function to read the current weight from HX711
float getWeight()
{
  // Serial.println("Current weight: " + String(weight, 2));
  Serial.print("Ready: ");
  Serial.println(scale.is_ready());
  Serial.println("Scale factor " + String(calibration_factor, 3));
  if (scale.is_ready())
  {
    Serial.println("Return reading");
    Serial.println(scale.get_units(), 1);

    return scale.get_units(10); // Average of 10 readings
  }
  else
  {
    Serial.println("Return 0.0");
    return 0.0;
  }
}

// Function to handle HTTP GET request and return the current weight
void handleRoot()
{
  String response = String(weight, 2);
  server.send(200, "text/plain", response);
}

// Function to draw the Wi-Fi icon
void drawWiFiIcon(bool connected)
{
  int x = 260; // X position for the top-right corner
  int y = 10;  // Y position

  if (connected)
  {
    tft.drawBitmap(x, y, wifi_icon, 48, 48, ST77XX_BLACK, ST77XX_GREEN);
  }
  else
  {
    tft.drawBitmap(x, y, wifi_icon, 48, 48, ST77XX_BLACK, ST77XX_RED);
  }
}

// Function to setup WiFi and web server
void setupWiFi()
{
  WiFi.begin(ssid, password);
  int attempt = 0;

  while (WiFi.status() != WL_CONNECTED && attempt < 20)
  {
    delay(500);
    attempt++;
    drawWiFiIcon(false); // Display red Wi-Fi icon while trying to connect
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiConnected = true;
    drawWiFiIcon(true); // Display green Wi-Fi icon when connected
  }
  else
  {
    wifiConnected = false;
    drawWiFiIcon(false); // Keep red Wi-Fi icon if connection failed
  }

  // Start web server
  if (wifiConnected)
  {
    server.on("/", handleRoot); // Root URL to display weight
    server.begin();
  }
}

// Function to display weight on the TFT LCD
void displayWeight(float weight)
{
  tft.setTextColor(ST77XX_WHITE); // Set text color to white
  tft.setTextSize(4);             // Set text size (larger font)
  tft.setCursor(20, 30);          // Set text starting position
  tft.print("Weight:");

  tft.fillRect(10, 80, 300, 200, ST77XX_BLACK); // Erase the previous icon

  tft.setTextColor(ST77XX_YELLOW); // Set text color to yellow for the weight
  tft.setCursor(10, 80);           // Position for the weight value
  tft.print(weight, 2);            // Display the weight with 2 decimal places
  tft.print(" kg");

  // Always display the Wi-Fi icon in the top right corner
  drawWiFiIcon(wifiConnected);
}

void calculate_scale_factor(float known_weight)
{
  Serial.println("Calculating scale factor...");

  // Get raw reading from the HX711
  long reading = scale.get_value(10); // Average of 10 readings
  Serial.print("Raw reading from HX711: ");
  Serial.println(reading);

  // Calculate scale factor (raw reading divided by known weight)
  calibration_factor = reading / known_weight;

  Serial.print("Calculated calibration factor: ");
  Serial.println(calibration_factor);

  // Set the scale to the new calibration factor
  scale.set_scale(calibration_factor);
  Serial.println("Scale calibrated successfully!");
}
// Setup function to initialize everything
void setup()
{
  // Initialize the scale
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor); // Set scale factor, adjust based on calibration
  scale.tare();                        // Reset the scale to 0
  long avg = scale.read_average();
  Serial.begin(9600);
  Serial.println(avg);

  // // Initialize TFT display
  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(3); // Landscape mode, change if needed

  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK); // Clear the screen with black background
  // displayWeight(10.2);

  // // If the ESP32 wakes up from deep sleep, it should check weight
  // if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
  //   weight = getWeight();
  //   displayWeight(weight); // Display weight on TFT

  //   // Start WiFi and web server
  setupWiFi();
  // }

  // // Cast WAKE_PIN to gpio_num_t to avoid the type error
  // esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 1);  // Use pin 33 to wake up

  // delay(10000);  // Keep awake for 10 seconds before sleep
  // esp_deep_sleep_start();  // Enter deep sleep mode
  // Serial.println("Place a known weight on the scale...");
  // delay(4000);
  // Serial.println("Start");

  // calculate_scale_factor(4.3);
}

// Main loop
void loop()
{
  // Handle any HTTP requests
  if (wifiConnected)
  {
    server.handleClient();
  }
  delay(50);
  // weight += 5.9;
  weight = getWeight();
  displayWeight(weight);
}
