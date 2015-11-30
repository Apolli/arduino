#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <stdlib.h>
#include <Adafruit_NeoPixel.h>

String maxBrigthness = "50";
String midBrigthness = "10";
int currentColor;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, 9, NEO_GRB + NEO_KHZ800);

// define ip
#define SERVER_ADDRESS 50

// init radioHead
RH_NRF24 driver;
RHReliableDatagram manager(driver, SERVER_ADDRESS);
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

// init display
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// init tank variables
float tankHeight = 1.50;
float tankWidth = 1.25;
float tankLength = 1.84;
float tankOffset = 0.40;

// init battery icon
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

// init timer for display updates
unsigned long timer;

// init message queue for json strings
char m1[28] = {};
char m3[28] = {};
char m6[28] = {};
char currentMessage[28] = {};
int currentContent = 0;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() 
{
  Serial.begin(9600);
  
  if (!manager.init()) {
    Serial.println("init failed");
  }
  
  // init display
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("Ready");

  strip.begin();
  strip.show();
}

void loop()
{
  // check for new messages
  if (manager.available())
  {
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)) {
      setMq(from, (char*)buf);
    }
  }

  // update display content and rotate messages
  checkContent();
  


  switch(currentColor) {
    case 1:
    case 2:
    case 4:
    case 5:
    case 7:
    case 8:
    case 10:
    case 11:
    case 13:
    case 14:
    case 16:
    case 17:
    case 19:
      delay(3000);
      break;
    case 0:
    default:
      tinkerSetColour(maxBrigthness+",0,0"); // red
      break;
    case 3:
      tinkerSetColour(maxBrigthness+","+maxBrigthness+",0"); // yellow
      break;
    case 6:
      tinkerSetColour("0,"+maxBrigthness+",0");// green
      break;
    case 9:
      tinkerSetColour("0,"+maxBrigthness+","+maxBrigthness); // türkis
      break;
    case 12:
      tinkerSetColour("0,0,"+maxBrigthness+""); // blue
      break;
    case 15:
      tinkerSetColour("50,50,200"); // lila
      break;
    case 18:
      tinkerSetColour(maxBrigthness+",0,"+maxBrigthness); // pink
      break;
    case 20:
      delay(3000);
      currentColor = 0;
      break;
  }

  currentColor++;
  

  serialEvent(); //call the function
  if (stringComplete) {
    int openBrace = inputString.indexOf('[');
    String jsonString = inputString.substring(openBrace);
    int from = inputString.toInt();
    
    char json[28];
    jsonString.toCharArray(json, 28);
    setMq(from, (char*)json);

    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    inputString += inChar;
    if (inChar == ']') {
      stringComplete = true;
    }
  }
}

void setMq(int from, char* json)
{
  Serial.println("");
  Serial.print("Set message from ip: "+String(from)+" json: ");
  Serial.println(json);

  switch(from) {
    case 1: strcpy(m1, json); break;
    case 3: strcpy(m3, json); break;
    case 6: strcpy(m6, json); break;
  }

  Serial.println("");
  Serial.println("");
  Serial.print("Ip: 1 json: ");
  Serial.println(m1);
  Serial.print("Ip: 3 json: ");
  Serial.println(m3);
  Serial.print("Ip: 6 json: ");
  Serial.println(m6);
  Serial.println("");
  Serial.println("");
}

void setCurrentMessage(int from)
{
  switch(from) {
    case 1: strcpy(currentMessage, m1); break;
    case 3: strcpy(currentMessage, m3); break;
    case 6: strcpy(currentMessage, m6); break;
  }
}


int getNextActive()
{
  // get next active
  switch(currentContent) {
    case 1: 
      if(m3[0] == '[') {
        return 3; 
      }
      if(m6[0] == '[') {
        return 6; 
      }
      break;
    case 3: 
      if(m6[0] == '[') {
        return 6;
      }
      if(m1[0] == '[') {
        return 1;
      }
      break;

    case 6: 
    default:
      if(m1[0] == '[') {
        return 1; 
      }
      if(m3[0] == '[') {
        return 3; 
      }
      break;
  }

  if(m1[0] == '[') {
    return 1; 
  }
  if(m3[0] == '[') {
    return 3; 
  }

  if(m6[0] == '[') {
    return 6; 
  }

  return 0;
}


void checkContent()
{ 
  // rotate every 5 seconds
  if(((millis()-timer) / 1000) > 5) {
    int from = getNextActive();

    // update timer
    timer = millis();
    
    // update current selected
    currentContent = from;

    switch(from) {
      case 3: // water 
        updateWaterLevel();
        break;

      case 1: // room
        updateTemperature();
        break;

      case 6: // outdoor
        updateTemperatureSoil();
        break;
    }
  } 
}

void updateWaterLevel()
{
  setCurrentMessage(1);

  StaticJsonBuffer<256> jsonBuffer;
  JsonArray& array = jsonBuffer.parseArray(currentMessage);

  String message;
  int lvl = array[0];
  float realLvl = getLvl(lvl);

  lcd.clear();

  message = "Prozent: "+getPercent(realLvl)+"%";
  lcd.setCursor(0, 0);
  lcd.print(message);

  message = "Liter: "+getLiter(realLvl);
  lcd.setCursor(0, 1);
  lcd.print(message);

  message = "Wasserstand: "+getHeight(realLvl)+"cm";
  lcd.setCursor(0, 2);
  lcd.print(message);
}

void updateTemperature()
{
  setCurrentMessage(1);

  StaticJsonBuffer<256> jsonBuffer;
  JsonArray& array = jsonBuffer.parseArray(currentMessage);

  String message;

  char temp[10];
  char hum[10];
  dtostrf(array[0], 1, 1, temp);
  dtostrf(array[1], 1, 1, hum);
  int battery = array[2];

  lcd.clear();

  lcd.createChar(0, bat);
  message = "Temp. innen      "+String(battery)+"%";
  lcd.setCursor(0, 0);
  lcd.print(message);
  lcd.setCursor(16, 0);
  lcd.write((byte)0);

  message = "Luft: ";
  message += String(temp);
  message += String((char)223)+"C   ";
  message += String(hum)+"%";
  lcd.setCursor(0, 1);
  lcd.print(message);
}

void updateTemperatureSoil()
{
  setCurrentMessage(6);

  StaticJsonBuffer<256> jsonBuffer;
  JsonArray& array = jsonBuffer.parseArray(currentMessage);

  String message;

  char temp[10];
  char hum[10];
  
  dtostrf(array[0], 1, 1, temp);
  dtostrf(array[1], 1, 1, hum);
  int battery = array[2];
  int soil = array[3];

  lcd.clear();

  lcd.createChar(0, bat);
  message = "Temp. aussen     "+String(battery)+"%";
  lcd.setCursor(0, 0);
  lcd.print(message);
  lcd.setCursor(16, 0);
  lcd.write((byte)0);
  
  message = "Luft: ";
  message += String(temp);
  message += String((char)223)+"C   ";
  message += String(hum)+"%";
  lcd.setCursor(0, 1);
  lcd.print(message);
  
  message = "Erdfeucht.: "+String(soil)+"%";
  lcd.setCursor(0, 2);
  lcd.print(message);
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


int Rstart = 0, Gstart = 0, Bstart = 0;
int Rnew = 0, Gnew = 0, Bnew = 0;
int tinkerSetColour(String command)
{
    // Clear strip.
    strip.show();

    int commaIndex = command.indexOf(',');
    int secondCommaIndex = command.indexOf(',', commaIndex+1);
    int lastCommaIndex = command.lastIndexOf(',');

    int red = command.substring(0, commaIndex).toInt();
    int grn = command.substring(commaIndex+1, secondCommaIndex).toInt();
    int blu = command.substring(lastCommaIndex+1).toInt();

    int Rend = red, Gend = grn, Bend = blu;

    // Larger values of 'n' will give a smoother/slower transition.
    int n = 100;
    for (int i = 0; i < n; i++)
    {
        Rnew = Rstart + (Rend - Rstart) * i / n;
        Gnew = Gstart + (Gend - Gstart) * i / n;
        Bnew = Bstart + (Bend - Bstart) * i / n;
        
        // Set pixel color here.
        strip.setPixelColor(0, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(1, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(2, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(3, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(4, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(5, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(6, strip.Color(Rnew, Gnew, Bnew));
        strip.setPixelColor(7, strip.Color(Rnew, Gnew, Bnew));
        strip.show();
        delay(10);
    }
    
    Rstart = red, Gstart = grn, Bstart = blu;
    
    return 1;
}