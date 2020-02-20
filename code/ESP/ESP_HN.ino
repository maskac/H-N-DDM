#include <ESP8266WiFi.h>
#include <WiFiClient.h>

const char* ssid = "*";
const char* password = "*";

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print("w");
  }

  Serial.println("\nRTS");
}

void loop() {
  String dataFromATTINY = Serial.readString();
  if (dataFromATTINY.length() == 0) return; // Ignore timeouts if ATTINY didnt send data yet

  if (checkData(dataFromATTINY)) {
    Serial.println("OK");
  } else {
    Serial.println("RN");
    return; // Read data again
  }

  Serial.print(dataFromATTINY);
}

bool checkData(String data) {
  if (data == "OKDATA\n") return true;
  return false;
}
