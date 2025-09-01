#include <WiFi.h>
#include <esp_now.h>

#define ESPNOW_WIFI_CHANNEL 6
#define TRIG_PIN_1 5
#define ECHO_PIN_1 18
#define TRIG_PIN_2 19
#define ECHO_PIN_2 21
#define BUZZER_PIN_1 15  // Buzzer for Sensor 1
#define BUZZER_PIN_2 17  // Buzzer for Sensor 2
#define SAMPLING_RATE 5000

typedef struct {
  int id;
  int objectDetected1;
  float distance1;
  int objectDetected2;
  float distance2;
  unsigned long detectionCount1;
  unsigned long detectionCount2;
} DistanceData;

DistanceData distanceData;

uint8_t receiverMacAddress[] = {0xD0, 0xEF, 0x76, 0x5B, 0x99, 0xE0};

unsigned long detectionCount1 = 0;
unsigned long detectionCount2 = 0;

float readUltrasonicSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);
  if (duration == 0) return -1; // Out of range
  return (duration * 0.0343) / 2.0;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void sendDistanceData() {
  float distanceValue1 = readUltrasonicSensor(TRIG_PIN_1, ECHO_PIN_1);
  float distanceValue2 = readUltrasonicSensor(TRIG_PIN_2, ECHO_PIN_2);

  // Determine if objects are detected (distance <= 5 cm for buzzer)
  distanceData.objectDetected1 = (distanceValue1 > 0 && distanceValue1 <= 5) ? 1 : 0;
  distanceData.objectDetected2 = (distanceValue2 > 0 && distanceValue2 <= 5) ? 1 : 0;

  // Update detection counts
  if (distanceData.objectDetected1) detectionCount1++;
  if (distanceData.objectDetected2) detectionCount2++;

  // Update distance data
  distanceData.id = 2;
  distanceData.distance1 = distanceValue1;
  distanceData.distance2 = distanceValue2;
  distanceData.detectionCount1 = detectionCount1;
  distanceData.detectionCount2 = detectionCount2;

  // Send data over ESP-NOW
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *)&distanceData, sizeof(DistanceData));
  Serial.printf("Data Sent: ID=%d, Distance1=%.2f cm, Distance2=%.2f cm\n", 
                distanceData.id, distanceData.distance1, distanceData.distance2);

  // Activate buzzer for Sensor 1 if distance <= 5 cm
  if (distanceData.objectDetected1) {
    digitalWrite(BUZZER_PIN_1, HIGH); // Turn on buzzer for Sensor 1
  } else {
    digitalWrite(BUZZER_PIN_1, LOW);  // Turn off buzzer for Sensor 1
  }

  // Activate buzzer for Sensor 2 if distance <= 5 cm
  if (distanceData.objectDetected2) {
    digitalWrite(BUZZER_PIN_2, HIGH); // Turn on buzzer for Sensor 2
  } else {
    digitalWrite(BUZZER_PIN_2, LOW);  // Turn off buzzer for Sensor 2
  }
}

void setup() {
  Serial.begin(115200);

  // Set up the trigger and echo pins for ultrasonic sensors
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Set up the buzzer pins
  pinMode(BUZZER_PIN_1, OUTPUT);
  pinMode(BUZZER_PIN_2, OUTPUT);

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  // Register callback function for sending data status
  esp_now_register_send_cb(OnDataSent);

  // Register the peer (receiver MAC address)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = ESPNOW_WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add receiver peer");
  }
}

void loop() {
  static unsigned long lastSampleTime = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastSampleTime >= SAMPLING_RATE) {
    lastSampleTime = currentMillis;
    sendDistanceData();
  }
}
