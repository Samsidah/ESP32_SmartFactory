#include <WiFi.h>
#include <esp_now.h>

// Pins for RGB LED
#define LED_RED_PIN 25
#define LED_GREEN_PIN 26
#define LED_BLUE_PIN 27

// Pins for relay modules controlling light bulbs
#define RELAY1_PIN 33 // Relay for Light Bulb 1
#define RELAY2_PIN 32 // Relay for Light Bulb 2

// Data structures to receive
typedef struct {
  int id;                // Board ID
  int smokeDetected;     // 0 = No smoke, 1 = Smoke detected
  float ppm;             // PPM value of smoke
  unsigned long smokeDetectedCount; // Smoke detected count
} SmokeData;

typedef struct {
  int id;               // Device ID
  int peopleIn;         // Number of people IN
  int tagScanned;       // 1 = Tag scanned, 0 = No scan
  char scannedUID[32];  // Last scanned UID
  char scannedLabel[32]; // Last scanned label
} RFIDData;

// State variables
SmokeData incomingSmokeData;
RFIDData incomingRFIDData;

bool newSmokeData = false; // Flag to indicate new smoke data received
bool newRFIDData = false;  // Flag to indicate new RFID data received
bool smokeSensorEnabled = false; // Flag to track smoke sensor state

// ESP-NOW callback for receiving data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  if (data_len == sizeof(SmokeData)) {
    memcpy(&incomingSmokeData, data, sizeof(SmokeData));
    newSmokeData = true; // Set the flag
  } else if (data_len == sizeof(RFIDData)) {
    memcpy(&incomingRFIDData, data, sizeof(RFIDData));
    newRFIDData = true; // Set the flag
  } else {
    Serial.println("Unknown or corrupted data received.");
  }
}

// Handles actions when RFID data is processed
void handleRFIDData() {
  if (incomingRFIDData.peopleIn >= 1) {
    // Turn on the LED (blue) and light bulbs via relays
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_BLUE_PIN, HIGH);
    digitalWrite(RELAY1_PIN, LOW); // Relay ON (bulb ON)
    digitalWrite(RELAY2_PIN, LOW); // Relay ON (bulb ON)
    smokeSensorEnabled = true;
    Serial.println("RFID: People detected. Lights on, LED blue, smoke sensor active.");
  } else {
    // Turn off the LED and light bulbs via relays
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, LOW);         
    digitalWrite(LED_BLUE_PIN, LOW);
    digitalWrite(RELAY1_PIN, HIGH); // Relay OFF (bulb OFF)
    digitalWrite(RELAY2_PIN, HIGH); // Relay OFF (bulb OFF)
    smokeSensorEnabled = false;
    Serial.println("RFID: No people detected. Lights and LED off, smoke sensor inactive.");
  }
}

// Handles actions when smoke is detected
void handleSmokeDetected() {
  // Turn off the LED and light bulbs via relays
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(RELAY1_PIN, HIGH); // Relay OFF (bulb OFF)
  digitalWrite(RELAY2_PIN, HIGH); // Relay OFF (bulb OFF)
  Serial.println("ALERT: Smoke detected! Lights and LED off.");
}

// Handles actions when no smoke is detected
void handleNoSmoke() {
  // Turn on the LED (green) and light bulbs via relays
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW); // Relay ON (bulb ON)
  digitalWrite(RELAY2_PIN, LOW); // Relay ON (bulb ON)
  Serial.println("System normal: No smoke detected. Lights on, LED green.");
}

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi in STA mode
  WiFi.mode(WIFI_STA);
  Serial.println("Receiver initialized");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Register callback for received data
  esp_now_register_recv_cb(OnDataRecv);

  // Setup LED pins
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  // Setup relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Initialize relays to OFF state
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  // Initialize LED and relays to "no smoke" state
  //handleNoSmoke();
}

void loop() {
  if (newRFIDData) {
    newRFIDData = false; // Reset the flag
    Serial.println("RFID Data received:");
    handleRFIDData(); // Process RFID data
  }

  // Process smoke data only if smoke sensor is enabled
  if (smokeSensorEnabled && newSmokeData) {
    newSmokeData = false; // Reset the flag
    Serial.println("Smoke Data received:");
    Serial.print("Device ID: ");
    Serial.println(incomingSmokeData.id);
    Serial.print("Smoke Detected: ");
    Serial.println(incomingSmokeData.smokeDetected ? "Yes" : "No");
    Serial.print("PPM: ");
    Serial.println(incomingSmokeData.ppm, 2);
    Serial.print("Smoke Detected Count: ");
    Serial.println(incomingSmokeData.smokeDetectedCount);

    // Update the system state based on the smokeDetected value
    if (incomingSmokeData.smokeDetected) {
      handleSmokeDetected();
    } else {
      handleNoSmoke();
    }
  }
}
