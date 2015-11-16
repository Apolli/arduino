#include <ArduinoJson.h>
#include <NewPing.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

#define TRIGGER_PIN  2 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

#define CLIENT_ADDRESS 3
#define SERVER_ADDRESS 100
#define DISPLAY_ADDRESS 50

// Singleton instance of the radio driver
RH_NRF24 driver;
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);
void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
}

// Dont put this on the stack:
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
void loop()
{
  int cm = 0;
  for(int i=0; i<5; i++) {
    cm += sonar.ping_cm();
    delay(200);
  }

  Serial.println(cm/5);

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& array = jsonBuffer.createObject();
  array["s"] = "water";
  array["lvl"] = (cm/5);

  char data[RH_NRF24_MAX_MESSAGE_LEN];
  array.printTo(data, sizeof(data));
  Serial.println(data);

  // Send a message to manager_server
  Serial.println("Send to SERVER_ADDRESS");
  manager.sendtoWait((uint8_t*)data, sizeof(data), SERVER_ADDRESS);

  Serial.println("Send to DISPLAY_ADDRESS");
  manager.sendtoWait((uint8_t*)data, sizeof(data), DISPLAY_ADDRESS);
  delay(2000);
}

