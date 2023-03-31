#include <ArduinoJson.h>
/* --------------------------------------------------------------------- */
/* ----------- Sensor de Corriente ----------- */
/* --------------------------------------------------------------------- */
#include "ACS712.h"
//  Arduino UNO has 5.0 volt with a max ADC value of 1023 steps
//  ACS712 5A  uses 185 mV per A
//  ACS712 20A uses 100 mV per A
//  ACS712 30A uses  66 mV per A


ACS712  ACS(A3,3.3, 4095, 66);
//  ESP 32 example (might requires resistors to step down the logic voltage)
//  ACS712  ACS(25, 3.3, 4095, 185);

/* --------------------------------------------------------------------- */
/* ----- Sensor de Voltaje ----- */
/* --------------------------------------------------------------------- */
#include <Filters.h>       
#define ZMPT101B A0                            //Analog input                 

float testFrequency = 60;                     // frecuencia (Hz)
float windowLength = 100/testFrequency;       // how long to average the signal, for statistist, changing this can have drastic effect
                                              // Test as you need


/* --------------------------------------------------------------------- */
/* ----------- Conexion con firebase  ----------- */
/* --------------------------------------------------------------------- */
#include <WiFi.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "powermetric-84060-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "BSgoR7SBOEPZvF01Sg1yKD1DKHhQ18OdLwwBJSXu"
FirebaseData firebaseData;
FirebaseJson json; 
String path = "/nodos";
String pathDatos = "/datos";
String macAdd = "00:00:00:00:00:00";

/* --------------------------------------------------------------------- */
/* ----- Pantalla OLED 128x32 ----- */
/* --------------------------------------------------------------------- */
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* --------------------------------------------------------------------- */
/* ----------- Bluetooth ----------- */
/* --------------------------------------------------------------------- */
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

/* --------------------------------------------------------------------- */
/* ----------- Guardado de variable en memoria NVS  ----------- */
/* --------------------------------------------------------------------- */
#include <Preferences.h>

Preferences preferences; 

/* --------------------------------------------------------------------- */
/* ----------- NTP(Network Time Protocol) ----------- */
/* ----------- Obtener la hora real ----------- */
/* --------------------------------------------------------------------- */
#include <time.h>
int8_t GTMOffset = -6; // SET TO UTC TIME

const char* ntpServer = "mx.pool.ntp.org";
const long  gmtOffset_sec = GTMOffset*60*60;
const int16_t   daylightOffset_sec = 60*60*1000;



/* --------------------------------------------------------------------- */
/* ----------- Definicion de Funciones  ----------- */
/* --------------------------------------------------------------------- */

void conectWifi();
void saveWifiNet(const char* saveSSID, String savePASS);
void deleteWifiNet(String deleteSSID);
void uploadData(float V,float I);
void wifiConf();
void loadingAnimation(int16_t x,int16_t n);

/* --------------------------------------------------------------------- */
/* ----------- Variables globales  ----------- */
/* --------------------------------------------------------------------- */
float energy = 0;
int RawValue = 0;     
float V;     // estimated actual voltage in Volts

float intercept = 0;  // to be adjusted based on calibration testin
float slope = 0.1924; 

unsigned long printP = 1000; //Frecuencia de lectura en millisegundos
unsigned long uploadP = 60000; //Frecuencia de subida de datos
unsigned long preMillis = 0;
unsigned long currentMillis = 0;

float pricekWh = 0.912;

RunningStatistics inputStats;


int16_t cont = 0;
int16_t suma = 0;

int8_t uploadCont = 0;

String getTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "error";
  }
  char now_time [20];
  strftime(now_time,20, "%X %d/%m/%Y", &timeinfo);
  
  return String(now_time) ;
}

/* ------------------------------------------------------------------------------ */
/* ----------- Setup (Secuencia de instrucciones al iniciar el ESP32) ----------- */
/* ------------------------------------------------------------------------------ */
void setup()
{
  Serial.begin(9600);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
     Serial.println(F("SSD1306 allocation failed"));
     for(;;);
  }
  pinMode(15, INPUT);
  Serial.println("Startup");

  display.clearDisplay();
  display.setTextSize(2);            
  
  display.setTextColor(SSD1306_WHITE);        
  display.print(F("Iniciando"));
  display.display();

  conectWifi();
  macAdd = WiFi.macAddress();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

}

/* ------------------------------------------------------------------------------ */
/* ----------- Loop (Se repiten las instrucciones) ----------- */
/* ------------------------------------------------------------------------------ */
void loop()
{
  /*digitalWrite(pin_acs,HIGH);
  digitalWrite(pin_zmpt,LOW);
  float I = ACS.mA_AC() ;
  delay(500);
  
  digitalWrite(pin_acs,LOW);
  digitalWrite(pin_zmpt,HIGH);
  ReadVoltage();
  delay(500);*/
  
  RawValue = analogRead(ZMPT101B);  // lee valor del sensor de voltaje:
  inputStats.input(RawValue);       
  currentMillis = (unsigned long)millis();
  
  if((currentMillis - preMillis) >= printP) { 
    preMillis = currentMillis;

    float vsigma = inputStats.sigma();
    V = vsigma * slope + intercept;
    
    Serial.print("Non Calibrated: ");
    Serial.print("\t");
    Serial.print(vsigma); 
    Serial.print("\t");
    suma += vsigma;  
    cont++;
    float prom = suma/cont;
    Serial.print("Promedio: ");
    Serial.print("\t");
    Serial.print(prom);
    
    float I = ACS.mA_AC()/1000 ;
    float P = V * I;
    energy += P * ((printP*0.001)/(60*60)); 
    Serial.print("\tV = ");
    Serial.print(V);
    Serial.print("\tI = ");
    Serial.print(I);
    Serial.print("\tP = ");
    Serial.print(P);
    Serial.print("\tE = ");
    Serial.print(energy);
    Serial.println("mWh");

    display.clearDisplay();
    display.setTextSize(1);            
    display.setCursor(0, 0);
    display.print(F("V = "));
    display.print(V);
    display.print("V");
    display.setCursor(0, 8);
    display.print(F("I = "));
    display.print(I);
    display.print("A");
    display.setCursor(0, 16);
    display.print(F("P = "));
    display.print(P);
    display.print("W");
    display.setCursor(0, 24);
    display.print(F("E = "));
    display.print(energy);
    display.print("mWh");
    display.display();
    uploadCont++;
    if (uploadCont == 60)
    {
      uploadData(V,I);
      uploadCont = 0;
    }
  }


}


/* --------------------------------------------------------------------- */
/* ----------- Funciones  ----------- */
/* --------------------------------------------------------------------- */

// Guardar red wifi
void saveWifiNet(const char* saveSSID, String savePASS){
  preferences.begin("credentials", false);
  preferences.putString(saveSSID,savePASS);
  Serial.print("Red guardada ");
  Serial.print(saveSSID);
  Serial.print(" : ");
  Serial.println(preferences.getString(saveSSID));
  preferences.end();
}
// Borrar red wifi
void deleteWifiNet(String deleteSSID){
  preferences.begin("credentials", false);
  preferences.remove(deleteSSID.c_str());
  preferences.end();
}

// Conectarse con red wifi guardada
void conectWifi(){
  WiFi.mode(WIFI_STA);  
  for(int8_t times = 0 ; times < 3 ; times++)
  {
    int8_t n = WiFi.scanNetworks();
    if (n == 0)
      Serial.println("No se encontraron redes");
    else
    {
      preferences.begin("credentials", true);
      Serial.print(n);
      Serial.println("Redes encontradas:");
      for (int8_t i = 0; i < n; ++i)
      {
        Serial.print(WiFi.SSID(i));
        if(preferences.isKey(WiFi.SSID(i).c_str())){
          String pass = preferences.getString(WiFi.SSID(i).c_str(),"");
          Serial.print(":");
          Serial.println(pass);
          WiFi.begin(WiFi.SSID(i).c_str(),pass.c_str());

          display.clearDisplay();
          display.setTextSize(1);            
          display.setCursor(0, 0);
          display.print(F("Conectando a "));
          display.setCursor(0, 8);
          display.print(WiFi.SSID(i));
          display.display();

          int8_t c = 0;
          while ( c < 20 ) {
            if (WiFi.status() == WL_CONNECTED)
            {

              display.clearDisplay();
              display.setTextSize(2);            
              display.setCursor(0, 0);
              display.print(F("Conectado"));
              display.setTextSize(1); 
              display.setCursor(0,16);
              display.print("Red: ");
              display.print(WiFi.SSID(i));
              display.display();

              Serial.print("Connected to ");
              Serial.print(WiFi.SSID(i));
              Serial.println(" Successfully");
              preferences.end();
              return;
            }
            Serial.print("*");
            for(int8_t i = 1 ; i <= 6 ; i++){
              loadingAnimation(100,i);
              delay(100);
            }
            c++;
          }
          Serial.print("No se pudo conectar a ");
          Serial.println(WiFi.SSID(i));
          delay(10);
          }
        Serial.println(".");
      }
    }
    preferences.end();
  }
  wifiConf();
}


// Agregar o Modificar credenciales Wifi
void wifiConf(){
  SerialBT.begin("ESP32test");
  Serial.println("Bluetooth activado");
  String message = "";
  
  while(message != "exit"){
    if (SerialBT.available()){
      char incomingChar = SerialBT.read();
      if (incomingChar != '\n'){
        message += String(incomingChar);
      }
      else{
        message = "";
      }
      Serial.write(incomingChar);  
    }
    if(message == "getRedes"){
      StaticJsonDocument<200> doc;
      int8_t n = WiFi.scanNetworks();
      Serial.print("Redes:");
      Serial.println(n);
      for (int8_t i = 0; i < n; i++)
      {
        doc[WiFi.SSID(i)] = WiFi.RSSI(i);
      }
      serializeJson(doc, SerialBT);
      serializeJson(doc, Serial);
    }
    if(message.substring(0,8) == "savewifi" && message.endsWith("}")){
      String json = message.substring(9);
      Serial.println(message.substring(9));
      String ssid = json.substring(json.indexOf("{\"")+2,json.indexOf("\":"));
      String pass =json.substring(json.indexOf(":\"")+2,json.indexOf("\"}"));
      Serial.print("SSID:");
      Serial.print(ssid);
      Serial.print(" PASS:");
      Serial.println(pass);
      saveWifiNet(ssid.c_str(),pass);
    }
  }
  Serial.println("BT stopping");
  SerialBT.flush();  
  SerialBT.end();
  Serial.println("BT stopped");
  ESP.restart();
}

//  Almacena voltaje y corriente en firebase
void uploadData(float V,float I){
  
  String pushDataPath = path + "/" + macAdd + "/" + pathDatos; 
  json.add("voltaje",V)
      .add("corriente",I)
      .add("hora",getTime());
  if (!Firebase.push(firebaseData, pushDataPath , json)){
    Serial.println("Fallo al subir datos");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
  }else{
    Serial.println("Datos almacenados en firebase");
    Serial.println("------------------------------------");
    Serial.println();
  }
}

//  Animacion de carga
void loadingAnimation(int16_t x,int16_t n){
  int16_t num = 6;
  int16_t angulo = 360/num;
  int16_t radio = 7;
  int16_t  y = 16;
  display.fillCircle(x,y,radio+4,SSD1306_BLACK);
  for(int16_t i=1; i <= num; i++) {
    if(i == n ){
      display.fillCircle(radio*cos(DEG_TO_RAD*(angulo*(i+2)))+x, radio*sin(DEG_TO_RAD*(angulo*(i+2))) + y, 3 , SSD1306_WHITE);
      display.drawCircle(radio*cos(DEG_TO_RAD*(angulo*(i+1)))+x, radio*sin(DEG_TO_RAD*(angulo*(i+1))) + y, 3 , SSD1306_WHITE);
      display.drawCircle(radio*cos(DEG_TO_RAD*(angulo*i))+x, radio*sin(DEG_TO_RAD*(angulo*i))+ y, 2 , SSD1306_WHITE);
      display.drawCircle(radio*cos(DEG_TO_RAD*(angulo*(i-1)))+x, radio*sin(DEG_TO_RAD*(angulo*(i-1))) + y, 1 , SSD1306_WHITE);
      display.display();
    }
  }
}
