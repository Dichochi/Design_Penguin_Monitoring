/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-save-microsd-card
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>  // read and write from flash memory
#include <esp_now.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <SPIFFS.h>  // Or use SD.h if you're reading from SD card

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

int pictureNumber = 0;
volatile bool dataReceived = false;
volatile bool TakeImage = false;
 String ImagePath;
// Define a data structure
typedef struct struct_message {
  char RFID[32];
  float Weight;
} struct_message;

// Create a structured object
struct_message myData;

const char* ssid = "MULWELI";
const char* password = "ESPTEST1";
unsigned long epochTime;

void TransferToServer() {
  const char* serverUrl = "http://192.168.137.163:8000/add/";
  //const char* host = "192.168.137.52";
  const int httpPort = 8000;
  //--------------------------------------------------
  epochTime = getTime();
  //SD_MMC.begin(true);  // Or SD.begin()
  HTTPClient http;

  String boundary = "----ESP32FormBoundary";
  //String jsonPart = "{\"name\":\"ESP32\",\"status\":\"OK\"}";

  String ID = (strcmp(myData.RFID, "") == 0)? "null":myData.RFID;

  String jsonPart = "{\"timestamp\": " + String(epochTime) + ", \"id\": " +  ID+
  ", \"weight\": " + String(myData.Weight) + "}";


  // Begin request
  http.begin(serverUrl);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  // Start building multipart body
  String bodyStart = "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"data\"\r\n";
  bodyStart += "Content-Type: application/json\r\n\r\n";
  bodyStart += jsonPart + "\r\n";

  // Prepare image part
  String imageHeader = "--" + boundary + "\r\n";
  imageHeader += "Content-Disposition: form-data; name=\"picture\"; filename="+ImagePath +"\r\n";
  imageHeader += "Content-Type: image/jpeg\r\n\r\n";

  String bodyEnd = "\r\n--" + boundary + "--\r\n";

  // Open image file (replace with camera buffer if needed)
 // File imageFile = SPIFFS.open("/photo.jpg", "r");  // Or SD.open()
 Serial.println(ImagePath);
 SD_MMC.begin();
  fs::FS& fs = SD_MMC;
  File imageFile = fs.open(ImagePath.c_str(), FILE_READ);

if (!imageFile || imageFile.isDirectory()) {
  Serial.println("Failed to open image: " + ImagePath);
} else {
  Serial.println("Opened image: " + ImagePath);
   //Now imageFile can be used (e.g., streamed for HTTP POST)
}

  if (!imageFile) {
    Serial.println("Failed to open image file");
    return;
  }
  int httpResponseCode = http.POST(bodyStart+imageHeader+bodyEnd);
imageFile.close();
  
}
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}
void captureAndSaveImage() {
  camera_fb_t* fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;


  epochTime = getTime();
  ImagePath = "/p" + String(epochTime) + ".jpg";
  SD_MMC.begin();
  fs::FS& fs = SD_MMC;

  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  Serial.printf("Picture file name: %s\n", ImagePath.c_str());

  File file = fs.open(ImagePath.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);  // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", ImagePath.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);

  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(50);             // Flash on
  digitalWrite(4, LOW);  // Flash off
}
void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Data received: ");
  Serial.println(len);
  Serial.print("RFID Value: ");
  Serial.println(myData.RFID);
  Serial.print("Weight Value: ");
  Serial.println(myData.Weight);
  Serial.println();
  if (strcmp(myData.RFID, "IMAGE") == 0) {
    Serial.println("Image Capture Triggered");
    TakeImage = true;

  } else {
    dataReceived = true;
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector

  Serial.begin(9600);

  //Serial.setDebugOutput(true);
  //Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //Serial.println("Starting SD Card");
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);

  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  configTime(0, 0, "pool.ntp.org");
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb((OnDataRecv));
}


void loop() {
  if (TakeImage) {
    TakeImage = false;
    captureAndSaveImage();
  }

  if (dataReceived) {
    dataReceived = false;  // clear the flag
    TransferToServer();
  }
}