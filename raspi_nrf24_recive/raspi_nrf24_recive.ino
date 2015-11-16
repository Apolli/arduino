#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <stdlib.h>

#define SERVER_ADDRESS 100
// Singleton instance of the radio driver
RH_NRF24 driver;
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);


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
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      Serial.write((char*)buf);
      //Serial.println((char*)buf);
    }
  }
}
