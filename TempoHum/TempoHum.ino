#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>


#define DHTPIN 14
// Digital pin connected to the DHT sensor, connected to D5
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --

//Button for Temperature only (D0 pin)
#define BTN1 16
//Button for Temperature and Humidity (D1 pin)
#define BTN2 5

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.

DHT dht(DHTPIN, DHTTYPE);
WiFiManager wm;
//WiFiServer server(80);
//Setting pin for Reset, Enable, D4, D5,D6,D7
//Pin connected D3, D4, D6, D7, D8, Tx(1), Change back to TX when //Serial not in use
LiquidCrystal lcd(0, 2, 12, 13, 15, 10);

const char* UID = "123456";
const char* WebURL = "https://tempo-hum-web-ui.vercel.app/";
const String URLlink = "https://tempo-hum-web-ui.vercel.app/";
char POSTURL[100];

WiFiClientSecure wifiClient;
//const char fingerprint[] PROGMEM = "CF 68 BB 32 2F 49 87 AC 06 20 70 5D 9E 2A 70 B6 40 11 AF 07";
int h, t;
bool res;
//Set the LCD number of text and number of lines
const int LCDTEXT = 16, LCDLINE = 2;
int BTNR = 0, BTNL = 0, REJWiFi = 0, URLP = 0, UIDP = 0, POST_interval = 300000, Timeout = 0, LeftBTN = 0, RightBTN = 0;
int prevTime, currentTime, duration, POSTprevTime;

void setup() {
  //Start DHT
  dht.begin();
  ////Serial monitor for debugging
  //Serial.begin(9600);
  //Set input pins for button at D0 and D1
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  delay(1000);
  //Initialize the number of text and number of line
  lcd.begin(LCDTEXT, LCDLINE);
  lcd.print("Initialising");
  //Serial.println("Initialising");
  //If possible, figure out how the secure network protocol work
  wifiClient.setInsecure();
  //Set as station to connect to WiFi
  WiFi.mode(WIFI_STA);
  //Set WiFi manager portal to timeout in 3 minutes
  wm.setConfigPortalTimeout(180);
  // reset settings - wipe stored credentials
  //wm.resetSettings();
  //Start WiFi manager to connect to WiFi, if no WiFi go into Access Point
  //Access point started with Name and Password
  res = wm.autoConnect("TEMPOHUM", "SENSYNC+TEMPOHUM");
  //if WiFi Connection fails
  if (!res) {
    //Serial.println("Failed to connect");
    lcd.clear();
    lcd.print("Failed to");
    lcd.setCursor(0, 1);
    lcd.print("Connect");
    delay(3000);
    lcd.clear();
    lcd.print("Right: Retry");
    lcd.setCursor(0, 1);
    lcd.print("Left: Skip WiFi");
    //Serial.print("Right: Retry");
    //Serial.print("Left: Skip WiFi");
    prevTime = millis();
    //Hold when button is not clicked
    while (!digitalRead(BTN1) && !digitalRead(BTN2)) {
      currentTime = millis();
      duration = currentTime - prevTime;
      //No input for 10 seconds, automatic skip wifi
      if (duration > 10000) {
        //Serial.println("Time out");
        Timeout = 1;
        break;
      }
      //Prevent WDT from timing out
      delay(10);
    }
    while (digitalRead(BTN1) || digitalRead(BTN2)) {}
    //Press Left button to restart the ESP
    if (digitalRead(BTN1)) {
      lcd.clear();
      lcd.print("Restarting...");
      //Serial.println("Restarting");
      ESP.restart();
    }
    //Press right button
    else if (digitalRead(BTN2) || Timeout == 1) {
      REJWiFi = 1;
      Timeout = 0;
      lcd.clear();
      lcd.print("Continue without");
      lcd.setCursor(0, 1);
      lcd.print("WiFi");
      //Serial.println("Continue Without WiFi");

      //Set power saving mode without WiFi
    }

  }
  //WiFi is connected, show that WiFi is connected
  else {
    //Serial.println("");
    //Serial.println("WiFi connected");
    lcd.clear();
    lcd.print("WiFi connected");
  }
  delay(1000);
  //SHow default setting
  lcd.clear();
  lcd.print("Default interval");
  lcd.setCursor(0, 1);
  lcd.print("5min - indoor");
  //Serial.println("5 min - default");
  prevTime = millis();
  //Wait for 5 seconds or user input
  do {
    currentTime = millis();
    duration = currentTime - prevTime;
    //if user clicks on button, escape te loop and continue function
    if (digitalRead(BTN1) || digitalRead(BTN2)) {
      break;
    }
    delay(10);
  } while (duration < 5000);
  DHTSENSOR();
  if (WiFi.status() == WL_CONNECTED) {
    POSTREQ();
  }
  DHTLCDPRINT();
  POSTprevTime = millis();
}

void loop() {
  do {
    LeftBTN = digitalRead(BTN1);
    RightBTN = digitalRead(BTN2);
    delay(1);
  } while (digitalRead(BTN1) || digitalRead(BTN2) && !(digitalRead(BTN1) && digitalRead(BTN2)) );
  if (WiFi.status() == WL_DISCONNECTED && REJWiFi == 0) {
    //Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.print("WiFi Disconnected");
    delay(1000);
    lcd.clear();
    lcd.print("Starting WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Manager");
    res = wm.autoConnect("TEMPOHUM", "SENSYNC+TEMPOHUM");
    if (!res) {
      //Serial.println("Failed to connect");
      lcd.clear();
      lcd.print("Failed to");
      lcd.setCursor(0, 1);
      lcd.print("Connect");
      delay(3000);
      lcd.clear();
      lcd.print("Right for rst");
      lcd.setCursor(0, 1);
      lcd.print("Left for skip");
      //Serial.print("Right for rst");
      //Serial.print("Left for skip");
      while (digitalRead(BTN1) == 0 && digitalRead(BTN2) == 0) {
        currentTime = millis();
        duration = currentTime - prevTime;
        //No input for 10 seconds, automatic skip wifi
        if (duration > 10000) {
          //Serial.println("Time out");
          Timeout = 1;
          break;
        }
        delay(10);
      }
      if (digitalRead(BTN1)) {
        lcd.clear();
        lcd.print("Retry");
        //Serial.println("Retry");
        return;
      }
      else if (digitalRead(BTN2)) {
        REJWiFi = 1;
        lcd.clear();
        lcd.print("Continue without");
        lcd.setCursor(0, 1);
        lcd.print("WiFi");
        //Serial.println("Continue Without WiFi");
        delay(1000);
        Timeout = 0;
        //Set power saving mode without WiFi
      }
    }
    else {
      //if you get here you have connected to the WiFi
      //Serial.println("");
      //Serial.println("WiFi connected");
      lcd.clear();
      lcd.print("WiFi connected");
      //Serial.println("IP address: ");
      //Serial.println(WiFi.localIP());
    }
    delay(1000);
  }

  if (LeftBTN && RightBTN) {
    //Serial.println("Resetting for WiFi connection");
    lcd.clear();
    lcd.print("Reconnect WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Hold to reset");
    //Restart ESP WiFi functions
    REJWiFi = 0;
    prevTime = millis();
    do {
      currentTime = millis();
      duration = currentTime - prevTime;
      delay(10);
      if (duration > 5000)
      {
        wm.resetSettings();
        lcd.clear();
        lcd.print("WiFi reset");
        delay(1000);
      }
    } while (digitalRead(BTN1) && digitalRead(BTN2));
    DHTLCDPRINT();
    return;
  }
  else if (LeftBTN) {
    //Serial.println("Left");
    prevTime = millis();
    do {
      currentTime = millis();
      while (digitalRead(BTN1)) {
        LeftBTN = 1;
      }
      if (LeftBTN) {
        BTNL++;
        LeftBTN = 0;
        if (BTNL == 1) {
          lcd.clear();
          lcd.print("1min interval");
          lcd.setCursor(0, 1);
          lcd.print("Outdoor Use");
          POST_interval = 60 * 1000;
          //Serial.println("1 min - Indoor Use");
          prevTime = currentTime;
        }
        else {
          lcd.clear();
          lcd.print("5min interval");
          lcd.setCursor(0, 1);
          lcd.print("Indoor Use");
          //Set to 5 minute interval
          //Serial.println("5 min - Indoor Use");
          POST_interval = 5 * 60 * 1000;
          BTNL = 0;
          prevTime = currentTime;
        }
      }
      duration = currentTime - prevTime;
      delay(10);
    } while (!digitalRead(BTN2) && duration < 5000);
    prevTime = currentTime;
    DHTLCDPRINT();
    while(digitalRead(BTN1) || digitalRead(BTN2)){delay(1);}
  }
  else if (RightBTN) {
    //Serial.println("Right");
    BTNR++;
    if (BTNR == 1) {
      lcd.clear();
      lcd.print("URL: ");
      //Serial.println("URL:");
      //Serial.println(WebURL);
      URLP = 1;
      prevTime = millis();
      int n = 0, c = 0;
      int l = URLlink.length();
      do {
        //        if(n/800 > l-15){n=0;}
        if (n >= 500) {
          if (c > l) {
            c = 0;
          }
          lcd.setCursor(0, 1);
          lcd.print(URLlink.substring(c, c + 15));
          n = 0;
          c++;
        }
        currentTime = millis();
        duration = currentTime - prevTime;
        delay(1);
        n++;
      } while (!digitalRead(BTN1) && !digitalRead(BTN2) && duration < 15000 );
    }
    else if (BTNR == 2) {
      lcd.clear();
      lcd.print("UID: ");
      lcd.setCursor(0, 1);
      lcd.print(UID);
      //Serial.println("UID:");
      //Serial.println(UID);
      UIDP = 1;
      BTNR = 0;
      prevTime = millis();
      do {
        currentTime = millis();
        duration = currentTime - prevTime;
        delay(1);
      } while (!digitalRead(BTN1) && !digitalRead(BTN2) && duration < 10000 );
    }
    DHTLCDPRINT();
    while(digitalRead(BTN1) || digitalRead(BTN2)){delay(1);}
  }
  currentTime = millis();
  duration = currentTime - POSTprevTime;
  if (duration > POST_interval && POST_interval > 0) {
    DHTSENSOR();
    if (WiFi.status() == WL_CONNECTED) {
      POSTREQ();
    }
    DHTLCDPRINT();
    POSTprevTime = currentTime;
  }
}


void DHTLCDPRINT() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(t / 100.0) + char(223) + "C ");
  lcd.setCursor(0, 1);
  lcd.print("Humid: " + String(h / 100.0) + "% ");
  //Serial.println("Temp: " + String(t / 100.0) + "°C ");
  //Serial.println("Humid: " + String(h / 100.0) + "% ");
  delay(100);
}

void DHTSENSOR() {
  float H = dht.readHumidity();
  float T = dht.readTemperature();
  H = H * 100.0;
  T = T * 100.0;
  h=(int)H;
  t=(int)T;
}

void POSTREQ() {
  String response;
  HTTPClient http;
  sprintf(POSTURL, "%slist/sensor/%s.json", WebURL, UID);
  //Serial.println("Sending data");
  lcd.clear();
  lcd.print("Sending data");
  http.begin(wifiClient, POSTURL);
  StaticJsonDocument<256> dat;
  String postjson;
  dat["UID"] = UID;
  dat["Temperature"] = t / 100.0;
  dat["Humidity"] = h / 100.0;
  //SerializeJson(dat, postjson);
  //Serial.println(POSTURL + postjson);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(postjson);
  //Serial.println("HttpResponse=" + String(httpResponseCode));
  response = http.getString();
  //Serial.println(response);
  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    lcd.clear();
    lcd.print("Data sent");
    //Serial.println("Data sent");

  }
  http.end();
  delay(2000);
}