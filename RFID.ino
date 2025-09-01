#include <WiFi.h>
#include <esp_now.h>
#include <MFRC522.h>
#include <map>

// Constants
#define ESPNOW_WIFI_CHANNEL 6
#define RST_PIN 22
#define SS_PIN 21

// RFID setup
MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

// ESP-NOW receiver MAC addresses (replace with actual values)
uint8_t receiverAddress1[] = {0xD0, 0xEF, 0x76, 0x5B, 0x99, 0xE0}; // SERVER
uint8_t receiverAddress2[] = {0xCC, 0x7B, 0x5C, 0x36, 0x16, 0x28}; // CONTROLLER

// Data structure to send
typedef struct {
  int id;               // Device ID
  int peopleIn;         // Number of people IN
  int tagScanned;       // 1 = Tag scanned, 0 = No scan
  char scannedUID[32];  // Last scanned UID
  char scannedLabel[32]; // Last scanned label
} RFIDData;

RFIDData rfidData;

// Tracking personnel
int peopleIn = 0;
std::map<String, bool> tagState;   // Maps UID to IN/OUT state
std::map<String, String> tagLabels; // Maps UID to label (e.g., name)

// ESP-NOW callback for send status
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status to ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " Success" : " Fail");
}

// Function to send RFID data via ESP-NOW
void sendRFIDData() {
  uint8_t *receivers[] = {receiverAddress1, receiverAddress2};
  const char *receiverNames[] = {"SERVER", "CONTROLLER"};
  
  for (int i = 0; i < 2; i++) {
    Serial.printf("Attempting to send RFID data to %s...\n", receiverNames[i]);
    esp_err_t result = esp_now_send(receivers[i], (uint8_t *)&rfidData, sizeof(rfidData));
    if (result == ESP_OK) {
      Serial.printf("RFID DATA sent to %s successfully\n", receiverNames[i]);
    } else {
      Serial.printf("Failed to send RFID DATA to %s. Error code: %d\n", receiverNames[i], result);
    }
  }
}

// Function to assign labels to RFID tags
void assignLabelToTag(String tagUID, String label) {
  tagLabels[tagUID] = label; // Assign the label (name) to the UID
}

// Function to get the RFID UID as a string
String getTagUID() {
  String tagUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    tagUID += String(rfid.uid.uidByte[i], HEX);
  }
  tagUID.toUpperCase(); // Convert to uppercase for consistency
  return tagUID;
}

void setup() {
  Serial.begin(115200);

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID initialized");

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Register callback for send status
  esp_now_register_send_cb(OnDataSent);

  // Add receiver peers
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo.encrypt = false;

  memcpy(peerInfo.peer_addr, receiverAddress1, 6);
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("SERVER added successfully as a peer");
  } else {
    Serial.println("Failed to add SERVER as a peer");
  }

  memcpy(peerInfo.peer_addr, receiverAddress2, 6);
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("CONTROLLER added successfully as a peer");
  } else {
    Serial.println("Failed to add CONTROLLER as a peer");
  }

  // Assign labels to known RFID tags
  assignLabelToTag("576A489", "John Doe");
  assignLabelToTag("2AD28617", "James Brown");

  Serial.println("System initialized. Ready to scan RFID cards...");
}

void loop() {
  // Check for RFID tags
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  // Read tag UID
  String tagUID = getTagUID();
  if (tagUID.isEmpty()) {
    Serial.println("Error: Empty UID");
    return;
  }

  Serial.println("Scanned UID: " + tagUID);

  // Display scanned UID and corresponding label
  String label = (tagLabels.find(tagUID) != tagLabels.end()) ? tagLabels[tagUID] : "[Unnamed Tag]";
  Serial.println("Name: " + label);

  // Toggle IN/OUT state for the tag
  if (tagState[tagUID]) {
    tagState[tagUID] = false; // Mark as OUT
    peopleIn--;
    Serial.println("Status: Marked OUT");
  } else {
    tagState[tagUID] = true; // Mark as IN
    peopleIn++;
    Serial.println("Status: Marked IN");
  }

  // Update data to send
  rfidData.id = 3; // Unique ID for this device
  rfidData.peopleIn = peopleIn;
  rfidData.tagScanned = 1;
  strncpy(rfidData.scannedUID, tagUID.c_str(), sizeof(rfidData.scannedUID) - 1);
  strncpy(rfidData.scannedLabel, label.c_str(), sizeof(rfidData.scannedLabel) - 1);

  // Send data via ESP-NOW
  sendRFIDData();

  // Display updated people count
  Serial.println("People IN: " + String(peopleIn));

  // Halt RFID detection briefly to avoid repeated scans
  delay(2000);
}
