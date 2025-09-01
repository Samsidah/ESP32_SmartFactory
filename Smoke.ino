#include <WiFi.h>
#include <esp_now.h>

#define ESPNOW_WIFI_CHANNEL 6  // Match this channel with the receiver's Wi-Fi channel
#define MQ9_SENSOR_PIN 34      // Keep as is or change to the desired analog pin
#define BUZZER_PIN 19          // Keep as is or change to the desired buzzer pin
#define SAMPLING_RATE 5000     // Desired sampling rate in milliseconds (5 seconds)

// Data structure to send smoke data
typedef struct {
  int id;                // Board ID
  int smokeDetected;     // 0 = No smoke, 1 = Smoke detected
  float ppm;             // PPM value of smoke
  unsigned long smokeDetectedCount; // Smoke detected count
} SmokeData;

SmokeData smokeData;  // Create an instance of SmokeData

// Counter for smoke events
unsigned long smokeDetectedCount = 0;  // Counts smoke detection events

// Peer MAC addresses for ESP-NOW communication (update these with the receiver's MAC addresses)
uint8_t peerAddress1[] = {0xD0, 0xEF, 0x76, 0x5B, 0x99, 0xE0};  // First receiver MAC address
uint8_t peerAddress2[] = {0xCC, 0x7B, 0x5C, 0x36, 0x16, 0x28};  // Second receiver MAC address

// Function to read the MQ-9 sensor and calculate PPM value
float readMQ9Sensor() {
  int sensorValue = analogRead(MQ9_SENSOR_PIN);       // Read the analog value from the sensor
  float voltage = sensorValue * (3.3 / 4095.0);       // Convert raw value to voltage (assuming 3.3V reference)
  float ppm = voltage * 1000;                         // Simplified conversion to PPM
  return ppm;
}

// ESP-NOW callback for data send status
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status to ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " Success" : " Fail");
}

// Function to send smoke data
void sendSmokeData() {
  // Read the smoke sensor value (PPM)
  float ppmValue = readMQ9Sensor();

  // Determine if smoke is detected
  smokeData.smokeDetected = (ppmValue > 500) ? 1 : 0;

  // Update counter if smoke is detected
  if (smokeData.smokeDetected) {
    smokeDetectedCount++;
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // Turn off the buzzer
  }

  // Populate the smoke data structure
  smokeData.id = 1;                  // Sender's unique ID
  smokeData.ppm = ppmValue;          // Smoke concentration in PPM
  smokeData.smokeDetectedCount = smokeDetectedCount;  // Smoke detected count

  // Send data to the first receiver
  esp_err_t result1 = esp_now_send(peerAddress1, (uint8_t *)&smokeData, sizeof(SmokeData));
  // Send data to the second receiver
  esp_err_t result2 = esp_now_send(peerAddress2, (uint8_t *)&smokeData, sizeof(SmokeData));

  // Debugging: Print data to the serial monitor
  Serial.print("Sending Smoke Data | Smoke Detected: ");
  Serial.print(smokeData.smokeDetected);
  Serial.print(", PPM: ");
  Serial.println(smokeData.ppm);
  Serial.print("Send Result to Peer 1: ");
  Serial.println(result1 == ESP_OK ? "Success" : "Fail");
  Serial.print("Send Result to Peer 2: ");
  Serial.println(result2 == ESP_OK ? "Success" : "Fail");

  // Print smoke detected count
  Serial.print("Count: ");
  Serial.println(smokeDetectedCount);
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Set up the MQ-9 sensor pin
  pinMode(MQ9_SENSOR_PIN, INPUT);

  // Set up the buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);

  // Set the ESP-NOW communication channel
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  // Register callback for send status
  esp_now_register_send_cb(OnDataSent);

  // Register the first receiver as a peer
  esp_now_peer_info_t peerInfo1;
  memset(&peerInfo1, 0, sizeof(peerInfo1));
  memcpy(peerInfo1.peer_addr, peerAddress1, 6);
  peerInfo1.channel = ESPNOW_WIFI_CHANNEL;  // Same channel as receiver
  peerInfo1.encrypt = false;

  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Failed to add first peer");
    return;
  } else {
    Serial.println("First peer added successfully");
  }

  // Register the second receiver as a peer
  esp_now_peer_info_t peerInfo2;
  memset(&peerInfo2, 0, sizeof(peerInfo2));
  memcpy(peerInfo2.peer_addr, peerAddress2, 6);
  peerInfo2.channel = ESPNOW_WIFI_CHANNEL;  // Same channel as receiver
  peerInfo2.encrypt = false;

  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Failed to add second peer");
    return;
  } else {
    Serial.println("Second peer added successfully");
  }
}

// Main loop
void loop() {
  static unsigned long lastSampleTime = 0; // Tracks the last sampling time
  unsigned long currentMillis = millis(); // Current time in milliseconds

  if (currentMillis - lastSampleTime >= SAMPLING_RATE) {
    lastSampleTime = currentMillis;  // Update last sampling time
    sendSmokeData();                 // Send smoke data to both peers
  }

  // Optional: Add other tasks or low-power sleep mode here
}
