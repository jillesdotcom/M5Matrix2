// Requires:
// https://github.com/gutierrezps/ESP32_I2C_Slave
// https://github.com/FastLED/FastLED

#include <Arduino.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WireSlave.h>
#include "WiFi.h"

//#define DEBUG
#define SDA_PIN           21
#define SCL_PIN           25
#define ATOM_DEFAULT_ID  127 // initial setup adres.
#define ATOM_FIRST         1
#define ATOM_LAST         16
#define PIXEL_COUNT       25
#define PIXEL_PIN         27
#define MAX_BRIGHTNESS     2
#define BUTTON_PIN        39
#define BUTTON_CLEAR       0
#define BUTTON_SHORT       1
#define BUTTON_LONG        2
#define SHORT_PRESS      500 // ms

static CRGB leds[PIXEL_COUNT];

char PixelHex[17][26] =
{"0111010001100011000101110",  // 0
 "0010000100001000010000100",  // 1
 "1111000001011101000011111",  // 2
 "1111000001011100000111110",  // 3
 "1000110001111110000100001",  // 4
 "1111110000111100000111110",  // 5
 "0111110000111101000101110",  // 6
 "0111100001000010000100001",  // 7
 "0111010001011101000101110",  // 8
 "0111010001011110000111110",  // 9
 "0111010001111111000110001",  // A
 "1111010001111101000111110",  // B
 "0111110000100001000001111",  // C
 "1111010001100011000111110",  // D
 "1111110000111101000011111",  // E
 "1111110000111101000010000",  // F
 "0000000000000000000000000"}; // clear
 
char PixelDec[18][26] =
{"0011100101001010010100111", //  0
 "0001000010000100001000010", //  1
 "0011100001001110010000111", //  2
 "0011100001001110000100111", //  3
 "0010100101001110000100001", //  4
 "0011100100001110000100111", //  5
 "0011100100001110010100111", //  6
 "0011100001000010000100001", //  7
 "0011100101001110010100111", //  8
 "0011100101001110000100111", //  9
 "1011110101101011010110111", // 10
 "1001010010100101001010010", // 11
 "1011110001101111010010111", // 12
 "1011110001101111000110111", // 13
 "1010110101101111000110001", // 14
 "1011110100101111000110111", // 15
 "1011110100101111010110111", // 16
 "0000000000000000000000000"}; // clear


int I2C_ADDRESS = ATOM_DEFAULT_ID;
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
int buttonPressed = BUTTON_CLEAR;

int picture[75];

void GetAtomID() {
  EEPROM.begin(1);
  byte EEPROMID  = EEPROM.read(0);
 
  if ( (EEPROMID >= ATOM_FIRST) && (EEPROMID <= ATOM_LAST) ) {
    I2C_ADDRESS = EEPROMID;
    Serial.printf("ATOM I2C address set to: %d\n",EEPROMID);
  }
}

void SetAtomID(byte id) { 
  if ( (id >= ATOM_FIRST) && (id <= ATOM_LAST) ) {
    EEPROM.write(0, id);
    EEPROM.commit();
    I2C_ADDRESS = id;
    Serial.printf("ATOM I2C address set to: %d\n",id);
    // ESP.restart();      //Reboot ESP, need to reset the I2C bus.
  }
}

void ShowPicture() {
  Serial.println("PICTURE: ");
  for(int x=0; x<75; x++) {
    Serial.printf("Index: %d %d\n",x,picture[x]);
  }
}

void ShowAtomID() {
  if(I2C_ADDRESS>=ATOM_FIRST && I2C_ADDRESS<=ATOM_LAST) { 
	  for(int pixel=0;pixel<PIXEL_COUNT;pixel++) {
	    if( PixelDec[I2C_ADDRESS][pixel] == '1' ) {
		    leds[pixel] = CRGB::Red;
	    } else {
		    leds[pixel] = CRGB::Black;
	    }
	  }
    FastLED.show();   
  }
}

void CountAP(int count) {
  for(int pixel=0;pixel<PIXEL_COUNT;pixel++) {
    if( pixel<count ) {
      leds[pixel] = CRGB::Red;
    } else {
      leds[pixel] = CRGB::Black;
    }
  }
  FastLED.show();   
}

void requestEvent() {
  // function that executes whenever data is requested by master
  // this function is registered as an event, see setup()

  bool buttonState = digitalRead(BUTTON_PIN);
  
  Serial.println(buttonState);
  WireSlave.write(buttonState); 

  if (buttonState==false) {
    WireSlave.write(0); 
  } else {
    WireSlave.write(1); 
  }
  
  if (I2C_ADDRESS == 17) {
    WireSlave.print("A ");    //Request for new I2C Adres when adres was 17
  } else {
    WireSlave.print("P ");
  }
}

void receiveEvent(int howMany) {
  int x;
  char c;
    while (1 < WireSlave.available()) { //Loop through all but the last byte
        c = WireSlave.read();           //Receive byte as a character
        Serial.print(c);                //Print the character
            if (c == 'A') {             //Receive A > Change I2C adres
            x = WireSlave.read();       //Receive byte as an integer
            Serial.print(x);            //Read the next byte, this will be the addres send from the master.
            SetAtomID(x);               //Change I2C Adres to received adres in EEPROM
           
            } else if ( c == 'P') {
              for(int x=0;x<=75;x++) {
                picture[x] = WireSlave.read();       //Fill array
              }
            }
     
      else {
        Serial.print("Unknown command");
      }
    }
 
    x = WireSlave.read();       //Receive byte as an integer
    Serial.println(x);          //Print the integer
}

void setup() {    
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.begin(115200); // start serial for output
  delay(500);           // Wait for server to boot
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
    
  FastLED.addLeds < WS2812B, PIXEL_PIN, GRB > (leds, PIXEL_COUNT);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  GetAtomID();
  if ( (I2C_ADDRESS >= ATOM_FIRST) && ( I2C_ADDRESS <= ATOM_LAST) ) {
    ShowAtomID();
    delay(2000);  
    if(!WireSlave.begin(SDA_PIN, SCL_PIN, I2C_ADDRESS)) {
      Serial.println("I2C slave init failed");
    } else {    
      Serial.printf("client joined I2C bus with address: %d\n", I2C_ADDRESS);
      WireSlave.onRequest(requestEvent); // register event
      WireSlave.onReceive(receiveEvent);
    }
  } else {
    Serial.println("No valid I2C Address found");    
  }
}

void loop() {
  char buffer[100];
  delay(1);
  WireSlave.update();

/*
  if(newData == true){
    int led = 0;
    for (int x=0; x<=72; x+=3) {    
      RgbColor color(picture[x+2],picture[x+1],picture[x]); //RedGreenBlue
      strip.SetPixelColor(led, color);
      led = led + 1;
    }
    newData = false;
    Serial.println("New patern");
    strip.Show();
  }
*/

  /*
   * 2 : ENC_TYPE_TKIP - WPA / PSK 
   * 4 : ENC_TYPE_CCMP - WPA2 / PSK 
   * 5 : ENC_TYPE_WEP  - WEP 
   * 7 : ENC_TYPE_NONE - open network 
   * 8 : ENC_TYPE_AUTO - WPA / WPA2 / PSK
   */
  // WiFi.scanNetworks will return the number of networks found
  if( (I2C_ADDRESS >= 1) && (I2C_ADDRESS <= 13) ) {
    int n = WiFi.scanNetworks(false,true,false,100,I2C_ADDRESS);
    Serial.println("scan done");
    if (n > 0) {
      CountAP(n);
      for (int i = 0; i < n; ++i) {
        sprintf(buffer,"%d,%d,%d,%012x,%s",WiFi.channel(i),WiFi.RSSI(i),WiFi.encryptionType(i),WiFi.BSSID(i),WiFi.SSID(i));
        Serial.println(buffer);
        delay(10);
        }
    }
  }

  currentState = digitalRead(BUTTON_PIN);
  if(lastState == HIGH && currentState == LOW) {   // button is pressed
    pressedTime = millis();    
  } else {
    if(lastState == LOW && currentState == HIGH) { // button is released
      releasedTime = millis();
      long pressDuration = releasedTime - pressedTime;
      if( pressDuration < SHORT_PRESS ) {
        Serial.println("A short press is detected");
        buttonPressed = BUTTON_SHORT;
        I2C_ADDRESS++;
        if(I2C_ADDRESS>ATOM_LAST) I2C_ADDRESS=ATOM_FIRST;
        Serial.printf("Channel: %d\n",I2C_ADDRESS);
        ShowAtomID();
        delay(500);
      } else {
        Serial.println("A long press is detected");    
        buttonPressed = BUTTON_LONG;
      }
    }
  }
  // save the the last state
  lastState = currentState;
}
