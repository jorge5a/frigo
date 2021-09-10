


#include <WiFiManager.h>

#include <PubSubClient.h>
#include <WebServer.h>
/*
 * Software for basic tests of temperature measurement on M5Stack-FIRE with several DS18B20 temperature sensors
 * 1-wire-pull-up resistor = 2 kohm  (did not not work with 4,7 kohm !!!!
 * modified by GRELM; 2019-06-16
 */
#include <WiFi.h>   
  #include <M5Stack.h>
  #include <OneWire.h>
  #include <DallasTemperature.h>

#define TRIGGER_PIN 39
#define FRECUENCY 10000
#define ONE_WIRE_BUS 21 //DS18B20 on Grove B corresponds to GPIO26 on ESP32
#define FORMAT_SPIFFS_IF_FAILED false
bool reconfig=false;
const char* fwUrlBase = "http://test.ga1a.eu/ota/";
const int FW_VERSION = 1;
const char*  topic = "areaproject/frigo";
const char* TYPE = "frigo1";
const char* mqtt_server = "mercajucarc.dyndns.org";
const char* mqttUser = "bqtest";
const char* mqttPassword = "almaciga";

unsigned long lastupdate = 0; // check FW update every FRECUENCY*720
unsigned long sendupdate = 0; // minimal sendupadate
double tempC = 0;
double oldtempC=0;
double minTemp = 200;
double maxTemp = 0; 
char* ssid,pass;

String mac;
WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);


  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature DS18B20(&oneWire);




  void setup() {
      if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
getMac();
client.setServer(mqtt_server, 1883);
      pinMode(TRIGGER_PIN,INPUT);
   

    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), reconfigure, FALLING);
 M5.begin();
    //Modo configuracion Wifi
M5.Lcd.fillScreen(WHITE);
   M5.Lcd.setTextColor(BLACK);
   M5.Lcd.setTextSize(2);
   M5.Lcd.setCursor(40, 40);
   M5.Lcd.printf("Push A for Config Mode");
   
while (millis()<10000){
  delay2(1);
 M5.Lcd.progressBar( 40,  100, 200, 50, millis()/100);
}
  detachInterrupt(digitalPinToInterrupt(TRIGGER_PIN)) ;

    ///
   M5.Lcd.fillScreen(WHITE);
   M5.Lcd.setTextColor(BLACK);
   M5.Lcd.setTextSize(2);
   M5.Lcd.setCursor(40, 10);
   M5.Lcd.printf("AREAPROJECT Thermometer");
   M5.Lcd.fillCircle(65, 180, 30, RED);
   M5.Lcd.fillRect(55, 60, 20, 150, RED);
   M5.Lcd.fillCircle(65, 60, 10, RED);    

   
}
  
  void loop() {
    

   
   
    DS18B20.begin();
    int count = DS18B20.getDS18Count();  //check for number of connected sensors



    if (count > 0) {
    DS18B20.requestTemperatures();
   
     tempC = DS18B20.getTempCByIndex(0);
   if(tempC > maxTemp)
      maxTemp = tempC;
   if(tempC < minTemp)
      minTemp = tempC;      
   Serial.println(tempC,1);
   M5.Lcd.fillRect(140, 40, 170, 190, YELLOW);
   M5.Lcd.setTextSize(4);
   M5.Lcd.setTextColor(BLUE);
   M5.Lcd.setCursor(175, 80);
   M5.Lcd.printf("%.1f",tempC);
   M5.Lcd.setTextSize(2);
   M5.Lcd.setTextColor(BLACK);
   M5.Lcd.setCursor(170, 150);
   M5.Lcd.printf("MIN: %.1f",minTemp);
   M5.Lcd.setCursor(170, 190);
   M5.Lcd.printf("MAX: %.1f",maxTemp);
    }
  if (readSensors()) {

    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    
    sendValues();
  }

  delay2(FRECUENCY);



}
   
  //mqtt



void reconnect() {
  // Loop until we're reconnected
  int i=0;
  

String ssid=readFile(SPIFFS, "/ssid.txt");
String pass=readFile(SPIFFS, "/pass.txt");

WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  while (!client.connected() && i < 8 ) {
    i++;
    Serial.print("Attempting MQTT connection...");

Serial.print("Wifi state:");
  Serial.println(WiFi.status());
  Serial.println( WiFi.SSID() );
 Serial.println( WiFi.psk() );
    // Attempt to connect
    if (client.connect(mac.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");



    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 10 seconds");
      // Wait 5 seconds before retrying
      delay2(10000);
      
    }
  }
  if (i==8){
    Serial.println(F("Restarting....."));
    delay(500);
    ESP.restart();
  }
}



bool sendValues() {
  String s;
   int msgLen = 0;

   Serial.print("Requesting URL: ");
s="{\"key\":\"type\",\"value\":\""+String(TYPE)+"\"},{\"key\":\"version\",\"value\":"+String(FW_VERSION)+"}, {\"key\":\"mac\",\"value\":\""+mac+"\"}";
  s += ",{\"key\":\"Temperatura\",\"value\":" + String(tempC) + "}";

  
     client.loop();

  msgLen += s.length()+2;
  Serial.print("msgLen=");Serial.println(msgLen);
 client.beginPublish(topic, msgLen, false);    Serial.print("Publish message: ");
    client.print("[");
    client.print(s);
   
    client.print("]");
     client.endPublish();
    
    delay2(1000);
     

  Serial.println(s);
  Serial.println("closing connection");



  return true;
}

bool readSensors() {
  
  

  

  Serial.print(F("Temp 1= ")); Serial.println(tempC);
 
  if ((diferencia(tempC, oldtempC) > 10)  || millis() - sendupdate > FRECUENCY * 30 ) {
    sendupdate = millis();

    oldtempC = tempC;
    


    return true;
  }
  else {
    return false;
  }



}
int diferencia(double a, double b) {
  Serial.println(max(a - b, b - a));

  return max(a - b, b - a) * 100;
}
  // is configuration portal requested?
void reconfigure() {
       reconfig=true;

  }
  void delay2(unsigned long espera)
{
  unsigned long tiempo = millis();



  while ( millis() - tiempo < espera) {
    if (tiempo > millis()) tiempo = 0;
if (reconfig){ 
  detachInterrupt(digitalPinToInterrupt(TRIGGER_PIN)) ;
  reconfig=false;
  
    wifiManager.resetSettings();
delay(1000);
//pantalla de mensajes


M5.Lcd.fillScreen(RED);
M5.Lcd.setCursor(10,100);
M5.Lcd.print("Wifi Config Mode...");

    if (!wifiManager.autoConnect("AutoConnectAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
    char ssid[25];
    char pass[25];
    strcpy(ssid,WiFi.SSID().c_str());
    strcpy(pass,WiFi.psk().c_str());
    writeFile(SPIFFS, "/ssid.txt", ssid);
    writeFile(SPIFFS, "/pass.txt", pass);
   // attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), reconfigure, FALLING);
ESP.restart();
}

    delay(10);
    
  }
}

  void getMac() {

  mac = WiFi.macAddress();
  mac.replace(":", "-");


}

String readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
   char a[25];
   int i=0;
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        
    }

    Serial.println("- read from file:");
    while(file.available()){
      a[i]=file.read();
      i++;
       
        
    }
    a[i]='\0';
     Serial.write(a);
    file.close();
    return a;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}
