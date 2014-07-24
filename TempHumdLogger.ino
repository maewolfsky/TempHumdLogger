// Based on the DHTtester and data logger sheild: script written by ladyada 
//  found at https://github.com/adafruit/DHT-sensor-library

#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "DHT.h"
#define DHTPIN 2   // what pin the data out is connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define VOLTPIN 

#define LOG_INTERVAL 30000
#define SYNC_INTERVAL 60000

uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL 1 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()
#define PROFILE 0 // Whether or not to calculate how long it takes to query the sensor and print it to the console

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// I'm seeing strange readings near the time the batteries run out. Assuming
//  this is because low voltage is either messing up the readings from RTC
//  or the DHT or the writing to the SD card.

// Because of this, I want to add a voltage measurement to the logging
//  and either turn off the Arduino when the batteries are low, or 
//  just stop logging

// Looking at the datasheet for the regulator on the Duemilanove, looks like
//  if the input voltage drops below 7V things are going to get funky. I've
//  never measured the voltage of the eneloops after the Arduino shutdown
//  so I guess I don't know what the voltage has dropped to.

// So, going to add Analog 0 as the voltage measure.  Since this is deisgned to 
//  work with a 6xAA battery pack (9volt max), we need to get this voltage down
//  since the analog in only measures 0-5v. Voltage divider to the rescue!
//  Using two resitors of matching values, that brings 9v down to 4.5 volt.

RTC_DS1307 RTC;
DHT dht(DHTPIN, DHTTYPE);

const int chipSelect = 10;
File logfile;
unsigned long start_millis; // Used to determine how long the code actually takes to execute.


void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);

  while(1);
}


void setup() {
  Serial.begin(9600); 
  dht.begin();

  Serial.println();
  
#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break; // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif //ECHO_TO_SERIAL
  }
  

  logfile.println("datetime,temperature,humidity");
#if ECHO_TO_SERIAL
  Serial.println("datetime,temperature,humidity");
#endif //ECHO_TO_SERIAL
 
  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}


// Get the current time from the RTC and write it to the
//  output file and console (if ECHO_TO_SERIAL is set)
void logTime() {
  DateTime now;
  
  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.year(), DEC);
  logfile.print("-");
  if (now.month() < 10) { 
    logfile.print('0'); 
  } 
  logfile.print(now.month(), DEC);
  logfile.print("-");
  if (now.day() < 10) { 
    logfile.print('0'); 
  } 
  logfile.print(now.day(), DEC);
  logfile.print("T");
    if (now.hour() < 10) { 
    logfile.print('0'); 
  }
  logfile.print(now.hour(), DEC);
  logfile.print(":");
    if (now.minute() < 10) { 
    logfile.print('0'); 
  } 
  logfile.print(now.minute(), DEC);
  logfile.print(":");
    if (now.second() < 10) { 
    logfile.print('0'); 
  } 
  logfile.print(now.second(), DEC);

#if ECHO_TO_SERIAL
  Serial.print(now.year(), DEC);

  Serial.print("-");

  if (now.month() < 10) { 
    Serial.print('0'); 
  } 
  Serial.print(now.month(), DEC);

  Serial.print("-");

  if (now.day() < 10) { 
    Serial.print('0'); 
  } 
  Serial.print(now.day(), DEC);
  Serial.print("T");
  if (now.hour() < 10) { 
    Serial.print('0'); 
  }   Serial.print(now.hour(), DEC);
  Serial.print(":");
  if (now.minute() < 10) { 
    Serial.print('0'); 
  } 
  Serial.print(now.minute(), DEC);
  Serial.print(":");
    if (now.second() < 10) { 
    Serial.print('0'); 
  } 
  Serial.print(now.second(), DEC);
#endif //ECHO_TO_SERIAL
  
}


void loop() {
  // use millis() to make sure that we have a sample exactly every LOG_INTERVAL?
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));

  logTime();
#if PROFILE
  start_millis = millis(); // Get current millis so we can time the code below
#endif //PROFILE

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature(HIGH); //HIGH = F  LOW = C
    
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
  }

  logfile.print(", ");
  logfile.print(t);
  logfile.print(", ");
  logfile.print(h);
  logfile.println();
  
#if ECHO_TO_SERIAL
  Serial.print(", ");
  Serial.print(t);
  Serial.print(", ");
  Serial.print(h);
  Serial.println();
#endif //ECHO_TO_SERIAL

#if PROFILE  // Print out the time it took to execute the code
  Serial.print("Code took: ");
  Serial.println(millis() - start_millis); 
#endif //PROFILE

  //digitalWrite(greenLEDpin, LOW);
  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
    
  logfile.flush();
}
