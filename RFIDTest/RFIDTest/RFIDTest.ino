#define TXD1 19
#define RXD1 21
HardwareSerial mySerial(2);

void RFIDUARTSetup() {
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup
}

String readRFID() {
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    return message;
  }
  return "";  // Return empty string if no data is available
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
RFIDUARTSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  String message=readRFID();
  if(message.length()>0) {
  Serial.println(message);}
  delay(100);

}
