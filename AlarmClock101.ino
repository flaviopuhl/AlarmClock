/*
* PROGRAM :        AlarmClock.c             
* 
* DESCRIPTION :
*       Alarm clock with time update from internet
*
* NOTES :
*       Suporte Nokia5110.............[101]
*       Suporte WiFi/OTA..............[TBD]
*       Suporte NTP...................[101]
*       Suporte Buzzer................[TBD]
*       Suporte WiFi Signal Strength..[TBD]
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


/************************************************************************
* Define pinout and constants
************************************************************************/ 
const int8_t RST_PIN = D2;
const int8_t CE_PIN = D1;
const int8_t DC_PIN = D6;
const int8_t BL_PIN = D0;

long rssi;
int8_t graph[83];
uint8_t i, col, pos = 0;
bool scroll = false;

const long utcOffsetInSeconds = -10800;
int Wifi_statusRecover=0;

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

void setup(void) {
  Serial.begin(9600);

/************************************************************************
* Hardware setup
************************************************************************/
pinMode(BL_PIN, OUTPUT); //backlight

  
  
  
  digitalWrite(BL_PIN, HIGH);  // Turn LCD backlight on

  // Configure LCD
  display.begin();
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
        ESP.restart();}
  }
  Wifi_statusRecover=0;
  timeClient.begin();
  
  display.clearDisplay();
  
}

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
  display.println("06");

  display.setCursor(20,34);
  display.println(":");

  display.setCursor(28,34);
  display.println("20");

  // toggle
  display.setTextSize(2);
  display.setTextColor(BLACK);
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
  delay(1000);
}
