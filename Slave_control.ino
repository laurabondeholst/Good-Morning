#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <time.h>

// for WiFi
char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;
String myStatus = "";

//for ThingSpeak
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
const char * ReadAPIKey = SECRET_READ_APIKEY;

//setup Lightsensor pin
const int lightsensor = A0; // select the input pin for LDR

//For the motorflag
const byte BlindStatusField = 3; 
boolean BlindStatus = 0; //1 is for open blind, 0 is for closed 
boolean OldBlindStatus = 0;

//motor setup
const byte motor = 16; //pin D1

//for time handling
int timezone = 1 * 3600;
int dst = 0;
time_t now = time(nullptr);
struct tm* p_tm = localtime(&now);

void setup() 
{
  Serial.begin(115200);  // Initialize serial

  //motor setup
  pinMode(motor, OUTPUT);
  
  setup_wifi();
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  setup_time();
}
void loop() 
{

  now = time(nullptr); 
  p_tm = localtime(&now);
  if(p_tm->tm_sec == 0)
  {
      
      int LightValue = analogRead(lightsensor); // read the value from the sensor
      Serial.print("Light intensity : ");
      Serial.println(LightValue); 
     
      // Write lightvalue to ThingSpeak
      int x = ThingSpeak.writeField(myChannelNumber, 1, LightValue, myWriteAPIKey);
      successful_upload(x);
    
      //get blindstatus from thingspeak
      BlindStatus = ThingSpeak.readLongField(myChannelNumber,BlindStatusField, ReadAPIKey);  
    
       // Check the status of the read operation to see if it was successful
      successful_download();
      
      //Open or close blinds
      if(BlindStatus == 1 && BlindStatus != OldBlindStatus)
      { //Open blinds
        run_motor();
        OldBlindStatus = 1;
      }
      else if (BlindStatus == 0 && BlindStatus != OldBlindStatus)
      { //close blinds
        run_motor();
        OldBlindStatus = 0;
      }

      if(p_tm->tm_hour ==22 && p_tm->tm_min ==0)
      { // close the blinds every evening af 22:00
        delay(20000);
        // Write lightvalue to thingspeak
        BlindStatus = 0;
        int x = ThingSpeak.writeField(myChannelNumber, BlindStatusField, BlindStatus, myWriteAPIKey);
        successful_upload(x);
      }
   }
   delay(100);
}

void run_motor()
{
    //start motor
    digitalWrite(motor,1);
    Serial.println("run motor");
    delay(5000);
    digitalWrite(motor,0); 
}

void setup_time()
{
    // time config
    configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
    Serial.println("\n Waiting for time");
    while(!time(nullptr))
    {
         Serial.print("*");
         delay(1000);
    }
    Serial.println("\nTime..OK");   
    now = time(nullptr); 
    p_tm = localtime(&now);
}

void setup_wifi()
{
  WiFi.mode(WIFI_STA); 
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network.
      Serial.print("No connection");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
}

void successful_download(){
      // displays error codes if download was not succesful
      int statusCode = 0;
      statusCode = ThingSpeak.getLastReadStatus();
      if(statusCode == 200)
      {
        Serial.println("MotorStatus " + String(BlindStatus));
      }
      else
      {
        Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
      }  
}

void successful_upload(int x)
{
  // displays error codes if upload was not succesful
  if(x == 200)
  {
    Serial.println("Channel update successful.");
  }
  else
  {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}
