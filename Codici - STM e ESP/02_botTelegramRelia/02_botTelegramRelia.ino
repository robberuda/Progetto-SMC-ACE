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



//DEFINIZIONE e INIZIALIZZAZIONE INGRESSI E USCITE:
#define pulsante D7
#define wifiled D8
#define led D1
#define sensor D4
int valoreled = 0;

//INIZIALIZZAZIONE TERMOMETRO
OneWire oneWirePin(sensor);
DallasTemperature sensors(&oneWirePin);
int cont=0;
float memoria[100];
float temp = -100;
int intervallotemp = 1800000;
long incrementotemp = 0;
int richiestaQuante =0;



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
char ssid[] = "ACUADORI";  
char password[] = "m07f19rN!";  


//                                        FUNZIONI 
//-------------------------------------------------------------------------------------------------------

  //FUNZIONE CHE AQCUISISCE LA TEMPERATURA:
  float acquisisciTemp (){
    float tempfunz;
    Serial.print("Acquisisco Temperatura:");
    sensors.requestTemperatures();
    tempfunz = sensors.getTempCByIndex(0);
    Serial.println("ACQUISITA");
    return tempfunz;
    }
  
  //FUNZIONE TELEGRAM per mandare messaggi:
  void mandamessaggio(){
    Serial.println("pulsante premuto!");
    bot.sendMessage(defaultChatId, "hai premuto il pulsante!!!");
    bot.sendMessage(chatidNico, "hai premuto il pulsante!!!");
    delay (500);
    }

  //FUNZIONE CHE RIEMPE IL BUFFER DELLE TEMPERATURE:
  void store(float temperatura){
  if (cont<100)
  {
    if (cont == 99 ) cont =0;
    memoria[cont]=temperatura;
    cont++;
    }
  }
  

  //FUNZIONE CHE MANDA LE TEMPERATURE
  void mandaTemperature(int quante, String chat_id){
    bot.sendMessage(chat_id, "Ecco a te le ultime " + String(quante) + " temperature, le mando dall'ultima che ho registrato, ti prego di aspettare finchè non le avrò mandate tutte, ci vorrà solo qualche istante:");
    int indice=(cont-1);
    int orario = 0;
    int i=1;
    while (quante > 0){
      bot.sendMessage(chat_id, "" +String(i)+ "° temper. , " +String(orario)+ "min fa: " +String(memoria[indice])+ "°C");
      quante--;
      if (indice == 0) indice =99;
        else indice --;
      orario = orario + (1800000/60000);
      i++;
      }
    
    }

  //FUNZIONE PER MANDARE IL MESSAGGIO DEI COMANDI:
  void mexhelp(String chat_id ){
    bot.sendMessage(chat_id, "Salve, io sono il bot che controlla le temperature, ne immagazino una ogni mezzora. Di seguito puoi trovare la lista dei comandi che puoi utilizzare:\n-LISTA COMANDI-\n /help \n /led_on o 'led on' \n /led_off o 'led ff' \n /temperatura (manda l'ultima) \n /temperature (manda più temp.)\n /FammiUnCaffe");
    Serial.println("il bot ha mandato la lista dei comandi...");
    }

  //FUNZIONE TELEGRAM per riconoscere i messaggi:
  void leggimessaggio(int numNewMessages){
  Serial.println("------------IL BOT HA RICEVUTO UN MESSAGGIO:");
  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Anonimo";
    Serial.print(">");
    Serial.print(from_name);
    Serial.print(": ");
    Serial.println(text);

    //  fammi un caffè
    if (text ==  "/FammiUnCaffe") {
      bot.sendMessage(chat_id, "hahahaha ma secondo te so fare il caffè!?!?!?!");
      bot.sendMessage(chat_id, "per ora sono stupido...so solo leggere temperature");
    }

    //  richiesta temperature varie:
    if ( (text ==  "temperature") || (text ==  "Temperature") || (text ==  "TEMPERATURE") || (text ==  "/temperature") ){
      bot.sendMessage(chat_id, "Quante temperature vuoi vedere??(ricorda che tra ogni temperatura passa mezzora)");
      richiestaQuante=1;
      }
    if ( (text.toInt() < 100 ) && (text.toInt() > 0) && (richiestaQuante ==1)){
      richiestaQuante = 0;
      int numero = text.toInt();
      mandaTemperature(numero, chat_id);
      }
      else if ((text.toInt() > 100 ) || (text.toInt() < 0) && (richiestaQuante ==1)){
        bot.sendMessage(chat_id, "La cifra inserita non è valida");
        richiestaQuante = 0;
        } 
        
    //  richiesta temperatura:
    if ( (text ==  "temperatura") || (text ==  "Temperatura") || (text ==  "TEMPERATURA") || (text ==  "/temperatura") ){
      float tempaus=acquisisciTemp();
      bot.sendMessage(chat_id, "L'ultima temperatura è di " + String(tempaus) + "°C");
      Serial.println("il bot ha mandato la temperatura...");  
      }
    
    // messaggio di menù:
    if ( (text ==  "menu") || (text ==  "Menu") || (text ==  "MENU") || (text ==  "/menu") || (text ==  "/help") || (text ==  "Comandi") || (text ==  "comandi") || (text ==  "/start") || (text ==  "ciao") || (text ==  "Ciao") )  {
      mexhelp(chat_id);
      } 
    
    //  se il led è spento:
    if ( ((text ==  "led on") || (text ==  "Led on") || (text ==  "LED ON") || (text ==  "/led_on") ) && (valoreled == 1) ){ 
      bot.sendMessage(chat_id, "il LED è gia acceso!");
      Serial.println("il LED è gia acceso!");      
    }
    if ( ((text ==  "led on") || (text ==  "Led on") || (text ==  "LED ON") || (text ==  "/led_on") ) && (valoreled == 0) ) {
      digitalWrite( led , HIGH);
        valoreled=1;
        bot.sendMessage(chat_id, "il LED è acceso!");
    }
    
    // se il led é acceso:
    if ( ((text ==  "led off") || (text ==  "Led off") || (text ==  "LED OFF") || (text ==  "/led_off") ) && (valoreled == 0) ){
      bot.sendMessage(chat_id, "il LED è gia spento!");
      Serial.println("il LED è gia spento!");
    }
    if ( ((text ==  "led off") || (text ==  "Led off") || (text ==  "LED OFF") || (text ==  "/led_off") ) && (valoreled == 1) ) {
        digitalWrite( led , LOW);
        valoreled=0;
        bot.sendMessage(chat_id, "il LED è spento!");
    }
    


  
    if (text == "/tastiera") {
      String keyboardJson = "[[\"/led on\", \"/led off\"]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Usa i tasti rapidi", "", keyboardJson, true);
    }
    
  }
  }



//                               VOID  SETUP  
//--------------------------------------------------------------------------------------------------
void setup() {
  //APRO IL MONITOR SERIALE:
  Serial.begin(9600);

  //APRO LA RICEZIONE AL SENSORE
  sensors.begin();
  
  //INIZIALIZZO INGRESSI E USCITE:
  pinMode(pulsante, INPUT);
  pinMode(wifiled, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(sensor, INPUT);

  //CONNESSIONE AL ROUTER
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(wifiled, HIGH);
    delay(250);
    digitalWrite(wifiled, LOW);
    delay(250);
  }
  if(WiFi.status() == WL_CONNECTED) 
    digitalWrite(wifiled, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);


  // messaggio di avvio:
  bot.sendMessage(defaultChatId, "La scheda è stata avviata!");
  mexhelp(defaultChatId);
  
}



//                                        VOID  LOOP
//------------------------------------------------------------------------------------------------------------------------
void loop() {

 //SE VIENE PREMUTO IL PULSANTE:
  if(digitalRead(pulsante) == HIGH) mandamessaggio();

 //SE VIENE RICEVUTO UN MESSAGGIO:
  long now = millis();
  if(now >= checkTelegramDueTime) {
    Serial.println("---- Checking Telegram -----");
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);        //quanti messaggi ricevuti
    while(numNewMessages) {
      leggimessaggio(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    checkTelegramDueTime = now + checkTelegramDelay;
  }

  //contatore di cicli loop:
  long contatore = millis();
  if(contatore >= incrementotemp) {
    temp = acquisisciTemp();
    store(temp);
    if(temp >= 25.00) {
      Serial.print("LA TEMPERATURA VALE:");
      Serial.print(temp);
      Serial.println("°C");
      bot.sendMessage(defaultChatId, "Occhio la temperatura ha superato i 25°C!!Ora è di " + String(temp) + "°C");
      bot.sendMessage(chatidNico, "Occhio la temperatura ha superato i 25°C!!Ora è di " + String(temp) + "°C");
    }
    incrementotemp = contatore + intervallotemp;
  }
  
}
