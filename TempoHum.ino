#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
// include the library code:
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
//Pin connected D3, D4, D6, D7, D8, Tx(1)
LiquidCrystal lcd(0, 2, 12, 13, 15, 4);

const char* UID = "123456";
const char* WebURL = "https://tempo-hum-web-ui.vercel.app/";
char POSTURL[100];

WiFiClientSecure wifiClient;
//const char fingerprint[] PROGMEM = "CF 68 BB 32 2F 49 87 AC 06 20 70 5D 9E 2A 70 B6 40 11 AF 07";
long timeout = 60000;
int h, t;
bool res, READ;
bool ref = HIGH;
//Set the LCD number of text and number of lines
const int LCDTEXT = 16, LCDLINE = 2;
int BTNR = 0, BTNL = 0, REJWiFi = 0, RUNONCE = 0, URLP = 0, UIDP = 0;
int ResetTime = 3000;
int prevTime, currentTime, duration;

void setup() {
  dht.begin();
  Serial.begin(9600);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  delay(1000);
  lcd.begin(LCDTEXT, LCDLINE);
  lcd.print("Initialising");
  Serial.println("Initialising");
  wifiClient.setInsecure();
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  //wm.setConfigPortalTimeout(180);


  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  //  wm.setAPCallback(configModeCallback);

  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("ESP32Test", "12345678abcdefg"); // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    lcd.clear();
    lcd.print("Failed to");
    lcd.setCursor(0, 1);
    lcd.print("Connect");
    delay(3000);
    lcd.clear();
    lcd.print("Right for rst");
    lcd.setCursor(0, 1);
    lcd.print("Left for skip");
    Serial.print("Right for rst");
    Serial.print("Left for skip");
    prevTime = millis();
    while (!digitalRead(BTN1) && !digitalRead(BTN2)) {
      currentTime = millis();
      duration = currentTime - prevTime;
      if (duration > timeout) {
        lcd.clear();
        lcd.print("TimeOut");
        Serial.println("Timeout");
        ESP.restart();
      }
    }
    if (digitalRead(BTN1)) {
      lcd.clear();
      lcd.print("Restarting...");
      Serial.println("Restarting");
      ESP.restart();
    }
    else if (digitalRead(BTN2)) {
      REJWiFi = 1;
      lcd.clear();
      lcd.print("Continue without");
      lcd.setCursor(0, 1);
      lcd.print("WiFi");
      Serial.println("Continue Without WiFi");
      delay(1000);
    }
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("");
    Serial.println("WiFi connected");
    lcd.clear();
    lcd.print("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  prevTime = millis();
}

void loop() {
  int T;
  int H;
  do {
    T = digitalRead(BTN1);
    H = digitalRead(BTN2);
    delay(100);
  } while (digitalRead(BTN1)|| digitalRead(BTN2) && !(digitalRead(BTN1) && digitalRead(BTN2)) );
  currentTime = millis();
  duration = currentTime - prevTime;
  if (duration > timeout) {
    RUNONCE = 0;
    prevTime = millis();
  }
  //Serial.println("TEST");
  if (WiFi.status() == WL_DISCONNECTED && REJWiFi == 0) {
    Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.print("WiFi Disconnected");
    delay(1000);
    lcd.clear();
    lcd.print("Starting WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Manager");
    res = wm.autoConnect("ESP32Test", "12345678abcdefg");
    if (!res) {
      Serial.println("Failed to connect");
      lcd.clear();
      lcd.print("Failed to");
      lcd.setCursor(0, 1);
      lcd.print("Connect");
      delay(3000);
      lcd.clear();
      lcd.print("Right for rst");
      lcd.setCursor(0, 1);
      lcd.print("Left for skip");
      Serial.print("Right for rst");
      Serial.print("Left for skip");
      while (digitalRead(BTN1) == 0 && digitalRead(BTN2) == 0) {}
      if (digitalRead(BTN1)) {
        lcd.clear();
        lcd.print("Retry");
        Serial.println("Retry");
        return;
      }
      else if (digitalRead(BTN2)) {
        REJWiFi = 1;
        lcd.clear();
        lcd.print("Continue without");
        lcd.setCursor(0, 1);
        lcd.print("WiFi");
        Serial.println("Continue Without WiFi");
        delay(1000);
      }
    }
    else {
      //if you get here you have connected to the WiFi
      Serial.println("");
      Serial.println("WiFi connected");
      lcd.clear();
      lcd.print("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  if (BTNL == 0 && BTNR == 0 && RUNONCE == 0) {
    lcd.clear();
    lcd.print("Left");
    lcd.setCursor(10, 0);
    lcd.print("Right");
    lcd.setCursor(0, 1);
    lcd.print("T/H");
    lcd.setCursor(11, 1);
    lcd.print("Menu");
    Serial.println("Press a Button");
    RUNONCE = 1;
  }
  if (T && H) {
    Serial.println("Resetting for WiFi connection");
    REJWiFi = 0;
    prevTime = currentTime;
    while(digitalRead(BTN1) || digitalRead(BTN2)){}
    return;
  }

  else if (T) {
    DHTSENSOR();
    BTNL = 1;
    BTNR = 0;
    // Serial. println("BTNR:" + String(BTNR) + "BTNL" + String(BTNL));
    if (WiFi.status() == WL_CONNECTED) {
      String response;
      HTTPClient http;
      sprintf(POSTURL, "%slist/sensor/%s.json", WebURL, UID);
      Serial.println("Sending data");
      lcd.clear();
      lcd.print("Sending data");
      http.begin(wifiClient, POSTURL);
      StaticJsonDocument<256> dat;
      String postjson;
      dat["UID"] = UID;
      dat["Temperature"] = t / 100.0;
      dat["Humidity"] = h;
      serializeJson(dat, postjson);
      //Serial.println(postjson);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(postjson);
      Serial.println("HttpResponse=" + String(httpResponseCode));
      response = http.getString();
      Serial.println(response);
      if(httpResponseCode >= 200 && httpResponseCode <300){
        lcd.clear();
        lcd.print("Data sent");
        Serial.println("Data sent");
      }
      http.end();
    }

    DHTLCDPRINT_ALL();
    prevTime = currentTime;
    return;
  }

  else if (H) {
    BTNL = 0;
    BTNR++;
    URLP = 0;
    UIDP = 0;
    if (BTNR == 1 && URLP == 0) {
      Serial.println("URL:");
      Serial.println(WebURL);
      URLP = 1;
      prevTime = currentTime;
      return;
    }
    else if (BTNR == 2 && UIDP == 0) {
      Serial.println("UID:");
      Serial.println(UID);
      UIDP = 1;
      prevTime = currentTime;
      return;
    }
    else if (BTNR > 2) {
      BTNR = 0;
      prevTime = currentTime;
      return;
    }
  }
}


void DHTLCDPRINT_ALL() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(t) + char(223) + "C ");
  lcd.setCursor(0, 1);
  lcd.print("Humid: " + String(h) + "% ");
  Serial.println("Temp: " + String(t / 100.0) + "°C ");
  Serial.println("Humid: " + String(h) + "% ");
  delay(100);
}

void DHTSENSOR() {
  delay(2000);
  h = dht.readHumidity();
  float T = dht.readTemperature();
  t = (int)T * 100.0;
}
