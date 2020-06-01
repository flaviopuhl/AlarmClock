/*
* PROGRAM :        AlarmClock.c             
* 
* DESCRIPTION :
*       Alarm clock with time update from internet
*
* NOTES :
*       Suporte Nokia5110.............[101]
*       Suporte NTP...................[101]
*       Suporte WiFi/OTA..............[102]
*       Suporte Interrupção ..........[103b]
*       Suporte EEPROM ...............[104]
*       Suporte Buzzer................[TBD]
*       

NodeMCU         ESP8266      Nokia 5110    Description
D2              (GPIO4)       0 RST         Output from ESP to reset display
GND                           1 CE          Output from ESP to chip select/enable display >> Connected to GND
D6              (GPIO12)      2 DC          Output from display data/command to ESP
D7              (GPIO13)      3 Din         Output from ESP SPI MOSI to display data input
D5              (GPIO14)      4 Clk         Output from ESP SPI clock
3V3                           5 Vcc         3.3V from ESP to display
D0              (GPIO16)      6 BL          3.3V to turn backlight on, or PWM
G                             7 Gnd         Ground


Dependencies:
https://github.com/adafruit/Adafruit-GFX-Library
https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
- This pull request adds ESP8266 support:
- https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library/pull/27
https://github.com/taranais/NTPClient

References
https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
*/

/************************************************************************
* Libraries
************************************************************************/
#include <ESP8266WiFi.h>                    //  WiFi
#include <ESP8266mDNS.h>                    //  OTA
#include <WiFiUdp.h>                        //  OTA
#include <ArduinoOTA.h>                     //  OTA
#include <NTPClient.h>                      //  NTP
#include <Arduino.h>
#include <WiFiClient.h>                     //  WiFi
#include <SPI.h>                            //  SPI
#include <Adafruit_PCD8544.h>               //  Nokia 5110 - ESP8266 version
#include <Adafruit_GFX.h>
#include <EEPROM.h>                         // EEPROM


/************************************************************************
* Define pinout and constants
************************************************************************/ 
const int8_t RST_PIN = D2;
const int8_t CE_PIN = D1;
const int8_t DC_PIN = D6;
//const int8_t BL_PIN = D0;                 // Removed to save pins

const int8_t H_INC_PIN = D3;                // needs physical debounce (RC)
const int8_t M_INC_PIN = D4;                // needs physical debounce (RC)

const int8_t ALARM_PIN = D8;

int alarm_hour = 0;
int alarm_minu = 0;

const long utcOffsetInSeconds = -10800;
int Wifi_statusRecover=0;

int EEPROM_addr_hour = 0;
int EEPROM_addr_minute = 01;

bool running = false;

/************************************************************************
* WiFi connection parameters and constants
************************************************************************/
const char* myssid = "CasaDoTheodoro";
const char* mypass = "09012011";


/************************************************************************
* Define NTP client to get time
************************************************************************/
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", utcOffsetInSeconds);

// Hardware SPI based on hardware controlled SCK (SCLK) and MOSI (DIN) pins. CS is still controlled by any IO pin.
// NOTE: MISO and SS will be set as an input and output respectively, so be careful sharing those pins!
Adafruit_PCD8544 display = Adafruit_PCD8544(DC_PIN, CE_PIN, RST_PIN);

/************************************************************************
* Interrupt Routines
************************************************************************/

ICACHE_RAM_ATTR void InterruptAlarmHourIncrease(){
  alarm_hour++;
  if(alarm_hour>=24){
    alarm_hour=0;
  }
  Serial.println("Alarm Hour change");
  EEPROM.write(EEPROM_addr_hour, alarm_hour);
  EEPROM.commit();
}

ICACHE_RAM_ATTR void InterruptAlarmMinuteIncrease(){
  alarm_minu++;
  if(alarm_minu>=60){
    alarm_minu=0;
  }
  Serial.println("Alarm Minute change");
  EEPROM.write(EEPROM_addr_minute, alarm_minu);
  EEPROM.commit();
}


void setup(void) {
  Serial.begin(9600);
  EEPROM.begin(10);

/************************************************************************
* Hardware setup
************************************************************************/
//pinMode(BL_PIN, OUTPUT); //backlight

pinMode(H_INC_PIN, INPUT_PULLUP ); //Button Alarm hour increase
  attachInterrupt(H_INC_PIN, InterruptAlarmHourIncrease, RISING);
  
pinMode(M_INC_PIN, INPUT_PULLUP ); //Alarm minute increase
  attachInterrupt(M_INC_PIN, InterruptAlarmMinuteIncrease, RISING);  

pinMode(ALARM_PIN, INPUT_PULLUP ); //Button to active/deactivate alarm
  
/************************************************************************
* Setup
************************************************************************/ 

  // Configure LCD
  display.begin();
  //digitalWrite(BL_PIN, HIGH);  // Turn LCD backlight on
  display.clearDisplay();
  display.setContrast(55);  // Adjust for your display

  
  //Tela inicial
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Alarm Clock");
  Serial.println("\nESP8266 WiFi alarm clock");

  display.display();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,10);
  display.println("Connecting");
  display.display();
  
  // Get date & time from web
  WiFi.begin(myssid, mypass);

    display.setCursor(0,20);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    display.print(".");
    display.display();
  
    Wifi_statusRecover++;
      if (Wifi_statusRecover>=20){
        Serial.println ( "#" );
        display.print("#");
        display.display();
        ESP.restart();
        }
  }

    /* ensure WIFI_STA mode */
  Serial.print("Current WiFi mode = ");
  Serial.println(WiFi.getMode());
  Serial.print("Set to WIFI_STA mode = ");
  WiFi.mode(WIFI_STA); // WIFI_STA = 1
  Serial.println(WiFi.getMode());
  
  Wifi_statusRecover=0;
  timeClient.begin();

  //Recall last alarm set
  alarm_hour = EEPROM.read(EEPROM_addr_hour);
  alarm_minu = EEPROM.read(EEPROM_addr_minute);
  display.println("EEPROM recovered");
  
  display.clearDisplay();
  
/************************************************************************
* OTA Functions - do not remove
************************************************************************/ 
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
      display.clearDisplay();          
      display.setTextSize(1);
      display.setCursor(0,0);
      display.print("OTA Start");
      display.display();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");        
      display.setTextSize(1);
      display.setCursor(0,30);
      display.print("OTA End");
      display.display();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    display.setTextSize(1);
      display.setCursor(0,20);
      display.print("In Progress");
      display.display();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}// end void setup()


/************************************************************************
* Main Loop
************************************************************************/
void loop() {

/************************************************************************
* Get Date&Time
************************************************************************/

timeClient.update();

String formattedDate = timeClient.getFormattedDate(); //2020-03-27T15:09:40Z
Serial.print(formattedDate);
Serial.print(" ");

// Extract date
//int splitT = formattedDate.indexOf("T");
//String dayStamp = formattedDate.substring(0, splitT);
String data_ano = formattedDate.substring(0, 4);
String data_mes = formattedDate.substring(5, 7);
String data_dia = formattedDate.substring(8, 10);


if(data_mes == "01") {data_mes = "JAN";}
if(data_mes == "02") {data_mes = "FEV";}
if(data_mes == "03") {data_mes = "MAR";}
if(data_mes == "04") {data_mes = "ABR";}
if(data_mes == "05") {data_mes = "MAI";}
if(data_mes == "06") {data_mes = "JUN";}
if(data_mes == "07") {data_mes = "JUL";}
if(data_mes == "08") {data_mes = "AGO";}
if(data_mes == "09") {data_mes = "SET";}
if(data_mes == "10") {data_mes = "OUT";}
if(data_mes == "11") {data_mes = "NOV";}
if(data_mes == "12") {data_mes = "DEZ";}


Serial.print(data_ano);
Serial.print(" ");
Serial.print(data_mes);
Serial.print(" ");
Serial.print(data_dia);
Serial.print(" ");


// Extract time
//String timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
String hora_hor = formattedDate.substring(11, 13);
String hora_min = formattedDate.substring(14, 16);

Serial.print(hora_hor);
Serial.print(" ");
Serial.print(hora_min);
Serial.print(" ");


/************************************************************************
* Update display
************************************************************************/

  display.clearDisplay();

  //Data
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(10,0);
  display.println(data_dia);

  display.setCursor(30,0);
  display.println(data_mes);

  display.setCursor(55,0);
  display.println(data_ano);

  //Hora
  display.setTextColor(BLACK);
  display.setTextSize(3);
  
  display.setCursor(0,10); 
  display.println(hora_hor);

  display.setCursor(31,10);
  display.println(":");

  display.setCursor(43,10);
  display.println(hora_min);

  //Alarme
  display.setTextSize(2);
  display.setTextColor(BLACK);
  display.setCursor(0,34);
  display.println(alarm_hour);

  display.setCursor(20,34);
  display.println(":");

  display.setCursor(28,34);
  display.println(alarm_minu);

  if(ALARM_PIN == HIGH){
    display.setTextColor(WHITE, BLACK);
  }
 else{
  display.setTextColor(BLACK);
 }
  //Toggle
  display.setTextSize(2);
  
  display.setCursor(70,34);
     if( WiFi.status() != WL_CONNECTED ) {
        Serial.print ( "Lost WiFi Connection" );
        //display.print(" ");
        display.write(19);
     }
    else{
        running = !running;
        if(running== false){
          display.print(" ");
        }
        else{
        display.write(30);
        }
     }

  display.display();
  
  Serial.println("");
  delay(500);

/************************************************************************
* OTA handles - do not remove
************************************************************************/ 
  if (WiFi.status() == WL_CONNECTED){
    ArduinoOTA.handle();
  }
  
}
