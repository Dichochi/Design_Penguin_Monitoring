#include <WiFi.h>

const char* ssid = "PeakStudios";
const char* password = "";
void setup() {
pinMode(2,OUTPUT);
 Serial.begin(115200);
  delay(1000);

  Serial.println("Connecting to open Wi-Fi network...");
  WiFi.begin(ssid, password);          // Still needs two arguments

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // put your main code here, to run repeatedly:
digitalWrite(2,HIGH);
delay(1000);
digitalWrite(2,LOW);
delay(1000);
}
