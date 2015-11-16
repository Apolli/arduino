#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <stdlib.h>

#define SERVER_ADDRESS 50
// Singleton instance of the radio driver
RH_NRF24 driver;
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

float tankHeight = 1.50;
float tankWidth = 1.25;
float tankLength = 1.84;
float tankOffset = 0.40;

byte bat[8] = {
  0b00100,
  0b11011,
  0b10001,
  0b10001,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");

  lcd.begin(20, 4);

  lcd.setCursor(0, 0);
  lcd.print("Ready");
}

// Dont put this on the stack:
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

unsigned long timer;
char* messageQueue[200] = {};


void loop()
{

  
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
//      Serial.println((char*)buf);

      messageQueue[(int)from] = (char*)buf;

      Serial.println("");      
      Serial.println("");      
      Serial.println("update queue data:");
      for (int i = 0; i < 200; i++){
          if(messageQueue[i]) {
              Serial.println(messageQueue[i]);
          }
      }   
      Serial.println("");      
        
    }
  }
  
  
  
  checkContent();
  


/*
  if(((millis()-timer) / 1000) > 240) {
    String message;  
    message = String((millis()-timer) / 1000 )+" sek";
    message += " / Data error ";
    
    lcd.setCursor(0, 3);
    lcd.print(message);
  }*/
}

int currentContent;

int getNextAcive()
{
    // get next active
    if(currentContent) {
      for (int i = 0; i < 200; i++) {
          if(i > currentContent && messageQueue[i]) {
              return i;
          }
      }
    }
    
    // no active found... get first entry
    for (int i = 0; i < 200; i++) {
        if(messageQueue[i]) {
            return i;
        }
    }
    
}


void checkContent()
{
    StaticJsonBuffer<256> jsonBuffer;
    int lvl;
    String sensor;
    float realLvl;
    String message;
    int battery;
    char temp[10];
    char tmp[10];
    char hum[10];
    int soil;
    
    if(((millis()-timer) / 1000) > 5) {
        int from = getNextAcive();
        if(!messageQueue[from]) {
          return;
        }
        
//        Serial.println("next active:");
//        Serial.println(from);

        JsonArray& array = jsonBuffer.parseArray(messageQueue[(int)from]);
        currentContent = from;


        
        timer = millis();
  
        lcd.clear();
        
        switch(from) {
          case 3: // water 
            lvl = array[0];
            realLvl = getLvl(lvl);
            
            Serial.println(realLvl);
            
            message = "Prozent: "+getPercent(realLvl)+"%";
            lcd.setCursor(0, 0);
            lcd.print(message);
    
            message = "Liter: "+getLiter(realLvl);
            lcd.setCursor(0, 1);
            lcd.print(message);
    
            message = "Wasserstand: "+getHeight(realLvl)+"cm";
            lcd.setCursor(0, 2);
            lcd.print(message);
            break;
            
          case 1: // room
            battery = array[2];
            dtostrf(array[0], 1, 1, tmp);
            dtostrf(array[1], 1, 1, hum);
            
            lcd.createChar(0, bat);
            message = "Temp. innen      "+String(battery)+"%";
            lcd.setCursor(0, 0);
            lcd.print(message);
            lcd.setCursor(16, 0);
            lcd.write((byte)0);

            message = "Luft: ";
            message += String(tmp);
            message += String((char)223)+"C   ";
            message += String(hum)+"%";
            lcd.setCursor(0, 1);
            lcd.print(message);
            break;
          case 6: // outdoor
            
            battery = array[2];
            dtostrf(array[0], 1, 1, temp);
            dtostrf(array[1], 1, 1, hum);
            soil = array[3];
            
            lcd.createChar(0, bat);
            message = "Temp. aussen     "+String(battery)+"%";
            lcd.setCursor(0, 0);
            lcd.print(message);
            lcd.setCursor(16, 0);
            lcd.write((byte)0);
            
            message = "Luft: ";
            message += String(tmp);
            message += String((char)223)+"C   ";
            message += String(hum)+"%";
            lcd.setCursor(0, 1);
            lcd.print(message);
            
            message = "Erdfeuchte: "+String(soil)+"%";
            lcd.setCursor(0, 2);
            lcd.print(message);
            break;
        }
    }
}


// helper methods

float getLvl(float lvl) {
  float waterLevel;  
  lvl = lvl/100;

  if(lvl > tankHeight) {
    waterLevel = 0;
  }
  else {
    waterLevel = tankHeight - lvl;
  }

  Serial.println("get waterlevel");  
  Serial.println(waterLevel);

  return waterLevel;
}

String getLiter(float realLvl) {
  int l = (realLvl * tankWidth * tankLength) * 1000;

  Serial.println("get liter");  
  Serial.println(realLvl);  
  return String(l);
}


String getPercent(float realLvl) {
  int p = 100 / (tankHeight - tankOffset) * realLvl;
  return String(p);
}

String getHeight(float realLvl) {
  int h = realLvl * 100;
  return String(h);
}
