#include <TM1637Display.h>
#include <Ticker.h>
#include <FS.h>

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266mDNS.h>

#include <EEPROM.h>
//#include <WiFiUdp.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
  

float Humidity = 0;
float Temperature = 0;
float Pressure = 0;

int TempCorr = 0;
//uint8_t TCOK = 0; 


unsigned long dhtDisplay = 0;
bool readDHT = false;

const char* host = "esp8266";
const char* ssid = "asushotyn";
const char* password = "hotyn777";

#define LED   D4 //свінтусдіод на Wemos

int LightSensorPin = A0;    // select the input pin for the potentiometer
int LightSensorValue = 0;  // variable to store the value coming from the sensor
int Brightness = 0;
//~10 kOhm resistors!!!

WiFiUDP ntpUDP;

#include <ezTime.h>

int8_t TZ = 3;
//uint8_t TZOK = 0; 
int TZaddressEEOK = 0; // check if this == 88
int TZaddressEE = 1;

int TCaddressEEOK = 2; // check if this == 88
int TCaddressEE   = 3;

String Name = "";
int NameAddressEE = 4;


ESP8266WebServer server(80);

void returnOK(String msg="OK\r\n" );

// GPIO#0 is for Adafruit ESP8266 HUZZAH board. Your board LED might be on 13.
const int LEDPIN = LED;


// Module connection pins (Digital Pins)
#define CLK1 D6 
#define DIO1 D5  

// Module connection pins (Digital Pins)
#define CLK2 D0  
#define DIO2 D7  



// The amount of time (in milliseconds) between tests
#define TEST_DELAY   500
#define DOTS_DELAY   500
#define SEC_DELAY   1000

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
  };

TM1637Display display(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);


Ticker flipper;
Ticker pollinput;
Ticker NTPUpdater;
Ticker DHTUpdater;
Ticker DisplayOrderer;

bool ntpGood = false;
int updateFails = 0;
int updPeriod = 10;

unsigned long hours = 0;
unsigned long minutes = 0;
unsigned long seconds = 0;

const char O_RTC = 0;
const char O_DHT = 1;
const char O_PRESS = 2; 

enum Order_t {o_rtc,o_dhc} ; 
uint8_t printOrder = O_DHT;
uint8_t displayOrder = O_DHT;

//char timeColor[7] = "00FF00";

enum xColors {
   red,
   green,
   yellow
  } timeColor;


bool dots = false;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete


void _streamFile(char* fn, char* content_type = "text/html") {
  File file = SPIFFS.open(fn, "r");
  if (!file) {
      Serial.println("file open failed");
  }

  server.streamFile(file, content_type);

  file.close();
  
}
  

void handleAuth(){
  
  return; //disable Auth
    
}

void handleRoot()
{
  handleAuth();
  
  if (server.hasArg("Name") || server.hasArg("TZ") || server.hasArg("TempCorr") || server.hasArg("TimeH") || server.hasArg("TimeM")) {
    handleSubmit();
  }
  else {
    _streamFile("/index.html");
    //server.send(200, "text/plain", "hello from esp8266!");
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit()
{
  if (server.arg("Name")!="" ) {
    String NameStr = server.arg("Name"); 
    
     setName(NameStr);
     returnOK(); 
    
  }
  
  int tz = 0 ;
  if (server.arg("TZ")!="" ) {
    String tzstr = server.arg("TZ"); 
    tz = tzstr.toInt();
    if ((tz>=-12) && (tz<=12)) {
     setTZ(tz);
      returnOK(); 
    }else {
      returnOK("TZ out of range!\r\n"); 
      }
    
  }
  
  int tc = 0;
  if (server.arg("TempCorr")!="" ) {
    String tcstr = server.arg("TempCorr"); 
    Serial.println("tcstr = " + tcstr );
    tc = tcstr.toInt();
    if ((tc>=-12) && (tc<=12)) {
     setTempCorr(tc);
      returnOK(); 
    }else {
      returnOK("Temp Corr out of range!\r\n"); 
      }
  }
  
  int th = 0;
  int tm = 0;
   if ( (server.arg("TimeH")!="") && (server.arg("TimeM")!="")) {
    String thstr = server.arg("TimeH"); 
    th = thstr.toInt();
    String tmstr = server.arg("TimeM"); 
    tm = tmstr.toInt();
    if ((th>=0) && (th<=23) && (tm>=0) && (tm<=59) ) {
     hours = th;
     minutes = tm;
      returnOK(); 
    }else {
      returnOK("Temp Corr out of range!\r\n"); 
      }
  }
  
}

void handleFavicon () {
  _streamFile("/favicon.ico", "image/ico"); 
}

void handleJquery () {
  _streamFile("/jquery-3.4.1.min.js", "application/javascript"); 
}



void returnOK(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", msg);
}

/*
 * Imperative to turn the LED on using a non-browser http client.
 * For example, using wget.
 * $ wget http://esp8266webform/ledon
 */
void handleLEDon()
{
  handleAuth();
  writeLED(true);
  server.send(200, "text/plain", "on");
 }

/*
 * Imperative to turn the LED off using a non-browser http client.
 * For example, using wget.
 * $ wget http://esp8266webform/ledoff
 */
void handleLEDoff()
{
  handleAuth();
  writeLED(false);
  server.send(200, "text/plain", "off");
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void writeLED(bool LEDon)
{
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon)
  {
    digitalWrite(LEDPIN, 0);
  } else {
    digitalWrite(LEDPIN, 1);
  }
}

void notFound() {
    server.send(404, "text/plain", "Not found");
}

void changeDisplayToDHT(){
    displayOrder = O_DHT;
    DisplayOrderer.detach();
    DisplayOrderer.attach(20,changeDisplayToPRESS);  
    Serial.println("displayOrder = O_DHT");
  }

void changeDisplayToPRESS(){
    displayOrder = O_PRESS;
    DisplayOrderer.detach();
    DisplayOrderer.attach(20,changeDisplayToDHT);  
    Serial.println("displayOrder = O_PRESS");
  }


void countTime(){
	 seconds++;
    if (seconds>59) {
        seconds = 0; minutes++;
          if (minutes>59) {
            minutes=0; hours++;
            if (hours>23) hours = 0;
        }
      }
      //Serial.printf("timeDisplay %d", timeDisplay);
}

void displayOut(){
	//
	//      A
	//     ---
	//  F |   | B
	//     -G-
	//  E |   | C
	//     ---
	//      D
	
	const uint8_t digitToSegment[] = {
	 // XGFEDCBA
	  0b00111111,    // 0
	  0b00000110,    // 1x
	  0b01011011,    // 2
	  0b01001111,    // 3x
	  0b01100110,    // 4x
	  0b01101101,    // 5
	  0b01111101,    // 6x
	  0b00000111,    // 7x
	  0b01111111,    // 8
	  0b01101111,    // 9x
	  0b01110111,    // A
	  0b01111100,    // b
	  0b00111001,    // C
	  0b01011110,    // d
	  0b01111001,    // E
	  0b01110001     // F
	  };


	const uint8_t digitToSegmentUW[] = {
	 // XGFEDCBA
	  0b00111111,    // 0
	  0b00110000,    // 1x
	  0b01011011,    // 2
	  0b01111001,    // 3x
	  0b01110100,    // 4x 0b01100110
	  0b01101101,    // 5
	  0b01101111,    // 6x
	 // XGFEDCBA
	  0b00111000,    // 7x
	  0b01111111,    // 8
	  0b01111101,    // 9x
	  
	  0b01110111,    // A
	  0b01111100,    // b
	  0b00111001,    // C
	  0b01011110,    // d
	  0b01111001,    // E
	  0b01110001     // F
	  };


	unsigned long timeDisplay = hours * 100 + minutes;	
	uint8_t dotPos = 0b00000000;
	if  (dots) {
		dotPos = 0b01100000;
		} 
	if (!ntpGood) {
		dotPos += 0b10000000;
		}
    
   // display.showNumberDecEx(timeDisplay, dotPos, true);
    // Вивести перевернуту цифру в позиції ..:х. 
	if (hours>0){
		const uint8_t segments[4] = {
			digitToSegment[hours/10] + (ntpGood?0b00000000:0b10000000),
			digitToSegment[hours - (hours/10)*10] + (dots?0b10000000:0b00000000),
			digitToSegmentUW[minutes/10] + (dots?0b10000000:0b00000000), 
			digitToSegment[minutes - (minutes/10)*10]
		};
		display.setSegments(segments, 4, 0);
		
	} else {
		const uint8_t segments[4] = {
			digitToSegment[minutes/10] + (ntpGood?0b00000000:0b10000000), 
			digitToSegment[minutes - (minutes/10)*10] + (dots?0b10000000:0b00000000),
			digitToSegmentUW[seconds/10] + (dots?0b10000000:0b00000000), 
			digitToSegment[seconds - (seconds/10)*10], 
		};
		display.setSegments(segments, 4, 0);
	}

    
    
    
}

void display2Out(){
	if (displayOrder == O_DHT) {
		dhtDisplay = int(Humidity) * 100 + int(round(Temperature));	
 		display2.showNumberDecEx(dhtDisplay, (0b01010000), true);
     return;
    }
    
    String PressStr;
  
  int PressInt;
  int8_t dotP = 0b00000000;
  if (Pressure < 1000) {
      PressInt = int(Pressure*10);
      String PressStr = String(int(Pressure*10)) ;
      dotP = 0b00100000;
    }else {
      PressInt = int(Pressure);
      String PressStr = String(int(Pressure));
      dotP = 0b00000000;
    }

  //Serial.printf("P= %.1f", Pressure);
  display2.showNumberDecEx(PressInt, dotP, true);
}

void setBrightness(){
	
	LightSensorValue = analogRead(LightSensorPin);
	float k = float(LightSensorValue)/1024 * 7;
	
	Brightness = 7 - k;
	
	//Serial.printf("Light Val = %d, k = %0.2f, Brigh = %d", LightSensorValue,k, Brightness);
	//Serial.println();
	
	display.setBrightness(Brightness);
    display2.setBrightness(Brightness/2);
	
}
  
void flip() {
	countTime();
	dots = !dots;
	displayOut();
	display2Out();
	setBrightness();
}


void handleStatus(){
  
  char jsonResponce[200] = "";
  char timestr[9] = "" ;
//  char colorstr[7]  = "";
  char ledstr[4] = "";
  char NameBuf[10] = "";
  
  for(int i=0;i<10;i++)
  {
    NameBuf[i] = Name[i]; 
  }
  Serial.println("NameBuf: " + String(NameBuf));

  if (ntpGood) {
    sprintf(timestr, "%02d:%02d:%02d", hours, minutes, seconds);
  } else {
    String err = errorString();
    char strbuf[20]="";
    int char_index = 0;
    for (int i = 0; i < err.length(); ++i) {
      strbuf[char_index++] = err[i];
    }
    sprintf(timestr, "NTP:%s %02d", strbuf, seconds);
  }
  sprintf(jsonResponce, "{\"time\":\"%s\",\"led\":%d, \"timezone\":%d, \"humid\":%.1f, \"temper\":%.1f, \"press\":%.1f, \"TempCorr\":%d, \"Name\":\"%s\" }",  timestr, digitalRead(LED) * 50,TZ, Humidity, Temperature,Pressure,TempCorr, NameBuf);
 
  server.send(200, "text/plain", jsonResponce);

}



void getInput(){
}

void handleWifiReset(){
  ESP.restart();
}




// =====================================================================================
void updateTIME();

void setup()
{
   
   SPIFFS.begin();

 
   pinMode(LED, OUTPUT);
   
   writeLED(false);
   
   Serial.begin(57600);
   delay(1000);
   Serial.println("");
   //Serial1.begin(57600);
   EEPROM.begin(512);
  
  
  for(int i=0;i<10;i++) 
  {
    Name = Name + char(EEPROM.read(NameAddressEE+i)); //Read one by one with starting address of 0x0F    
  }  
    Serial.println("EEPROM Name=" + Name );
    
  if ( EEPROM.read(TZaddressEEOK)==88 ) {
    TZ = EEPROM.read(TZaddressEE);
    Serial.println("EEPROM TZ=" + String(TZ) );
  }
  
  if ( EEPROM.read(TCaddressEEOK)==88 ) {
    TempCorr = EEPROM.read(TCaddressEE);
    if (TempCorr>12) {
		TempCorr = TempCorr - 256;
	}
	Serial.println("EEPROM TempCorr=" + String(TempCorr) );
  }

    // BME280
    unsigned status;
    
    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    status = bme.begin();  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        
    }
   // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  
   display.setBrightness(0x00);
   display2.setBrightness(0x00);
   
   flipper.attach(1, flip);
//   pollinput.attach(0.1, getInput);

   setInterval(10);
   setDebug(ERROR,Serial);

  
   //waitForSync();
   

  WiFi.begin(ssid, password);
  Serial.println("");

  // ждем соединения:
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");  //  "Подключились к "
  Serial.println(ssid);
  Serial.print("IP address: ");  //  "IP-адрес: "
  Serial.println(WiFi.localIP());

   
  if (!MDNS.begin("esp8266"))   {  Serial.println("Error setting up MDNS responder!");  }
      else   {  Serial.println("mDNS responder started");  }
 
  
  server.on("/", handleRoot);
  server.on("/ledon", handleLEDon);
  server.on("/ledoff", handleLEDoff);
  server.on("/getstatus", handleStatus);                                                                             
  //server.on("/setTimeZone", [](){handleAuth(); setTZ(server.arg()); server.send(200, "text/plain", "setTimeHM OK"); });

  server.on("/wifireset", handleWifiReset);
  server.onNotFound(handleNotFound);

  server.on("/favicon.ico", handleFavicon);
  server.on("/jquery-3.4.1.min.js", handleJquery);
  server.begin();


  NTPUpdater.attach(updPeriod, updateTIME);
  DHTUpdater.attach(5,updateDHT);
  DisplayOrderer.attach(20,changeDisplayToDHT);
  

}

void loop()
{
  events();  
  server.handleClient();
  
  MDNS.update();
  
  if (readDHT) {
    readDHT = false;
    updateDHT();
    }
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString);
    if (inputString=="wifireset\n") {
        Serial.println("Triggered wifireset");
        handleWifiReset();
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  
  
}

void setName(String Name) {
  
  //Write string to eeprom
  
  for(int i=0;i<10;i++)
  {
    EEPROM.write(NameAddressEE+i, Name[i]); //Write one by one with starting address of 0x0F
  }
  EEPROM.commit();    //Store data to EEPROM
  
  Serial.println("New Name = " + Name);
  
}

void setTZ(int8_t tz) {
  TZ = tz;
  
  EEPROM.write(TZaddressEE, TZ);
  EEPROM.write(TZaddressEEOK, 88);
  EEPROM.commit(); 
  Serial.println("New TZ = " + String(TZ));
  updateTIME();
  
}


void setTempCorr(int8_t tc) {
  TempCorr = tc;
  
  EEPROM.write(TCaddressEE, TempCorr);
  EEPROM.write(TCaddressEEOK, 88);
  EEPROM.commit(); 
  Serial.println("New TempCorr = " + String(TempCorr));
  updateDHT();
  
}


void updateDHT(){
  DHTUpdater.detach();
  DHTUpdater.attach(20,updateDHT);
  float h = 0;
  float t = 0;
  float p = 0;
  
  t = bme.readTemperature();
  p = bme.readPressure() / 100.0F;
  h = bme.readHumidity();
  
  //int err = SimpleDHTErrSuccess;
//    if ((err = dht22.read2(&t, &h, NULL)) != SimpleDHTErrSuccess) {
//      Serial.print("Read DHT22 failed, err="); Serial.println(err);
//      return;
//    }
    Humidity = h;
    Temperature = t + TempCorr; 
    Pressure = p;
    
    
}

void updateTIME() { //restart after NTP fails for a long time (need fixes) 
  
  if (timeStatus()==timeNotSet) {
    ntpGood = 0;
    updateFails++;
    
    if ( (updPeriod < 300) && (updateFails>10)) {
	   updateFails = 0;
	   updPeriod = updPeriod * 2;
	   NTPUpdater.detach();
       NTPUpdater.attach(updPeriod, updateTIME);
	}
	
    if ( (updPeriod >= 300) && (updateFails>10) ) {
	   ESP.restart();  
	}
	Serial.println(errorString());
	} else {
	  ntpGood = 1;
    }

      
   Serial.println(UTC.dateTime());

  if (!ntpGood) {
    ntpGood = 0;
    return ;
  }
  
  hours = (UTC.hour() + TZ) > 23 ? UTC.hour() + TZ - 24 : UTC.hour() + TZ;
  hours = hours < 0 ? hours + 24: hours;
  minutes = UTC.minute();
  seconds = UTC.second();
    
  //ntpGood = timeClient.update();
}
