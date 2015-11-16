#include <ArduinoJson.h>
#include <Vcc.h>
#include <dht.h>
#include <stdio.h>
#include <LowPower.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>


#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 100
#define DISPLAY_ADDRESS 50

const float VccMin        = 1.8;  // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccMax        = 4.0*1.2;  // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccCorrection = 1.0/1.0;  // Measured Vcc by multimeter divided by reported Vcc
Vcc vcc(VccCorrection);

dht DHT;

#define DHT22_PIN 5

struct
{
  uint32_t total;
  uint32_t ok;
  uint32_t crc_error;
  uint32_t time_out;
  uint32_t connect;
  uint32_t ack_l;
  uint32_t ack_h;
  uint32_t unknown;
} 
stat = { 
  0,0,0,0,0,0,0,0};

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

void loop()
{
  int chk = DHT.read22(DHT22_PIN);

  if(chk == DHTLIB_OK) {
    
    StaticJsonBuffer<200> jsonBuffer;

    JsonArray& array = jsonBuffer.createArray();
    array.add((double)DHT.temperature);
    array.add((double)DHT.humidity);
    array.add((int)vcc.Read_Perc(VccMin, VccMax));
    
    char data[RH_NRF24_MAX_MESSAGE_LEN];
    array.printTo(data, sizeof(data));
    Serial.println(data);
      
    // Send a message to manager_server
    Serial.println("Send to SERVER_ADDRESS");
    manager.sendtoWait((uint8_t*)data, sizeof(data), SERVER_ADDRESS);
  
    Serial.println("Send to DISPLAY_ADDRESS");
    manager.sendtoWait((uint8_t*)data, sizeof(data), DISPLAY_ADDRESS);
    
  }
  else {
    Serial.println("temp meassure failed");
  }


  digitalWrite(8, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  
  sleep(60);
}

String double2string(double ambientTemp) {
  char TempString[10];

  dtostrf(ambientTemp,2,2,TempString);
  return String(TempString);
}


void sleep(int time) {
  Serial.println("sleep...");
  int intervals;
  intervals = time/4;
  delay(1000);
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  for (int sleepInterval = 0; sleepInterval < intervals; sleepInterval++) { 
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); 
  }
}
