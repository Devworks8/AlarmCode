#include <SPI.h>
#include <RF24.h>

// Radio pin assignments
#define CE 9
#define CSN 10

// PIR Sensor pin assignment
#define PIR 8

// Create radio object
RF24 radio(CE, CSN);

// Radio address
const byte address[][6] = {"TxAAA", "rXaaa"};

// Messages
const unsigned char response = 6;
const unsigned char alarmON = 3;
const unsigned char alarmOFF = 4;

unsigned char data; // Data collector
long sst; // Set system start time

// Flags
bool isArmed = false;
bool isAlarmON = false;
bool initialize = true;

// Define method prototypes
void CheckForCode();

void CheckForCode()
{
  radio.startListening();
  
  if (radio.available())
    {
      while(radio.available())
      {
        radio.read(&data, sizeof(unsigned char));
      }

      if (data == 2) isArmed = true;
      else if (data == 3) isAlarmON = true;
      else if (data == 4)
      {
        isAlarmON = false;
        isArmed = false;
      }
      else if (data == 5) initialize = true;
    }
}

void setup() 
{
  pinMode(PIR, INPUT);

  // Initialize and configure readio tranceiver
  radio.begin(); // Initialize radio
  radio.setPALevel(RF24_PA_MIN); // Set tansmit distance ~ 1 meter
  radio.setDataRate(RF24_2MBPS);
  radio.setChannel(124);
  radio.openWritingPipe(address[0]); // Bind address for writing
  radio.openReadingPipe(1, address[1]); // Bind address for reading
  
  radio.startListening();
}

void loop() 
{
  if (initialize) // Wait for a hello code from the Hub
  {
    if (radio.available())
    {
      while (radio.available())
      {
        radio.read(&data, sizeof(unsigned char));
      }
    }
    
    if (data == 5) // Hello code
    {
      radio.stopListening(); // Set as transmitter
      radio.write(&response, sizeof(unsigned char)); // Send ACK code
      radio.startListening(); // Resume listen for HUB
      
      initialize = false; // Done with initialization 
      sst = millis(); // Set start time
    } 
  }
  else
  {
    bool reset = false;
    long cnt;

    while (!reset)
    {
        if (millis() - sst > 5000) // Check for a code from the Hub every 5 seconds
        {
          CheckForCode();
          sst = millis(); // Reset system start time
          reset = true; // Reset the loop, just in case hello code received
        }
        else
        {
          if (digitalRead(PIR) == HIGH) // Motion detected
          {
            cnt = millis(); // Set disarm start timer
            
            while (isArmed)
            {
              CheckForCode();
              delay(200); // Allow time to parse code
              
              if (millis() - cnt > 15000 && !isAlarmON) // Give 15 secinds to disarm before the alarm turn on
              {
                radio.stopListening();
                radio.write(&alarmON, sizeof(unsigned char)); // Send the alarm on code
                radio.startListening();
              }

              while (isAlarmON)
              {
                CheckForCode();
                delay(200); // Allow time to parse code
                
                reset = true; // Reset the loop, just in case hello code received
              }
            }
          }
        }
     }
  }
}
