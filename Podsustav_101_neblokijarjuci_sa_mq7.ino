#include "TimerOne.h"                     // Timer Interrupt set to 2 second for read sensors 
#include <math.h> 
#include <PubSubClient.h>
#include <YunClient.h>
#include <MQUnifiedsensor.h>
#include <Bridge.h>
#define MQTT_PUB_BRZINA_VITRA "ind/brzinavitra"
#define MQTT_PUB_SMJER_VITRA "ind/smjervitra"
#define MQTT_PUB_OPIS_BRZINE_VITRA "ind/opisvjetra"
#define MQTT_PUB_OPIS_SMJERA_VITRA "ind/smjervjetra"
#define MQTT_PUB_CO "ind/CO"
#define MQTT_PUB_WIFI "ind/wifi"

#define placa "Arduino Industrijal 101"
#define Voltage_Resolution 5
#define pin A1 //Analog input 0 of your arduino
#define type "MQ-7" //MQ7
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ7CleanAir 27.5 //RS / R0 = 27.5 ppm 
#define mosfet 5 // Pin connected to mosfet
#define WindSensorPin (2)                 // Pin na koji je priključen senzor brine vjetra 
#define WindVanePin (A0)                  // Pin na koji je priključen senzor smjera vjetra
#define VaneOffset -180;                  // Pomak  u odnosu na pravi sjever

  int c=0,d=0,g=0;

IPAddress server(192, 168, 1, 14);
YunClient ethClient;
PubSubClient client(ethClient);

int VaneValue;                            // Vrijenost analognon signala smjera vjetra
int Direction;                            // Za translatiranje 0-360 stupnjeva 
int CalDirection;                         // Vrijednost sa uračunatim pomakom
int LastValue;                            // Vrijednost zadnjeg smjera

volatile bool IsSampleRequired;           // Svakih 2.5 sekunda se postavi na 1 Get wind speed 
volatile unsigned int TimerCount;         // Za odeređivanje 2.5 sekunde brojanja timera 
volatile unsigned long Rotations;         // Brojač rotacije korištem u rutini s prekidima 
volatile unsigned long ContactBounceTime; // Timer za izbjegavanje dodirivanja kontakata u isr 

float WindSpeed; // Brzina, milje na sat 

unsigned long oldTime = 0;
const char* opisVjetra; 
const char* smjerVjetra;
long lastReconnectAttempt = 0, vrime=0;
bool b=0, a=0;
float COppm, COppm_check;;
int CalDirection_check;
MQUnifiedsensor MQ7(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);


void setup() 
{ 
 LastValue = 0; 
 IsSampleRequired = false; 
 TimerCount = 0; 
 Rotations = 0;// Postavljanje rotacija na 0 za dalju kalkulaciju
 Serial.begin(9600); 
 Bridge.begin();
 client.setServer(server, 1883);
 //client.setCallback(callback);

 lastReconnectAttempt = 0;

 pinMode(mosfet, OUTPUT);
 analogWrite(mosfet,255);//postaviti napajanje senzora na 0V

//Set math model to calculate the PPM concentration and the value of constants
 MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
 MQ7.setA(99.042); MQ7.setB(-1.518); // Configurate the ecuation values to get CO concentration
/*
    Exponential regression:
  GAS     | a      | b
  H2      | 69.014  | -1.374
  LPG     | 700000000 | -7.703
  CH4     | 60000000000000 | -10.54
  CO      | 99.042 | -1.518
  Alcohol | 40000000000000000 | -12.35
  */
  
  /*****************************  MQ Init ********************************************/ 
  //Remarks: Configure the pin of arduino as input.
  /************************************************************************************/ 
 MQ7.init(); 
  
    //If the RL value is different from 10K please assign your RL value with the following method:
 MQ7.setRL(10);
 
  /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
  // In this routine the sensor will measure the resistance of the sensor supposing before was pre-heated
  // and now is on clean air (Calibration conditions), and it will setup R0 value.
  // We recomend execute this routine only on setup or on the laboratory and save on the eeprom of your arduino
  // This routine not need to execute to every restart, you can load your R0 if you know the value
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
/*  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ7.update(); // Update data, the arduino will be read the voltage on the analog pin
    calcR0 += MQ7.calibrate(RatioMQ7CleanAir);
    Serial.print(".");
  }
  MQ7.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
 MQ7.setR0(5.76);
 MQ7.serialDebug(true);
  

  
 pinMode(WindSensorPin, INPUT); 
 attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); 
 Timer1.initialize(500000);              //Interupt timer svakih 2,5s
 Timer1.attachInterrupt(isr_timer); 
} 

void loop() 
{ 
 getWindDirection(); 
 // Osvježi vrijednosti samo ako je razlika vrijednosti veća od 2 stupnja 
 if(abs(CalDirection - LastValue) > 2) 
    LastValue = CalDirection; 
 
 if(IsSampleRequired) 
   { 
    // Pretvorba u milje na sat V=P(2.25/T) 
    // V = P(2.25/2.5) = P * 0.9 
    WindSpeed = Rotations * 0.9; 
    Rotations = 0; // resetira brojač za sljedeće brojanje
    IsSampleRequired = false; 
 /* Serial.print(WindSpeed); Serial.print("\t\t"); 
    Serial.print(getms(WindSpeed)); Serial.print("\t"); 
    Serial.print(CalDirection); 
    Serial.print("\t\t"); */
    getHeading(CalDirection); 
    getWindStrength(WindSpeed); 
  }

 if (!client.connected()) 
  { 
    long now = millis();
    if (now - lastReconnectAttempt > 5000) 
     {
      Serial.println("Pokusaj spajanja na MQTT"); 
      lastReconnectAttempt = now;
      // pokušaj ponovnog spajanja
      if (reconnect()) 
         lastReconnectAttempt = 0;
     }
  } 
else 
  { // Client connected
   client.loop();
   if (millis() - vrime > (360000))
    {
     slanje_vrijednosti();
     vrime=millis();
    }
   }

  // 90s cycle, 1.8V
 if (millis() - oldTime >= (450000))//poništiti vrijeme
        oldTime = millis();
 if (millis() - oldTime <= (90000))//90 sekundi s naponom 1.4V
     {
      analogWrite(mosfet,84);
     if(g==0)
      Serial.println("Početak ciklusa od 1.4V 90 sekundi");
      g++;
     // 60s cycle
     int d=0;}
     
 if(millis() - oldTime <= (151000)&&(millis() - oldTime >= (90000)))//60 sekundi s naponom od 5V
  { 
    if(d==0)
    Serial.println("Početak ciklusa od 5V 60 sekundi");
    d=1;
    analogWrite(mosfet, 255); // 255 is DC 5V output
    if(millis() - oldTime < (151000)&&(millis() - oldTime >= (150000))&&(b==0))
    // sekundu iza očitaj vrijednost 
      {
       // Serial.println("istek 60s");
        b=1;
        c=0;
        Serial.println("očitavanje vrijednosti sa senzora");
        MQ7.update();
        COppm={MQ7.readSensor()};  
             }
   }

     
  if(millis() - oldTime <= (450000)&&(millis() - oldTime > (151000))&&(c==0))// ugasi senzor
  { 
    c++;
    d=0;
    g=0;
    Serial.println("Gašenje senzora 299 sekundi");
    b=0;//postavi zastavicu na 0
   analogWrite(mosfet, 0);// ugasi napajanje senzora 
  }
}

// isr handler for timer interrupt 
void isr_timer() 
{ 
  TimerCount++; 
  if(TimerCount == 6) 
  { 
    IsSampleRequired = true; 
    TimerCount = 0; 
  } 
} 

// This is the function that the interrupt calls to increment the rotation count 
void isr_rotation() 
{ 
  if((millis() - ContactBounceTime) > 15 ) 
  { // debounce the switch contact. 
   Rotations++; 
   ContactBounceTime = millis(); 
  } 
} 

// Convert MPH to Knots 
float getms(float speed) 
{ 
  return speed * 0.44704; 
} 

// Get Wind Direction 
void getWindDirection() 
{ 
  VaneValue = analogRead(WindVanePin); 
  Direction = map(VaneValue, 0, 1023, 0, 359); 
  CalDirection = Direction + VaneOffset; 
  
  if(CalDirection > 360) 
    CalDirection = CalDirection - 360; 
  
  if(CalDirection < 0) 
    CalDirection = CalDirection + 360; 
} 

// Converts compass direction to heading 
void getHeading(int direction) { 
  if(direction > 337 and direction <= 22) 
    smjerVjetra="Sjeverni"; 
  else if (direction > 22 and direction <= 67) 
    smjerVjetra="Sjeverno - Istočni"; 
  else if (direction > 67 and direction <= 112) 
   smjerVjetra="Istočni"; 
  else if (direction > 112 and direction <= 157) 
    smjerVjetra="Južno - Istočni"; 
  else if (direction > 157 and direction <= 202) 
   smjerVjetra="Južni"; 
  else if (direction > 202 and direction <= 247) 
   smjerVjetra="Južno - Zapadni"; 
  else if (direction > 247 and direction <= 292) 
    smjerVjetra="Zapadni"; 
  else if (direction > 292 and direction <= 337) 
    smjerVjetra="Sjeverno - Zapadni"; 
  else 
    smjerVjetra="Sjeverni"; 
} 

// converts wind speed to wind strength 
void getWindStrength(float speed) { 

  if(speed < 0.3) 
  opisVjetra="Tiho";
   else if(speed >= 0.3 && speed < 1.6) 
   opisVjetra="Lahor"; 
  else if(speed >= 1.6 && speed < 3.4) 
    opisVjetra="Povjetarac";
  else if(speed >= 3.4 && speed < 5.5) 
    opisVjetra="Slab"; 
  else if(speed >= 5.5 && speed < 8) 
    opisVjetra="Umjeren"; 
  else if(speed >= 8 && speed < 10.8) 
   opisVjetra="Umjereno jak"; 
  else if(speed >= 10.8 && speed < 13.9) 
   opisVjetra="Jak"; 
  else if(speed >= 13.9 && speed < 17.2) 
    opisVjetra="Žestok"; 
  else if(speed >= 17.2 && speed < 20.8) 
   opisVjetra="Olujan"; 
  else if(speed >= 20.8 && speed < 24.5) 
  opisVjetra="Jak olujan"; 
  else if(speed >= 24.5 && speed < 28.5) 
    opisVjetra="Orkanski"; 
  else if(speed >= 28.5 && speed < 32.7) 
   opisVjetra="Jak orkanski"; 
  else if(speed > 32.7) 
   opisVjetra="Orkan"; 
}
/*void callback(char* topic, byte* payload, unsigned int length) 
{
  // handle message arrived
}*/
boolean reconnect() {
  if (client.connect("arduinoClient", "adi", "pagtrg40")) {
    Serial.println("Spojen na MQTT");
    // Once connected, publish an announcement...
   // client.publish("smjervitra","58");
    // ... and resubscribe
  }
  return client.connected();
}

void slanje_vrijednosti() 
{
   int str_len = wifiquality().length() + 1;
   char wifistring[str_len];
   wifiquality().toCharArray(wifistring, str_len);
   //Serial.println(wifistring);
   char tempString[8];
   dtostrf(getms(WindSpeed), 1, 2, tempString );
   char smjerString[8];
   dtostrf(CalDirection, 1, 2, smjerString);   
   char COstring[8];
   dtostrf(COppm, 1, 2, COstring);  
   client.publish(MQTT_PUB_BRZINA_VITRA, tempString);
   if(CalDirection!=CalDirection_check)
   {
   client.publish(MQTT_PUB_SMJER_VITRA, smjerString);
   CalDirection_check=CalDirection;   
   }
   client.publish(MQTT_PUB_OPIS_BRZINE_VITRA, opisVjetra);
   client.publish(MQTT_PUB_OPIS_SMJERA_VITRA, smjerVjetra);
   if(COppm!=COppm_check)
   {
   client.publish(MQTT_PUB_CO, COstring);
   COppm_check=COppm;}
   client.publish(MQTT_PUB_WIFI,wifistring);
   Serial.println("Poslane MQTT poruke"); 
}
String wifiquality ()
{
 Process wifiCheck;  // initialize a new process
 wifiCheck.runShellCommand("/usr/bin/wifi-info.lua");  // command you want to run
 String parse ;
 while (wifiCheck.available() > 0) 
 {
   parse = parse + String(char(wifiCheck.read()));
 }
  //   Serial.println(parse);
 return parse;
}
