#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <Wire.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>

#define RST_PIN 0 // ESP32
// #define RST_PIN 14 // D5 on NodeMCU

// 0x28 is i2c address of the NFC Reader. Check your address with i2cscanner if not match.
MFRC522_I2C mfrc522(0x28, RST_PIN);   // Create MFRC522 instance.


const char* ssid = "xxxxx";
const char* password = "xxxxxx";
const char* awsEndpoint = "xxxxx.amazonaws.com";
const char* deviceID = "xxxxx";

const char* certificate = "xxxxxxxx";

const char* privateKey = "xxxxxxx";

const char* rootCA = "xxxxxxxxx";

WiFiClientSecure espClient = WiFiClientSecure();
PubSubClient client(espClient);

void setup() {
  M5.begin();
  Wire.begin();
  Serial.begin(115200);
  M5.Lcd.setCursor(140, 0, 4);
  M5.Lcd.println("RFID");
  mfrc522.PCD_Init(); // Init MFRC522
  ShowReaderDetails(); // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  M5.Lcd.setCursor(0,30,2);
  M5.Lcd.println("Scan PICC to see UID, type, and data blocks...");
  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
  delay(100);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected");

  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);

  client.setServer(awsEndpoint, 8883);
  client.setCallback(callback);

  while (!client.connected()) {
    if (client.connect(deviceID)) {
      Serial.println("Connected to AWS IoT Core");
      client.subscribe("esp32/trial");
    } else {
      Serial.print("Failed to connect to AWS, rc=");
      Serial.println(client.state());
      delay(1000);
    }
  }

  // Publish a message after connecting to AWS IoT Core
  //client.publish("esp32/trial", "Hello from ESP32!");
}

void loop() {
   client.loop();

   // Look for new cards, and select one if present
   if (!mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial()) {
   delay(50);
   return;
   }

  String cardUID = ""; // Variable to store card UID

    // Concatenate UID bytes into cardUID variable
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardUID += (mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }

  publishMessage(cardUID);
  delay(5000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void publishMessage(String UID)
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["Card UID"] = UID;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish("esp32/trial", jsonBuffer);
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}