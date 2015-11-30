#include <ArduinoJson.h>
#include <NewPing.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

// prepape sonar lib
#define TRIGGER_PIN  2 // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// define ips
#define CLIENT_ADDRESS 3
#define SERVER_ADDRESS 100
#define DISPLAY_ADDRESS 50

// init radiohead
RH_NRF24 driver;
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init()) {
    Serial.println("init failed");
  }
}

void loop()
{
  // load distance
  int cm = 0;
  for(int i=0; i<5; i++) {
    cm += sonar.ping_cm();
    delay(200);
  }

  Serial.println(cm/5);

  // prepare json
  StaticJsonBuffer<200> jsonBuffer;

  JsonArray& array = jsonBuffer.createArray();
  array.add((int)(cm/5));
  
  char data[RH_NRF24_MAX_MESSAGE_LEN];
  array.printTo(data, sizeof(data));
  Serial.println(data);

  // Send a message to manager_server
  Serial.println("Send to SERVER_ADDRESS");
  manager.sendtoWait((uint8_t*)data, sizeof(data), SERVER_ADDRESS);

  Serial.println("Send to DISPLAY_ADDRESS");
  manager.sendtoWait((uint8_t*)data, sizeof(data), DISPLAY_ADDRESS);
  
  delay(10000);
}

