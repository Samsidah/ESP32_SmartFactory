#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <esp_wifi.h>
#include <indexhtml.h>

// --- Definitions and Constants ---
#define ESPNOW_WIFI_CHANNEL 6

// WiFi credentials
const char* ssid = "Aqua";
const char* password = "0987uiop";

// --- Data Structures ---
typedef struct {
  int id;                // Board ID
  int smokeDetected;     // 0 = No smoke, 1 = Smoke detected
  float ppm;             // PPM value of smoke
  unsigned long smokeDetectedCount; // Smoke detected count
} SmokeData;

typedef struct {
  int id;                // Board ID
  int objectDetected1;   
  float distance1;       
  int objectDetected2;   
  float distance2;       
  unsigned long detectionCount1; 
  unsigned long detectionCount2;
} UltrasonicData;

typedef struct {
  int id;             // Device ID
  int peopleIn;       // Number of people IN
  int tagScanned;     // 1 = Tag scanned, 0 = No scan
  char scannedUID[32]; // Last scanned UID
  char scannedLabel[32]; // Last scanned label
} RFIDData;

// --- Global Variables ---
SmokeData smokeData;         // Smoke sensor data
UltrasonicData ultrasonicData; // Ultrasonic sensor data
RFIDData rfidData;


// Peer MAC addresses for ESP-NOW communication
uint8_t peerAddress[] = {0x10, 0x06, 0x1C, 0x81, 0xFD, 0x58};  // Eilex; smoke
uint8_t peerAddress2[] = {0xD0, 0xEF, 0x76, 0x5C, 0x58, 0xFC}; // Sammy; ultrasonic
uint8_t peerAddress3[] = {0x88, 0x13, 0xBF, 0x0D, 0x6B, 0xB0}; // Kizah; RFID

// WiFi Server
WiFiServer server(80);

// --- Callback Function ---
void onDataReceive(const uint8_t *macAddr, const uint8_t *incomingData, int len) {
  
  if (len == sizeof(SmokeData)) {
    memcpy(&smokeData, incomingData, sizeof(SmokeData));
    Serial.println("Smoke Sensor Data Received");
    Serial.printf("Board ID: %d, PPM: %.2f, Smoke Detected Count: %lu\n", 
                  smokeData.id, smokeData.ppm, smokeData.smokeDetectedCount);
  } else if (len == sizeof(UltrasonicData)) {
    memcpy(&ultrasonicData, incomingData, sizeof(UltrasonicData));
    Serial.println("Ultrasonic Sensor Data Received");
    Serial.printf("Board ID: %d\n", ultrasonicData.id);
    Serial.printf("Sensor 1 - Detected: %d, Distance: %.2f cm, Count: %lu\n",
                  ultrasonicData.objectDetected1, ultrasonicData.distance1, ultrasonicData.detectionCount1);
    Serial.printf("Sensor 2 - Detected: %d, Distance: %.2f cm, Count: %lu\n",
                  ultrasonicData.objectDetected2, ultrasonicData.distance2, ultrasonicData.detectionCount2);
  } else if (len == sizeof(RFIDData)){
    memcpy(&rfidData, incomingData, sizeof(RFIDData));
    Serial.println("RFID Sensor Data Received");
    Serial.printf("Board ID: %d, Personnel Count: %d\n", 
                  rfidData.id, rfidData.peopleIn);
  } else {
    Serial.println("Invalid data received.");
  }
}


void sendDataToPeers() {
  // Forward smoke data
  if (esp_now_send(peerAddress, (uint8_t *)&smokeData, sizeof(smokeData)) != ESP_OK) {
    Serial.println("Failed to send smoke data.");
  } else {
    Serial.println("Smoke Data sent successfully.");
  }

  // Forward ultrasonic data
  if (esp_now_send(peerAddress2, (uint8_t *)&ultrasonicData, sizeof(ultrasonicData)) != ESP_OK) {
    Serial.println("Failed to send ultrasonic data.");
  } else {
    Serial.println("Ultrasonic Data sent successfully.");
  }

  // Forward RFID data
  if (esp_now_send(peerAddress3, (uint8_t *)&rfidData, sizeof(rfidData)) != ESP_OK) {
    Serial.println("Failed to send RFID data.");
  }else {
    Serial.println("RFID Data sent successfully.");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status to ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " Success" : " Fail");
}


// --- Setup Function ---
void setup() {
  Serial.begin(115200);

  // Initialize WiFi in station mode for ESP-NOW
  WiFi.mode(WIFI_STA);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());

  // Start the server
  server.begin();
  esp_now_register_recv_cb(onDataReceive);

  // Add ESP-NOW peers
  esp_now_peer_info_t peerInfo1, peerInfo2, peerInfo3, peerInfo4;
  memcpy(peerInfo1.peer_addr, peerAddress, 6);
  peerInfo1.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo1.encrypt = false;
  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Failed to add smoke peer");
  } else {
    Serial.println("Smoke peer successfully added");
  }

  memcpy(peerInfo2.peer_addr, peerAddress2, 6);
  peerInfo2.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo2.encrypt = false;
  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Failed to add ultrasonic peer");
  } else {
    Serial.println("Ultrasonic peer successfully added");
  }

  memcpy(peerInfo3.peer_addr, peerAddress3, 6);
  peerInfo3.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo3.encrypt = false;
  esp_now_add_peer(&peerInfo3);
  if (esp_now_add_peer(&peerInfo3) != ESP_OK) {
    Serial.println("Failed to add RFID Sensor peer");
  } else {
    Serial.println("RFID Sensor peer successfully added");
  }
}

// --- Main Loop ---
void loop() {
  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    // Handle Smoke Data Request
    if (request.indexOf("GET /smoke") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();

      String smokeMessage;
      if (smokeData.id != 0) {
        smokeMessage = "Board ID: " + String(smokeData.id) + "<br>";
        smokeMessage += smokeData.smokeDetected ? 
                        "<span class='warning'>Smoke Detected!</span><br>" : 
                        "<span class='safe'>No Smoke Detected</span><br>";
        smokeMessage += "PPM: " + String(smokeData.ppm) + "<br>";
      } else {
        smokeMessage = "No smoke data received yet.";
      }
      client.print(smokeMessage);
      Serial.println(smokeMessage);
    }

    // Handle Ultrasonic Data Request
    else if (request.indexOf("GET /ultrasonic") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();

      String ultrasonicMessage1;
     if (ultrasonicData.distance1 <= 5 && ultrasonicData.distance1 != 0){
        ultrasonicMessage1 = "<span class='warning'>WARNING! </span><br>Someone is Approaching Machine 1!<br>Distance: " + String(ultrasonicData.distance1) + " cm<br>";
     }
     else if (ultrasonicData.distance1 >= 5 && ultrasonicData.distance2 >= 5) {
        ultrasonicMessage1 = "<span class='safe'>CLEAR! </span><br>No one is near the Machine 1<br>";
     }

     String ultrasonicMessage2;

     if (ultrasonicData.distance2 <= 5 && ultrasonicData.distance2 != 0){
        ultrasonicMessage2 = "<span class='warning'>WARNING! </span><br>Someone is Approaching Machine 2!<br>Distance: " + String(ultrasonicData.distance2) + " cm<br>";
     }
     else if (ultrasonicData.distance2 >= 5) {
        ultrasonicMessage2 = "<span class='safe'>CLEAR! </span><br>No one is near the Machine 2<br>";
     }
     else {
        ultrasonicMessage2 = "No ultrasonic data received yet.";
      }
      client.print(ultrasonicMessage1);
      client.print(ultrasonicMessage2);
      Serial.println(ultrasonicMessage1);
      Serial.println(ultrasonicMessage2);
    }

    // Handle RFID Data Request
    else if (request.indexOf("GET /rfid") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();

      String RfidMessage1;
      String RfidMessage2;
      if (rfidData.peopleIn >= 1) {
        Serial.print("RFID Personnel Count: ");
        Serial.println(rfidData.peopleIn);

        RfidMessage1 = "Personnel Count: " + String(rfidData.peopleIn)+ "<br>";
        RfidMessage2 = "Logged: " + String(rfidData.scannedLabel)+ "<br>";
      } 
  else {
        RfidMessage1 = "No one is In\n";
      }


      client.println(RfidMessage1);
      client.println(RfidMessage2);
      Serial.print(RfidMessage1);
      Serial.print(RfidMessage2);
    }

    else if (request.indexOf("GET /lights") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();

      String lmMessage1;
      if (rfidData.peopleIn >= 1) {
        lmMessage1 = "On Operation\n";
      } 
  else {
        lmMessage1 = "Off Operation\n";
      }

      client.println(lmMessage1);
      Serial.print(lmMessage1);
      
    }

    // Serve Default HTML Page
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.print(index_html);
      Serial.println("HTML page served.");
    }
    client.stop();
  }
  delay(100);  // To avoid server overload
}
