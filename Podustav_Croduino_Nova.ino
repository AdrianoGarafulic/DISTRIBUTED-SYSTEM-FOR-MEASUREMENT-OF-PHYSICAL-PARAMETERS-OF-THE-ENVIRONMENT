/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-mqtt-publish-bme280-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_SGP30.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "GAASI_WIFI-EXT-2"
#define WIFI_PASSWORD "armando1"

#define SEALEVELPRESSURE_HPA (1013.25)//za bme 680

Adafruit_SGP30 sgp; 
Adafruit_BME680 bme; // I2C

// Raspberry Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, 14)
#define MQTT_PORT 1883

//  MQTT Teme
#define MQTT_PUB_TEMP "nova/Temperatura"
#define MQTT_PUB_HUM "nova/Vlaznost"
#define MQTT_PUB_PRES "nova/Pritisak"
#define MQTT_PUB_CO2 "nova/CO2"
#define MQTT_PUB_VOC "nova/VOC"
#define MQTT_PUB_CO2_base "nova/co2_base"
#define MQTT_PUB_VOC_base "nova/voc_base"
#define MQTT_PUB_SOLAR "nova/Zracenje_sunca"
#define MQTT_PUB_PROVJERA_SGP "nova/sgp_provjera"
#define MQTT_PUB_PROVJERA_BME "nova/bme_provjera"
#define MQTT_PUB_PROVJERA_SOLAR "nova/solar_provjera"
#define MQTT_PUB_WIFI_SIGNAL "nova/wifi_signal"
#define MQTT_CO2_BASE_PRETPLATA "nova/co2_baza"
#define MQTT_VOC_BASE_PRETPLATA "nova/baza_voc"
// Variables to hold sensor readings
float temp,hum,pres,radsolar;
int SOLAR = A0;
bool a=0,c=0;//varijable za provjeru senzora
float temp_check,radsolar_check,pres_check,hum_check;                       
int eCO2_check,TVOC_check;                                                                     
int b=0, brojac_baseline  = 0;;//brojači
uint16_t TVOC_base, eCO2_base;//varijable za slanje vrijednosti mqtt-om
String co2_baza,voc_baza;//varijable za primanje baseline vrijednosti mqtt
int co2b, vocb;//tip varijable koji prima sgp.setIAQBaseline(co2b, vocb);
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
int co2=38465;
int voc=38015;
//39169 int voc=38407
unsigned long previousMillis = 0;   // Sprema zadnje vrijeme kad je objavljeno
const long interval = 360000;        // Interval za slanje vrijednosti

void setup() 
{
 Serial.begin(115200);
 pinMode(SOLAR, INPUT);

 sgp.begin();  
 bme.begin(0x76);
 


  
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  
  connectToWifi();//funkcija za spajanje na wifi
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // autentifikacija postavljena na MQTT posredniku
  mqttClient.setCredentials("adi", "pagtrg40");
 
}

void loop() 
{  
   //Serial.println(sgp.TVOC);
   //Serial.println(sgp.eCO2);
 unsigned long currentMillis = millis();
 // Svakih (interval) sekundi  
 // Objavljuje MQTT poruke
  if (currentMillis - previousMillis >= interval) 
  {
   // Spremanje zadnjeg vremena kad je objavljeno
    previousMillis = currentMillis;
   radsolar = (5/3.3)*3.30 /1023 *1000/1.67*analogRead(SOLAR);//svakih 1,67 mV znači promjenu od 1w/m2
          
    if ( bme.performReading())
    { 
    temp=bme.temperature;
    hum=bme.humidity;
    pres=bme.pressure / 100.0;
    sgp.setHumidity(getAbsoluteHumidity(bme.temperature, bme.humidity)); //Korektiraj mjerenja s SGP30 mjerenjima s BME680
    }
  if (sgp.IAQmeasure())
  {
     sgp.IAQmeasureRaw();
      if(c==0)//samo prvi prolazak kroz petlju(nakon resetiranja arduina)postavi baseline vrijednosti
    {
      
  
    sgp.setIAQBaseline(co2b, vocb);  
    Serial.println("Postavljene referentne vrijednosti");
    c=1;
    }
    brojac_baseline++;
    if (brojac_baseline == 20)//svakih sat vremena očitaj baseline ako je interval 360s
    {
      Serial.println("Dohvati nove referente vrijednosti(nakon sat vremena)");
      brojac_baseline = 0;
      if (!sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) 
      {
      }
      else 
      {
        Serial.println("Prave vrijednosti");
      Serial.println("CO2 BAZA");
      Serial.println(eCO2_base);
       Serial.println("VOC BAZA ");
      Serial.println(TVOC_base);
      sgp.IAQmeasure();
      
      }
      } 

    }
   slanje_vrijednosti();
   }
}

uint32_t getAbsoluteHumidity(float temperature, float humidity) 
{//dobivanje apsolune vlažnosti potrebne za SGP korekciju mjerenja 
 // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
 const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
 const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
 return absoluteHumidityScaled;
}

void connectToWifi() 
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
void onWifiConnect(const WiFiEventStationModeGotIP& event) 
{
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) 
{
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() 
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) 
{
  Serial.println("Connected to MQTT.");
  //Serial.print("Session present: ");
  //Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_CO2_BASE_PRETPLATA, 2);//mqtt pretplata na teme 
  uint16_t packetIdSub2 = mqttClient.subscribe(MQTT_VOC_BASE_PRETPLATA, 2);
}
  void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) 
{
 if(b<2)//odradi samo prva dva prolazka(ukoliko se arduino resetira)
 {     
 if(a==1)//Uzmi drugu vrijednost dobivenu mqtt-om i spremi je u voc_baza
 voc_baza=payload;
 if(a==0) //Uzmi prvu vrijednost dobivenu mqtt-om i spremi je u co2_baza
 co2_baza=payload;          
                   
 a=!a;   
 co2b=co2_baza.toInt();//Pretvorba stringa dobivenim mqtt-om u int
 Serial.println("co2b");
 Serial.println(co2b);
 vocb=voc_baza.toInt();
 Serial.println("vocb");
 Serial.println(vocb);
 b++;
 }    
}
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) 
{
  //Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) 
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}





void slanje_vrijednosti() 
{
    if(temp!=temp_check)
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str()); 
    temp_check=temp;
    if((hum!=hum_check)||(hum==100))                     
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str()); 
    hum_check=hum;   
    if(pres!=pres_check)                        
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_PRES, 1, true, String(pres).c_str()); 
    pres_check=pres;  
    if ((sgp.eCO2!=eCO2_check)||(sgp.eCO2==400))                        
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_CO2, 1, true, String(sgp.eCO2).c_str());  
    eCO2_check=sgp.eCO2;  
    if ((sgp.TVOC!=TVOC_check)||(sgp.TVOC==0))                        
    uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_VOC, 1, true, String(sgp.TVOC).c_str());
    TVOC_check=sgp.TVOC;
    if(eCO2_base!=0)                     
    uint16_t packetIdPub6 = mqttClient.publish(MQTT_PUB_CO2_base, 1, true, String(eCO2_base).c_str());    
    if(TVOC_base!=0)                         
    uint16_t packetIdPub7 = mqttClient.publish(MQTT_PUB_VOC_base, 1, true, String(TVOC_base).c_str());                               
    if (radsolar!=radsolar_check)  
    uint16_t packetIdPub8 = mqttClient.publish(MQTT_PUB_SOLAR, 1, true, String(radsolar).c_str());
    radsolar_check= radsolar;                         
    uint16_t packetIdPub12 = mqttClient.publish(MQTT_PUB_WIFI_SIGNAL, 1, true, String(WiFi.RSSI()).c_str()); 
    Serial.println("Poslane MQTT poruke");       
}
