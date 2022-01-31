#include <SPI.h>
#include <MFRC522.h>

// BT assignments
#define btBaudRate 9600
#define TX 5
#define RX 6
#define redLED 3
#define greenLED 4

// RFID pin assignments
#define ssPin 10
#define rstPin 13

// Valid UID
String UID = "3C 18 20 18";

const char valid = '0';

MFRC522 rfid(ssPin, rstPin); // Create RFID object

// Method prototypes
void LEDState(bool valid);

void LEDState(bool authorized)
{
  if (authorized)
  {
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED,HIGH);
  }
  else
  {
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED,HIGH);
  }
}

void setup() 
{
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  SPI.begin(); // Initialize SPI bus
  rfid.PCD_Init(); // Initialize RFID
  Serial.begin(9600);
}

void loop() 
{
  LEDState(false);
  // Look for a new card
  if (!rfid.PICC_IsNewCardPresent())
  {
    return;
  }

  // Select one the cards
  if (!rfid.PICC_ReadCardSerial())
  {
    return;
  }

  String content= "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  
  if (content.substring(1) == "3C 18 20 18")
  {
    LEDState(true);
    // Send code to HUB
    Serial.print(0);
    delay(1000);
  }
}
