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
#include "HX711.h"
#define echoPin 2  // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define trigPin 4  // CHANGE PIN NUMBER HERE IF YOU WANT TO USE A DIFFERENT PIN
#define TXD1 19
#define RXD1 21
#define SCK 25
#define DT1 33
#define DT2 32
#define DT3 34
#define DT4 35
#define SCALE_FACTOR_1 427.772  // calibration factors reading over known mass
#define SCALE_FACTOR_2 422.633
#define SCALE_FACTOR_3 428.811
#define SCALE_FACTOR_4 445.3465
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;


long duration, distance;
volatile bool flagCapture;
float ema;  //EMA Weight sample data
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = { 0xEC, 0xE3, 0x34, 0xD6, 0x88, 0x9C };


// Use Serial1 for UART communication
HardwareSerial mySerial(2);
//EC:E3:34:C0:A3:78

// Insert your SSID
constexpr char WIFI_SSID[] = "PHILA";

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
  //WiFi.printDiag(Serial);  // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  // WiFi.printDiag(Serial);  // Uncomment to verify channel change after

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

void ScaleSetup() {
  scale1.begin(DT1, SCK);
  scale2.begin(DT2, SCK);
  scale3.begin(DT3, SCK);
  scale4.begin(DT4, SCK);

  while (!scale1.is_ready() || !scale2.is_ready() || !scale3.is_ready() || !scale4.is_ready()) {
    Serial.println("Waiting for HX711 modules...");
    delay(500);
  }

  scale1.tare();
  scale2.tare();
  scale3.tare();
  scale4.tare();

  scale1.set_scale(SCALE_FACTOR_1);  // reading over known mass
  scale2.set_scale(SCALE_FACTOR_2);
  scale3.set_scale(SCALE_FACTOR_3);
  scale4.set_scale(SCALE_FACTOR_4);


  Serial.println("All scales tared and calibrated. Ready for measurements in grams.");
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


void getWeight() {
  ema = 0.04 * (scale1.get_units(3) + scale2.get_units(3) + scale3.get_units(3) + scale4.get_units(3)) + (1 - 0.04) * ema;
}

float getFirstWeight() {
  return scale1.get_units(3) + scale2.get_units(3) + scale3.get_units(3) + scale4.get_units(3);
}

float getFinalWeight() {
  float AverageWeight = 0;
  uint8_t Weightsamples = 0;
  //ema = getFirstWeight();
  while (Weightsamples <= 26) {
    delay(10);
    //getWeight();
    String RFIDReading = readRFID();

    RFIDReading.trim();
    if (RFIDReading.length() >= 8) {
      strcpy(myData.RFID, RFIDReading.c_str());
    }
    // AverageWeight = ema + AverageWeight;
    AverageWeight = getFirstWeight() + AverageWeight;
    Weightsamples += 1;
    // if (ema != 0) {

    // } else {
    //   break;
    // }
  }

  AverageWeight = AverageWeight / Weightsamples;

  return AverageWeight;
}


void SendSignalCaptureImage() {
  strcpy(myData.RFID, "IMAGECAPTURE");
  myData.Weight = 0;
  SendDataToCamera();
}

void setup() {
  Serial.begin(115200);
  WifiESPNOWSetup();
  RFIDUARTSetup();
  DistanceSensorSetup();
  ScaleSetup();
  flagCapture = true;
}

void loop() {
  // Check if something is within 30 cm and flag is true
  long distance = getDistance();
  if (distance <= 30 && flagCapture == true) {
    Serial.println("Distance: " + String(distance));
    Serial.println("Sending Signal to Camera");
    flagCapture = false;
    SendSignalCaptureImage();
  }
  // Reset data
  strcpy(myData.RFID, "");
  myData.Weight = 0;
  unsigned long startTime = millis();  // Start timer
  if (!flagCapture) {
    while (1) {
      // Break after 120 seconds
      if (millis() - startTime > 30000) {
        Serial.println("Timeout reached: exiting loop.");
        break;
      }

      // Try reading RFID
      String RFIDReading = readRFID();

      RFIDReading.trim();
      if (RFIDReading.length() >= 8) {
        Serial.println("RFID: " + RFIDReading);
        strcpy(myData.RFID, RFIDReading.c_str());
      }

      // Try reading weight
      float finalWeight = getFinalWeight();
      if (finalWeight > 100) {
        Serial.println("Weight: " + String(finalWeight));
        myData.Weight = (finalWeight / 1000);
        break;  // Exit the loop early once weight is received
      }

      //delay(50);  // Small delay to avoid hammering sensors
    }
  }


  // Send data (RFID + Weight) to camera
  if ((flagCapture == false) && myData.Weight == 0) {
    strcpy(myData.RFID, "IMAGEONLY");  // Commented for testing only
    SendDataToCamera();
  }

  if ((flagCapture == false) && myData.Weight > 0) {
    SendDataToCamera();
  }



  delay(100);  // Optional: give ESP some breathing time
  while (distance <= 30) {
    distance = getDistance();
    if (distance > 30) {
      flagCapture = true;
    }
    delay(100);
  }
}
