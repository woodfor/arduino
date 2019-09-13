
#include "FirebaseESP8266.h"
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define FIREBASE_HOST "https://fir-250004.firebaseio.com"
#define FIREBASE_AUTH "YN38YPAvJCNgmJdx8tojcdsEFRSKs5AnpQ9UjjFf"
#define FIREBASE_FCM_SERVER_KEY "AAAAZvZ9w7c:APA91bF_OGX3ilFdJ2JMjuz3gN1x1WAZ2SUmi1KNurZutXxTKfZil8w_iQzlcltoJgHF3FOQUN1CpZKhrTkuYpPNQ0aNHlFe_Yc4TJNt7-220Udtb38o4ajDo1bCU9Oj7sDxFtnbCAV0"
// WiFi and server parameters to be configured
String ssid = "Zeon";
String password = "duedue87";
//bluetooth
SoftwareSerial btSerial(D5, D6); // Rx,Tx
//Fire base
int turn = 0;
FirebaseData firebaseData1;
FirebaseData firebaseData;
String FIREBASE_FCM_DEVICE_TOKEN_1 = "1" ;
//water flow
float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
//pin
const int buttonPin = D2; // waterflow sensor
const int pirPin = D0; //digital 2
const int valve = D1;  
//distance
int range[20];
int fixed=0;
long duration;
int distance;
int tankCount = 0;
//water sensor
byte sensorInterrupt = 0;
int sTimeCount = 0;
unsigned long interval = 0;
String deviceName="";
String houseID;
String tapID;

void ICACHE_RAM_ATTR pulseCounter();
  
void setup(void)
{ 
  
  Serial.begin(9600);
  btSerial.begin(9600);
//wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
//  if(! WiFi.config(ip, gateway, subnet)) {
//    Serial.println("Cannot set static IP"); 
//  } 
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    WiFi.setAutoConnect(true);
  
  //write pinmode
  pinMode(valve, OUTPUT);
  pinMode(pirPin, INPUT);
  pinMode(buttonPin, INPUT);  
 
   
  //setToken();
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
 
  //digitalWrite(buttonPin, HIGH);
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  attachInterrupt(digitalPinToInterrupt(buttonPin), pulseCounter, RISING);
  Serial.println("Good to go");
  
}



bool sendNotification(String title, String content)
{
  firebaseData1.fcm.setNotifyMessage(title, content);
  if (Firebase.sendMessage(firebaseData1, 0))//send message to recipient index 0
   {
      
        String result = firebaseData1.fcm.getSendResult();
        Serial.println(result);
        Serial.println("------------------------------------");
        Serial.println();
        return true;
   }
   else
   {
        setToken();
       // Serial.println("FAILED");
       // Serial.println("REASON: " + firebaseData1.errorReason());
       // Serial.println("------------------------------------");
      //  Serial.println();
      return false;
   }
}
bool sendFireData(String json)
{
  firebaseData1.fcm.setDataMessage(json);
  if (Firebase.sendMessage(firebaseData1, 0))//send message to recipient index 0
   {
        String result = firebaseData1.fcm.getSendResult();
        Serial.println(result);
        Serial.println("------------------------------------");      
        Serial.println();
        return true;
   }
   else
   {
        setToken();
        return false;
        
        //Serial.println("FAILED");
        //Serial.println("REASON: " + firebaseData1.errorReason());
        //Serial.println("------------------------------------");
        //Serial.println();
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
String setToken()
{
     if (WiFi.status() == WL_CONNECTED) 
  {
    HTTPClient http; //Object of class HTTPClient
    http.begin("http://capiwaterapi.ap-southeast-2.elasticbeanstalk.com/token");
    int httpCode = http.GET();

    
    if (httpCode > 0) 
    {
      
      String json = http.getString();
      StaticJsonDocument<2000> doc;
      DeserializationError error = deserializeJson(doc, json);

    if (error) {
      
      return "F";
  }

      const char* tmp = doc["token"];
      FIREBASE_FCM_DEVICE_TOKEN_1 = tmp;
      Serial.print(tmp);
      
      return "T";

    }
    http.end(); //Close connection
  }
}
int sort_desc(const void *cmp1, const void *cmp2)
{
  // Need to cast the void * to int *
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  // The comparison
  return a > b ? -1 : (a < b ? 1 : 0);
  // A simpler, probably faster way:
  //return b - a;
}
int mostFrequent(int arr[], int n) 
{ 
    // Sort the array 
     qsort(arr, n, sizeof(arr[0]), sort_desc); 
  
    // find the max frequency using linear traversal 
    int max_count = 1, res = arr[0], curr_count = 1; 
    for (int i = 1; i < n; i++) { 
        if ((arr[i] == arr[i - 1]) && (arr[i] !=0)) 
            curr_count++; 
        else { 
            if (curr_count > max_count) { 
                max_count = curr_count; 
                res = arr[i - 1]; 
            } 
            curr_count = 1; 
        } 
    } 
  
    // If last element is most frequent 
    if (curr_count > max_count) 
    { 
        max_count = curr_count; 
        res = arr[n - 1]; 
    } 
  
    return res; 
}
 
void loop() {


  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
  
     if (Firebase.getInt(firebaseData, "/turn")) {
    if  (firebaseData.dataType() == "int") {
      int val = firebaseData.intData();
      if (val != turn) {
        turn = val;
        if(turn==1){
          digitalWrite(valve,HIGH);
          }else{
            digitalWrite(valve,LOW);
            }        
      }
    }
  }
//      long pirVal = digitalRead(pirPin);
//      if(pirVal == LOW){ //was motion detected
//      Serial.println("Motion Detected");
//      }
      
     // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(digitalPinToInterrupt(buttonPin));
        
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    flowMilliLitres = (flowRate / 60) * 1000;
    
    totalMilliLitres += flowMilliLitres;
     /* */
    unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
//    Serial.print("Flow rate: ");
//    Serial.print(flowRate);  // Print the integer part of the variable
//    Serial.print("L/min");
//    Serial.print("\t");       // Print tab space
//
//     //Print the cumulative total of litres flowed since starting
//    Serial.print("Output Liquid Quantity: ");        
//    Serial.print(totalMilliLitres);
//    Serial.println("mL"); 
//    Serial.print("\t");       // Print tab space
//    Serial.print(totalMilliLitres/1000);
//    Serial.print("L");
    

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(buttonPin), pulseCounter, FALLING);
    if(flowRate > 0)
    {
      sTimeCount ++;
      if(sTimeCount > 10 )//&& (millis() - interval > 60000)
      {
        int count = 0;
        while(!sendNotification("IOT","Your tap is on, at least 2 seconds")){
          count++;
          if(count>2){
            break;
           }
       }
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
        int count = 0;
        while(!sendFireData("{\"U\":\"" + String(totalMilliLitres,DEC) + "\"," + "\"D\":\"" + String(sTimeCount) 
          + "\"" +"," +"\"T\":\""+ time + "\""+ "," + "\"N\":\"" + deviceName +"\""+ "," + "\"hid\":\"" + houseID +"\"" + "," + "\"tid\":\"" + tapID +"\"" +"}")){
           count++;
          if(count>2){
            break;
           }
        }
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
      deviceName = getValue(content,'/',0);
      houseID = getValue(content,'/',1);
      tapID = getValue(content,'/',2);
      btSerial.println("1");//1 for respond device name
      Serial.println(deviceName);
    }else if (identifier == "UN"){
      deviceName = getValue(content,'/',0);
      houseID = getValue(content,'/',1);
      tapID = getValue(content,'/',2);
      btSerial.println("UC");//update complete
      Serial.println(deviceName);
    }
    else if(identifier == "S"){
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
//sound wave
/* 
 *    for(int i=0;i<20;i++){
    range[i] = 0;
    }
 *  
    digitalWrite(trigPin,LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin,HIGH,20000);
   if(duration == 0) // If we timed out
    {
      pinMode(echoPin, OUTPUT); // Then we set echo pin to output mode
      digitalWrite(echoPin, LOW); // We send a LOW pulse to the echo pin
      delayMicroseconds(200);
      pinMode(echoPin, INPUT); // And finaly we come back to input mode
    }

    distance= int ((duration/2) / 29.1);    
    Serial.println(distance);
    //set tank range
    if(tankCount < 20){      
      range[tankCount] = distance;
      tankCount ++;   
    }else if (tankCount>=20){
      tankCount = 0;
      int n = sizeof(range)/sizeof(range[0]);
      fixed = mostFrequent(range,n);
      Serial.println("fixed: ");
      Serial.println(fixed);
    } 
   
    if(distance < fixed)  {
      //couting waste water
     } 
*/  
 
  
  
  
}
