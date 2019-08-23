
#include "FirebaseESP8266.h"
#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define FIREBASE_HOST "https://fir-250004.firebaseio.com"
#define FIREBASE_AUTH "YN38YPAvJCNgmJdx8tojcdsEFRSKs5AnpQ9UjjFf"
#define FIREBASE_FCM_SERVER_KEY "AAAAZvZ9w7c:APA91bF_OGX3ilFdJ2JMjuz3gN1x1WAZ2SUmi1KNurZutXxTKfZil8w_iQzlcltoJgHF3FOQUN1CpZKhrTkuYpPNQ0aNHlFe_Yc4TJNt7-220Udtb38o4ajDo1bCU9Oj7sDxFtnbCAV0"
#define FIREBASE_FCM_DEVICE_TOKEN_1 "dReBxATBAJI:APA91bF09anWpQuikaARI1j4nP-U_pTmuCWYNXnkeqmY7kwfwuDulWDvcv8Hk0p7a7bO_D9J3aUGBOISi7ot8DTxDJa3ELlNAm2-_g7cBsAJPHwSxBpshkISUgctaknC8OgclWgdw3KN"
// WiFi parameters to be configured
String ssid = "Zeon";
String password = "duedue87";
const int analogInPin = A0;
int sensorValue = 0;  // value read from the pot
int outputValue = 0;  // value to output to a PWM pin
SoftwareSerial btSerial(D5, D6); // Rx,Tx
//Fire base
FirebaseData firebaseData1;

//water flow
float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
const int buttonPin = D2;
byte sensorInterrupt = 0;
int sTimeCount = 0;
unsigned long interval = 0;
String deviceName="";
void ICACHE_RAM_ATTR pulseCounter();
  
void setup(void)
{ 
  
  /*
  
  
  // Connect to WiFi
  WiFi.begin(ssid, password);

  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  //print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());*/

  //btSerial.begin(9600); // bluetooth serial
  Serial.begin(9600);
  btSerial.begin(9600);


 /* WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    WiFi.setAutoConnect(true);*/
  // inital firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
/*
  if (!Firebase.beginStream(firebaseData2, path + "/" + nodeID))
   {
        Serial.println("Could not begin stream");
        Serial.println("REASON: " + firebaseData2.errorReason());
        Serial.println();
   }*/
  firebaseData1.fcm.begin(FIREBASE_FCM_SERVER_KEY);
  firebaseData1.fcm.addDeviceToken(FIREBASE_FCM_DEVICE_TOKEN_1);
  firebaseData1.fcm.setPriority("high");
  firebaseData1.fcm.setTimeToLive(1000);
  
  // initial sensor
  pinMode(buttonPin, INPUT);
  //digitalWrite(buttonPin, HIGH);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  attachInterrupt(digitalPinToInterrupt(buttonPin), pulseCounter, RISING);
  Serial.println("Good to go");
 
}

void sendNotification(String title, String content)
{
  firebaseData1.fcm.setNotifyMessage(title, content);
  if (Firebase.sendMessage(firebaseData1, 0))//send message to recipient index 0
   {

        Serial.println("PASSED");
        Serial.println(firebaseData1.fcm.getSendResult());
        Serial.println("------------------------------------");
        Serial.println();
   }
   else
   {
        Serial.println("FAILED");
        Serial.println("REASON: " + firebaseData1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
   }
}
void sendFireData(String json)
{
  firebaseData1.fcm.setDataMessage(json);
  if (Firebase.sendMessage(firebaseData1, 0))//send message to recipient index 0
   {

        Serial.println("PASSED");
        Serial.println(firebaseData1.fcm.getSendResult());
        Serial.println("------------------------------------");
        Serial.println();
   }
   else
   {
        Serial.println("FAILED");
        Serial.println("REASON: " + firebaseData1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
   }
}
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String getTime()
{
     if (WiFi.status() == WL_CONNECTED) 
  {
    HTTPClient http; //Object of class HTTPClient
    http.begin("http://worldclockapi.com/api/json/est/now");
    int httpCode = http.GET();

    
    if (httpCode > 0) 
    {
      
      String json = http.getString();
      StaticJsonDocument<2000> doc;
      DeserializationError error = deserializeJson(doc, json);

    if (error) {
      
      return "F";
  }
    
      const char* time = doc["currentDateTime"];

      
      return time;

    }
    http.end(); //Close connection
  }
}

void loop() {
  /*
  sensorValue = analogRead(analogInPin);
  outputValue = map(sensorValue, 0, 1024, 0, 30000); // 30000ml
  Serial.print("sensor = ");
  Serial.print(sensorValue);
  Serial.print("\t output = ");
  Serial.println(outputValue);*/
  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(digitalPinToInterrupt(buttonPin));
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
     /* */
    unsigned int frac;
    /*
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(flowRate);  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

     //Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");        
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 
    Serial.print("\t");       // Print tab space
    Serial.print(totalMilliLitres/1000);
    Serial.print("L");
    */

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(buttonPin), pulseCounter, FALLING);
    if(flowRate > 0)
    {
      sTimeCount ++;
      if(sTimeCount > 300 && (millis() - interval > 60000))
      {
        sendNotification("IOT","Your tap is on, at least 5 minutes");
        Serial.print(sTimeCount);
        Serial.print(interval);
        Serial.print(totalMilliLitres);
        interval = millis();            
      }
     
    }
   else
    {
      if(sTimeCount > 0)
      { 
        String time = getTime();
        sendFireData("{\"U\":\"" + String(totalMilliLitres,DEC) + "\"," + "\"D\":\"" + String(sTimeCount) 
          + "\"" +"," +"\"T\":\""+ time + "\""+ "," + "\"N\":\"" + deviceName +"\""  +"}");
        sTimeCount = 0;
        totalMilliLitres = 0;
      } 
    }
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
 }

 
  //bluetooth setUp
if (btSerial.available() > 0) {

   // check if bluetooth module sends some data to esp8266
    String data = btSerial.readString();  // read the data from HC-05
    Serial.println(data);
    data.trim();
    String identifier = getValue(data,':',0);
    String content = getValue(data,':',1);
    Serial.println(identifier);
    
    if (identifier == "N")
    {
      deviceName = content;
      btSerial.println("1");//1 for respond device name
      Serial.println(deviceName);
    }else if(identifier == "S"){
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      delay(100);
  
    // WiFi.scanNetworks will return the number of networks found
      int n = WiFi.scanNetworks();
      if (n == 0) {
        btSerial.println("-2");
      } else {
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        btSerial.print(WiFi.SSID(i));
        btSerial.print(" (");
        btSerial.print(WiFi.RSSI(i));
        btSerial.print(")");
        btSerial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        delay(10);
        }
      } 
    }else if(identifier == "C"){
       ssid = getValue(content,'/',0);
       password = getValue(content,'/',1);
       Serial.println(ssid);
       Serial.println(password);
       WiFi.begin(ssid, password);
          int count = 0;
          while (WiFi.status() != WL_CONNECTED) {
             delay(500);
             count ++;
             btSerial.print(".");
             if(count == 40){break;}
           
          }
           if(WiFi.status() != WL_CONNECTED && count == 40 )
           {
              btSerial.println("-3"); //connect failed
           }
           else
           {
              Serial.println("success");
              btSerial.println("3");
           }
      
      }
    
    //btSerial.println(data);
  
    
    btSerial.flush();
}

//control water flow

  
 
  
  
  
}
