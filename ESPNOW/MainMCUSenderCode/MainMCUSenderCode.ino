/*
  ESP-NOW Demo - Transmit
  esp-now-demo-xmit.ino
  Sends data to Responder
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Wire.h>
#define echoPin 2  // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define trigPin 4  // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define TXD1 19
#define RXD1 21
long duration, distance;
volatile bool flagCapture;
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = { 0xEC, 0xE3, 0x34, 0xD6, 0x88, 0x9C };


// Use Serial1 for UART communication
HardwareSerial mySerial(2);
//EC:E3:34:C0:A3:78

// Insert your SSID
constexpr char WIFI_SSID[] = "MULWELI";

// Define a data structure
typedef struct struct_message {
  char RFID[32];
  float Weight;
} struct_message;

// Create a structured object
struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

bool isAlphaNumericOnly(String input) {
  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (!isAlphaNumeric(c)) {
      return false;
    }
  }
  return true;
}

void WifiESPNOWSetup() {

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial);  // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial);  // Uncomment to verify channel change after

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}
void RFIDUARTSetup() {
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup
}

void DistanceSensorSetup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}


// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void SendDataToCamera() {

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
}

String readRFID() {
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    return message;
  }
  return "";  // Return empty string if no data is available
}

long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58.2;
  return distance;
}


float getWeight() {
  return 0;
}
void SendSignalCaptureImage() {
  strcpy(myData.RFID, "IMAGE");
  myData.Weight = 0;
  SendDataToCamera();
}

void setup() {
  Serial.begin(115200);
  WifiESPNOWSetup();
  RFIDUARTSetup();
  DistanceSensorSetup();
  flagCapture = true;
  startMillis = millis();
}

void loop() {
  // Check if something is within 30 cm and flag is true
  long distance = getDistance();
  Serial.println("Distance: " + String(distance));

  if (distance <= 30 && flagCapture == true) {
    flagCapture = false;
    SendSignalCaptureImage();
  }

  // Reset flag when space is clear


  // Reset data
  strcpy(myData.RFID, "");
  myData.Weight = 0;

  unsigned long startTime = millis();  // Start timer
  if (!flagCapture) {
    while (1) {
      // Break after 120 seconds
      if (millis() - startTime > 10000) {
        Serial.println("Timeout reached: exiting loop.");
        break;
      }

      // Try reading RFID
      String RFIDReading = readRFID();

      RFIDReading.trim();
      if (RFIDReading.length() >= 8) {
        strcpy(myData.RFID, RFIDReading.c_str());
      }

      // Try reading weight
      float Weight = getWeight();
      if (Weight > 0) {
        myData.Weight = Weight;
        break;  // Exit the loop early once weight is received
      }

      delay(100);  // Small delay to avoid hammering sensors
    }
  }


  // Send data (RFID + Weight) to camera
  if ((flagCapture == false) && myData.Weight == 0) {
    //strcpy(myData.RFID, "IMAGEONLY"); // Commented for testing only
    SendDataToCamera();
  }

  if ((flagCapture == false) && myData.Weight > 0) {
    SendDataToCamera();
  }



  delay(1000);  // Optional: give ESP some breathing time
  if (distance <= 30) {
    distance= getDistance();
    if (distance > 30) {
      flagCapture = true;
      delay(40);
    }
  }
}
