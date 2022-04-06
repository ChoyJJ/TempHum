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

// Button for Left (D0 pin)
#define BTN1 16
// Button for Right (D1 pin)
#define BTN2 5

// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.

DHT dht(DHTPIN, DHTTYPE);
WiFiManager wm;
// WiFiServer server(80);
// Setting pin for Reset, Enable, D4, D5,D6,D7
// Pin connected D3, D4, D6, D7, D8, Tx(1), Change back to TX when Serial not in use
LiquidCrystal lcd(0, 2, 12, 13, 15, 1);

const char *UID = "123456";
const char *WebURL = "https://tempo-hum-web-ui.vercel.app/";
const String URLlink = "https://tempo-hum-web-ui.vercel.app/";
char POSTURL[100];

WiFiClientSecure wifiClient;
// const char fingerprint[] PROGMEM = "CF 68 BB 32 2F 49 87 AC 06 20 70 5D 9E 2A 70 B6 40 11 AF 07";
int h, t;
bool res;
// Set the LCD number of text and number of lines
const int LCDTEXT = 16, LCDLINE = 2;
// Set initial conditions for button counts, wifi disabled(Not disabled), interval(5min),  button inputs and no error from DHT
int BTNR = 0, BTNL = 0, REJWiFi = 0, POST_interval = 300000, Timeout = 0, LeftBTN = 0, RightBTN = 0, e = 0;
// Global integer for passing time to other functions
int prevTime, currentTime, duration, POSTprevTime;

void setup()
{
  // Start DHT
  dht.begin();
  // Serial print is disabled because Tx is in use, Serial monitor is used for debugging
  // Serial monitor for debugging
  // Serial.begin(9600);
  // Set input pins for button at D0 and D1
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  delay(1000);
  // Initialize the number of text and number of line
  lcd.begin(LCDTEXT, LCDLINE);
  lcd.print("Initialising");
  // Serial.println("Initialising");
  // If possible, figure out how the secure network protocol work
  wifiClient.setInsecure();
  // Set as station to connect to WiFi
  WiFi.mode(WIFI_STA);
  // Set WiFi manager portal to timeout in 3 minutes
  wm.setConfigPortalTimeout(180);
  // reset settings - wipe stored credentials
  // wm.resetSettings();
  // Start WiFi manager to connect to WiFi, if no WiFi go into Access Point
  // Access point started with Name and Password
  delay(1000);
  lcd.clear();
  lcd.print("Starting WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Manager");
  res = wm.autoConnect("TEMPOHUM", "SENSYNC+TEMPOHUM");
  // if WiFi Connection fails
  if (!res)
  {
    // Serial.println("Failed to connect");
    lcd.clear();
    lcd.print("Failed to");
    lcd.setCursor(0, 1);
    lcd.print("Connect");
    delay(3000);
    lcd.clear();
    lcd.print("Right: Retry");
    lcd.setCursor(0, 1);
    lcd.print("Left: Skip WiFi");
    // Serial.print("Right: Retry");
    // Serial.print("Left: Skip WiFi");
    prevTime = millis();
    // Hold when button is not clicked
    while (!digitalRead(BTN1) && !digitalRead(BTN2))
    {
      currentTime = millis();
      duration = currentTime - prevTime;
      // No input for 10 seconds, automatic skip wifi
      if (duration > 10000)
      {
        // Serial.println("Time out");
        Timeout = 1;
        break;
      }
      // Prevent WDT from timing out
      delay(10);
    }
    while (digitalRead(BTN1) || digitalRead(BTN2))
    {
    }
    // Press right button to restart the ESP
    if (digitalRead(BTN2))
    {
      lcd.clear();
      lcd.print("Restarting...");
      // Serial.println("Restarting");
      ESP.restart();
    }
    // Press left button
    else if (digitalRead(BTN1) || Timeout == 1)
    {
      // Defines wifi is diabled
      REJWiFi = 1;
      Timeout = 0;
      lcd.clear();
      lcd.print("Continue without");
      lcd.setCursor(0, 1);
      lcd.print("WiFi");
      // Serial.println("Continue Without WiFi");

      // Set power saving mode without WiFi
    }
  }
  // WiFi is connected, show that WiFi is connected
  else
  {
    // Serial.println("");
    // Serial.println("WiFi connected");
    lcd.clear();
    lcd.print("WiFi connected");
  }
  delay(1000);
  // SHow default setting
  lcd.clear();
  lcd.print("Default interval");
  lcd.setCursor(0, 1);
  lcd.print("5min - indoor");
  // Serial.println("5 min - default");
  prevTime = millis();
  // Wait for 5 seconds or user input
  do
  {
    currentTime = millis();
    duration = currentTime - prevTime;
    // if user clicks on button, escape te loop and continue function
    if (digitalRead(BTN1) || digitalRead(BTN2))
    {
      break;
    }
    delay(10);
  } while (duration < 5000);
  DHTSENSOR();
  // Only send data when WiFi connected
  if (WiFi.status() == WL_CONNECTED)
  {
    POSTREQ();
  }
  DHTLCDPRINT();
  // Starts counting for next update
  POSTprevTime = millis();
}

void loop()
{
  // Check for button clicks, loop until button is released
  // In case of both button clicked, go to reset WiFi function
  do
  {
    LeftBTN = digitalRead(BTN1);
    RightBTN = digitalRead(BTN2);
    delay(1);
    if (LeftBTN && RightBTN)
    {
      break;
    }
    // digitalRead is used because using the variable will always end up with logic Low
  } while (digitalRead(BTN1) || digitalRead(BTN2) && !(digitalRead(BTN1) && digitalRead(BTN2)));
  //Check if WiFi is disconnected
  //if WiFi not disabled, attempt reconnect
  if ((WiFi.status() == 1 || WiFi.status() == 7) && REJWiFi == 0)
  {
    // Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.print("WiFi Disconnected");
    delay(1000);
    lcd.clear();
    lcd.print("Starting WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Manager");
    //Start WiFi Manager to connect to WiFi
    res = wm.autoConnect("TEMPOHUM", "SENSYNC+TEMPOHUM");
    //On Connection error, show connection failed and prompt for input
    if (!res)
    {
      // Serial.println("Failed to connect");
      lcd.clear();
      lcd.print("Failed to");
      lcd.setCursor(0, 1);
      lcd.print("Connect");
      delay(3000);
      lcd.clear();
      lcd.print("Right for rst");
      lcd.setCursor(0, 1);
      lcd.print("Left for skip");
      // Serial.print("Right for rst");
      // Serial.print("Left for skip");
      //Start counting for Timeout
      prevTime = millis();
      //Loop when there is no input for up to 10s
      while (digitalRead(BTN1) == 0 && digitalRead(BTN2) == 0)
      {
        currentTime = millis();
        duration = currentTime - prevTime;
        // No input for 10 seconds, automatic skip wifi
        if (duration > 10000)
        {
          // Serial.println("Time out");
          Timeout = 1;
          break;
        }
        delay(10);
      }
      //Right button will escape the function and attempt reconnect in main loop
      if (digitalRead(BTN2))
      {
        lcd.clear();
        lcd.print("Retry");
        // Serial.println("Retry");
        return;
      }
      //Left button disables WiFi
      else if (digitalRead(BTN1) || Timeout == 1)
      {
        REJWiFi = 1;
        lcd.clear();
        lcd.print("Continue without");
        lcd.setCursor(0, 1);
        lcd.print("WiFi");
        // Serial.println("Continue Without WiFi");
        delay(1000);
        Timeout = 0;
        // Set power saving mode without WiFi
      }
    }
    else
    {
      // if you get here you have connected to the WiFi
      // Serial.println("");
      // Serial.println("WiFi connected");
      lcd.clear();
      lcd.print("WiFi connected");
      // Serial.println("IP address: ");
      // Serial.println(WiFi.localIP());
    }
    delay(1000);
    //Display values again after setup
    DHTLCDPRINT();
  }
  //When both buttons are clicked
  if (LeftBTN && RightBTN)
  {
    // Serial.println("Resetting for WiFi connection");
    lcd.clear();
    lcd.print("Reconnect WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Hold to reset");
    // Restart ESP WiFi functions
    REJWiFi = 0;
    //Start delay count on how long the user held down
    prevTime = millis();
    //Loop until user release or 5 seconds delay reach and clear WiFi Manager
    do
    {
      currentTime = millis();
      duration = currentTime - prevTime;
      delay(10);
      //If user hold down both buttons for longer than 5s
      //Reset WiFi Manager, can connect to other WiFi
      if (duration > 5000)
      {
        //Reset WiFi Manager Settings
        wm.resetSettings();
        //Disconnect from previous WiFi 
        WiFi.disconnect();
        lcd.clear();
        lcd.print("WiFi reset");
        delay(1000);
      }
    } while (digitalRead(BTN1) && digitalRead(BTN2));
    //Always print values again after functions
    DHTLCDPRINT();
    return;
  }
  //Left button for switching between intervals
  else if (LeftBTN)
  {
    // Serial.println("Left");
    //Give a delay for user to select the interval they want
    //Should had done this for displaying info as well
    prevTime = millis();
    //If button click within 5 seconds, will remain in interval switching
    do
    {
      currentTime = millis();
      //await user input within 5s, and update that button is clicked
      while (digitalRead(BTN1))
      {
        LeftBTN = 1;
      }
      //each time left button is clicked
      if (LeftBTN)
      {
        //increment to button left count
        BTNL++;
        //Reset the left button trigger
        LeftBTN = 0;
        //Set Interval for 1 minutes
        if (BTNL == 1)
        {
          lcd.clear();
          lcd.print("1min interval");
          lcd.setCursor(0, 1);
          lcd.print("Outdoor Use");
          //set interval to 60,000 ms
          POST_interval = 60 * 1000;
          // Serial.println("1 min - Indoor Use");
          //reset time after activity
          prevTime = currentTime;
        }
        //Set Interval for 5 minutes
        else
        {
          lcd.clear();
          lcd.print("5min interval");
          lcd.setCursor(0, 1);
          lcd.print("Indoor Use");
          // Set to 5 minute interval
          // Serial.println("5 min - Indoor Use");
          //Set interval at 300,000 ms
          POST_interval = 5 * 60 * 1000;
          //Set next selection to 1 min
          BTNL = 0;
          //Reset after activity
          prevTime = currentTime;
        }
      }
      duration = currentTime - prevTime;
      delay(10);
      //Either wait after 5s or right button to exit immediately
    } while (!digitalRead(BTN2) && duration < 5000);
    prevTime = currentTime;
    //Show value again
    DHTLCDPRINT();
    //Make sure it doesn't trigger right button directly
    while (digitalRead(BTN2))
    {
      delay(1);
    }
  }
  //Right button, For Displaying URL and UID on LCD
  else if (RightBTN)
  {
    // Serial.println("Right");
    BTNR++;
    //1st option to show URL
    if (BTNR == 1)
    {
      lcd.clear();
      lcd.print("URL: ");
      // Serial.println("URL:");
      // Serial.println(WebURL);
      //Start timeout for the display
      prevTime = millis();
      int n = 0, c = 0;
      int l = URLlink.length();
      //Loops to show different character in the URL string
      do
      {
        //        if(n/800 > l-15){n=0;}
        // Every 500ms, shift 1 to left
        if (n >= 500)
        {
          //Start the string from the left again
          if (c > l)
          {
            c = 0;
          }
          lcd.setCursor(0, 1);
          //Only display 15 characters from the string
          lcd.print(URLlink.substring(c, c + 15));
          n = 0;
          //Shifting left by 1
          c++;
        }
        currentTime = millis();
        duration = currentTime - prevTime;
        delay(1);
        //Counting for 500ms
        n++;
      } while (!digitalRead(BTN1) && !digitalRead(BTN2) && duration < 15000);
    }
    //Display UID
    else if (BTNR == 2)
    {
      lcd.clear();
      lcd.print("UID: ");
      lcd.setCursor(0, 1);
      lcd.print(UID);
      // Serial.println("UID:");
      // Serial.println(UID);
      BTNR = 0;
      prevTime = millis();
      //display for set delay or change display when button clicked
      do
      {
        currentTime = millis();
        duration = currentTime - prevTime;
        delay(1);
      } while (!digitalRead(BTN1) && !digitalRead(BTN2) && duration < 10000);
    }
    //Print the values
    DHTLCDPRINT();
    //Hold to make sure no trigger on the button
    while (digitalRead(BTN1) || digitalRead(BTN2))
    {
      delay(1);
    }
  }
  
  currentTime = millis();
  //Delay since last post
  duration = currentTime - POSTprevTime;
  //Interval reached, read and post data
  if (duration > POST_interval)
  {
    DHTSENSOR();
    //Only send data when WiFi is available
    if (WiFi.status() == WL_CONNECTED)
    {
      POSTREQ();
    }
    //Print new Value on LCD
    DHTLCDPRINT();
    POSTprevTime = currentTime;
  }
}

void DHTLCDPRINT()
{
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(t / 100.0) + char(223) + "C ");
  lcd.setCursor(0, 1);
  lcd.print("Humid: " + String(h / 100.0) + "% ");
  // Serial.println("Temp: " + String(t / 100.0) + "°C ");
  // Serial.println("Humid: " + String(h / 100.0) + "% ");
  delay(100);
}

void DHTSENSOR()
{
  float H = dht.readHumidity();
  float T = dht.readTemperature();
  H = H * 100.0;
  T = T * 100.0;
  h = (int)H;
  t = (int)T;
  if (t > 10000 || h > 10000)
  {
    e = 1;
  }
}

void POSTREQ()
{
  String response;
  HTTPClient http;
  sprintf(POSTURL, "%slist/sensor/%s.json", WebURL, UID);
  // Serial.println("Sending data");
  if (e)
  {
    lcd.clear();
    lcd.print("Invalid Data");
    e = 0;
  }
  else
  {
    lcd.clear();
    lcd.print("Sending data");
    http.begin(wifiClient, POSTURL);
    StaticJsonDocument<256> dat;
    String postjson;
    dat["UID"] = UID;
    dat["Temperature"] = t / 100.0;
    dat["Humidity"] = h / 100.0;
    serializeJson(dat, postjson);
    // Serial.println(POSTURL + postjson);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(postjson);
    // Serial.println("HttpResponse=" + String(httpResponseCode));
    response = http.getString();
    // Serial.println(response);
    if (httpResponseCode >= 200 && httpResponseCode < 300)
    {
      lcd.clear();
      lcd.print("Data sent");
      // Serial.println("Data sent");
    }
    http.end();
  }
  delay(2000);
}
