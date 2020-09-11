/*
#include <ESP8266WiFi.h> 
#include <WiFiClientSecure.h> 
#include <TelegramBot.h> 
*/

//LIBRERIE:
#include <stdlib.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WiFi.h> 
#include <ArduinoJson.h>
//SSL Client
#include <WiFiClientSecure.h>
//TEMPERATURA
#include <DallasTemperature.h>
#include <OneWire.h>

// the number of the LED pin
#define LED D0 //led pin number

/*
// Initialize Wifi connection to the router 
const char* ssid     = "cuginet"; 
const char* password = "cugicugimilitari!";
*/

//INIZIALIZZAZIONE BOT TELEGRAM:
#define botToken "997464842:AAGiQt4lHq47Wg-SQm_gOPwjXRS0hDNDrpw"  //Bot Token AcuaBot
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);
long checkTelegramDueTime = 0;
int checkTelegramDelay = 1000; 
int numNewMessages = 0;
String defaultChatId = "196098030"; //chat id Augu(default):  196098030
String chatidNico ="182934371";     //chat id nico:          182934371
String chatidSilvia="325872161";    //chat id Silvia:        325872161

//INIZIALIZZO WIFI:
char ssid[] = "TIM-24612141"; // ACUADORI cuginet
char password[] = "boccino:miaproallachiusura"; // m07f19rN! cugicugi

// Initialize Telegram BOT 
const char BotToken[] = "997464842:AAGiQt4lHq47Wg-SQm_gOPwjXRS0hDNDrpw";
 
WiFiClientSecure net_ssl;
 
TelegramBot bot (BotToken, net_ssl); 
   
void setup()  
{   
  Serial.begin(115200);   
  while (!Serial) {}  //Start running when the serial is open  
  delay(3000);   
  // attempt to connect to Wifi network:   
  Serial.print("Connecting Wifi: ");   
  Serial.println(ssid);   
  while (WiFi.begin(ssid, password) != WL_CONNECTED)  
  {   
    Serial.print(".");   
    delay(500);   
  }   
  Serial.println("");   
  Serial.println("WiFi connected");   
  bot.begin();   
  pinMode(LED, OUTPUT);   
}
   
void loop()  
{   
  message m = bot.getUpdates(); // Read new messages   
  if (m.text.equals("on"))  
  {   
    digitalWrite(LED, 1);    
    bot.sendMessage(m.chat_id, "The Led is now ON");   
  }   
  else if (m.text.equals("off"))  
  {   
    digitalWrite(LED, 0);    
    bot.sendMessage(m.chat_id, "The Led is now OFF");   
  }   
}   
