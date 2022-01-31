#include <SPI.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Radio pin assignments
#define CE 53 //9
#define CSN 48 //53

// Alarm pin assignment
#define peizo 28

// Keypad
const byte numRows = 4;
const byte numCols = 4;

char keymap[numRows][numCols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} };

byte rowPins[numRows] = {13, 12, 11, 10}; // Rows 0 to 3
byte colPins[numRows] = {9, 8, 7, 6}; // Cols 0 to 3

// Radio address
const byte address[][6] = {"TxAAA", "rXaaa"};

// Messages
const unsigned char hello = 5;
const unsigned char arm = 2;
const unsigned char alarmON = 3;
const unsigned char alarmOFF = 4;

// Data collectors
unsigned char data;
String btData;

// Flags
bool initialize = true;
bool isArmed = false;
bool isAlarmON = false;

// Disarm Code
const char code[4] = {'1', '1', '1', '1'};

char keypress[4]; // Collects input from keypad
int i=0; // Keep track of number of keypresses
char keyPressed = 0;
bool disarming = false; // Entering disarm code flag

// Create objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
RF24 radio(CE, CSN);
Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

// Prototype definitions
void getBTStream();

// Create a string from the BT stream
void getBTStream()
{
  btData = "";
  
  while (Serial1.available())
  {
    char inChar = (unsigned char)Serial1.read(); // Get new byte
    btData += inChar; // Append to btData
  }
}

void setup() 
{
  pinMode(peizo, OUTPUT);
  
  // Initialize and configure radio tranceiver
  radio.begin(); // Initialize radio
  radio.setPALevel(RF24_PA_MIN); // Set transmit range ~ 1 meter
  radio.setDataRate(RF24_2MBPS);
  radio.setChannel(124);
  radio.openWritingPipe(address[1]); // Bind address for writing
  radio.openReadingPipe(1, address[0]); // Bind address for reading

  // Initialize and configure LCD
  lcd.backlight();
  lcd.init();

  // Initialize serial for BT
  Serial1.begin(9600);
}

void loop() 
{
  if (initialize) // Confirm presence of sensor
  {
    radio.stopListening();
    
    if (!radio.write(&hello, sizeof(unsigned char)))
    {
      lcd.setCursor(0,0);
      lcd.print("No ACK");
    }
    
    radio.startListening();
    unsigned long started_waiting_at = millis();

    while(!radio.available())
    {
      if (millis() - started_waiting_at > 200)
      {
        lcd.setCursor(0,1);
        lcd.print("Timeout!");
        return;
      }
    }

    radio.read(&data, sizeof(unsigned char));
    lcd.clear();

    if (data == 6) // ACK code
    {
      initialize = false; // Done with initialization
      
      lcd.setCursor(0,0);
      lcd.print("Sensor Ready...");
      
      delay(2000);
      lcd.clear();
      lcd.print("System Ready...");
    }
    
  }
  else
  {
    keyPressed = myKeypad.getKey();
   
    if (radio.available())
    {
      while (radio.available()) // Check for message from sensor
      {
        radio.read(&data, sizeof(unsigned char));
      }

      if (data == 3) // Alarm On code
      {
        isAlarmON = true;
        
        radio.stopListening();
        radio.write(&alarmON, sizeof(unsigned char));
        radio.startListening();

        lcd.clear();
        lcd.print("Alarm ON");
        
        digitalWrite(peizo, HIGH);
      }
      else if (data == 4 || !isAlarmON) // Alarm Off code
      {

        isAlarmON = false;
        
        radio.stopListening();
        radio.write(&alarmOFF, sizeof(unsigned char));
        radio.startListening();

        lcd.clear();
        
        digitalWrite(peizo, LOW);
      }
    }

    if (Serial1.available() > 0) // Check for message from RFID
    {
      while (Serial1.available() > 0)
      {
        getBTStream();
      }
      
      if (btData == "0") // Valid RFID
      {
        radio.stopListening();
        radio.write(&alarmOFF, sizeof(unsigned char)); // Send alarm off code
        radio.startListening();
        
        isArmed=false;
        
        digitalWrite(peizo, LOW); // Turn off alarm
        
        lcd.clear();
        lcd.print("System Not Armed...");
      }
    }
    else if (keyPressed != NO_KEY)
    {
      if (keyPressed == 'A') // Arm the system
      {
        radio.stopListening();
        radio.write(&arm, sizeof(unsigned char));
        radio.startListening();
        isArmed = true;
        lcd.clear();
        lcd.print("System Armed...");
      }

      if (keyPressed == 'D' && isArmed)
      {
        disarming = true;
        lcd.clear();
        lcd.print("Enter Code:");
        lcd.setCursor(0,1);
        keyPressed = 0;
      }

      if (disarming)
      {
        if (keyPressed)
        {
          keypress[i++] = keyPressed;
          lcd.print(keyPressed);
        }

      }

      if (i==4)
      {
        bool valid = true;
        for (int k=0; k<4; k++)
        {
          if (keypress[k] == code[k]) continue;
          else valid = false;
        }
        if (valid)
        {
          radio.stopListening();
          radio.write(&alarmOFF, sizeof(unsigned char)); // Send alarm off code
          radio.startListening();
          
          isArmed=false;
          isAlarmON=false;
          disarming=false;
          i=0; // Reset keypad character count
          
          lcd.clear();
          lcd.print("System Not Armed...");

          digitalWrite(peizo, LOW); // Turn off alarm
        }
        else
        {
          lcd.clear();
          lcd.print("Wrong Password");
          
          i=0; // Reset keypad character count
          disarming=false;
        }
      }
    }  
  }
}
