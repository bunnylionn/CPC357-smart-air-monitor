#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>

// =============================================================
// 1. CONFIGURATION
// =============================================================

// Wi-Fi Credentials
const char* ssid = "Galaxy S23 Ultra";       
const char* password = "jannah03"; 

// Google Cloud Function URL
const char* serverUrl = "https://api-receiver-air-data-607019422067.asia-southeast1.run.app";

// Pin Definitions 
#define GAS_PIN       A2    // Analog pin for MQ-135
#define BUZZER_PIN    12    // Built-in Piezo Buzzer
#define NEOPIXEL_PIN  46    // Built-in NeoPixel LED
#define NUM_PIXELS    1     // Number of NeoPixels

// Thresholds
const int GAS_THRESHOLD = 1300; 

// Initialize NeoPixel
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// =============================================================
// 2. SETUP
// =============================================================
void setup() {
  // Start Serial Console
  Serial.begin(115200);
  delay(2000); // Give time to open Serial Monitor

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GAS_PIN, INPUT);
  
  // Initialize NeoPixel
  pixels.begin();
  pixels.setBrightness(50); // 0-255 brightness
  pixels.show(); // Initialize all pixels to 'off'

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  // Blink Blue while connecting
  while (WiFi.status() != WL_CONNECTED) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Blue
    pixels.show();
    delay(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Off
    pixels.show();
    delay(250);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Indicate Connection Success (Green Blink)
  for(int i=0; i<3; i++) {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green
    pixels.show();
    delay(200);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(200);
  }
}

// =============================================================
// 3. MAIN LOOP
// =============================================================
void loop() {
  // --- A. Read Sensor ---
  int gasValue = analogRead(GAS_PIN); // Reads 0 to 4095 (12-bit ADC)
  
  Serial.println("-----------------------------");
  Serial.print("Gas Sensor Value: ");
  Serial.println(gasValue);

  // --- B. Local Alert Logic (Immediate Safety) ---
  if (gasValue > GAS_THRESHOLD) {
    // DANGER: Red LED + Buzzer Sound
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red
    pixels.show();
    
    // Beep the buzzer
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    
    Serial.println("STATUS: WARNING! High Gas Levels.");
  } else {
    // NORMAL: Green LED + No Sound
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green
    pixels.show();
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("STATUS: Normal.");
  }

  // --- C. Send to Google Cloud ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.print("Sending data to Cloud... ");
    
    // 1. Begin Connection
    http.begin(serverUrl);
    
    // 2. Add Headers
    http.addHeader("Content-Type", "application/json");

    // 3. Create JSON Payload
    // Format must match Node.js: {"gas_value": 1234}
    String jsonPayload = "{\"gas_value\": " + String(gasValue) + "}";
    
    // 4. POST Request
    int httpResponseCode = http.POST(jsonPayload);
    
    // 5. Check Response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Success! Code: ");
      Serial.print(httpResponseCode);
      Serial.print(" | Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error. Code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end(); // Free resources
  } else {
    Serial.println("Error: WiFi Disconnected");
  }

  // --- D. Wait before next reading ---
  delay(5000); // Wait 5 seconds
}
