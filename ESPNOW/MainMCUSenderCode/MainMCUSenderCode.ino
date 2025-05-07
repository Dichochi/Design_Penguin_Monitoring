/*
  ESP-NOW Demo - Transmit
  esp-now-demo-xmit.ino
  Sends data to Responder
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include <esp_now.h>
#include <WiFi.h>

// Variables for test data
int int_value;
float float_value;
bool bool_value = true;
bool flag=true;
// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0xEC, 0xE3, 0x34, 0xC0, 0xA3, 0x78};
//EC:E3:34:C0:A3:78

// Define a data structure
typedef struct struct_message {
  char RFID[32];
  float Weight;
} struct_message;

// Create a structured object
struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  
  // Set up Serial Monitor
  Serial.begin(115200);
 
  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {

  // Create test data

  // Generate a random integer
  

  // Use integer to make a new float
  float_value = 32;

  // Invert the boolean value
  
  // Format structured data
  strcpy(myData.RFID, "ID1285");
  myData.Weight = float_value;

  int error=0;
  // Send message via ESP-NOW
   if (flag) {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sending confirmed");
    flag=false;
  }
  else {
    if(error>1){
     Serial.println("Sending error");
     flag=false;  
     error=0;   
    }
    else{
      delay(8000);
      error+=1;
    }
  }
   }
  delay(2000);
}