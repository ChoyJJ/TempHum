

#include <DHT.h>
//#include <WiFiManager.h>
//#include <WiFi.h>
#include <ESP8266WiFi.h>

// include the library code:
#include <LiquidCrystal.h>

#define DHTPIN 5
// Digital pin connected to the DHT sensor, connected to D1
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --

//#define DHTPOWER 12
#define BTN1 10
#define BTN2 4

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.

DHT dht(DHTPIN, DHTTYPE);
//WiFiManager wm;
WiFiServer server(80);
//Setting pin for Reset, Enable, D4, D5,D6,D7
//Pin connected D2, D3, D5, D6, D7, D8
LiquidCrystal lcd(2, 0, 15, 13, 12, 14);
//String HTTP;
//long timeout = 2000;
float h, t, f;
bool res, READ;
bool ref = HIGH;
//Set the LCD number of text and number of lines
const int LCDTEXT = 16, LCDLINE = 2;
int BTNMODE = 0;

void setup() {
  //  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  //  Serial.begin(9600);


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
  //  res = wm.autoConnect("ESP32Test", "123456789"); // password protected ap
  //
  //  if (!res) {
  //    Serial.println("Failed to connect");
  //
  //  }
  //  else {
  //    //if you get here you have connected to the WiFi
  //    Serial.println("");
  //    Serial.println("WiFi connected.");
  //    Serial.println("IP address: ");
  //    Serial.println(WiFi.localIP());
  //    server.begin();
  //  }
  Serial.begin(9600);
  dht.begin();
  pinMode(BTN1, FUNCTION_3);
  //pinMode(BTN2, FUNCTION_3);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  delay(1000);
  lcd.begin(LCDTEXT, LCDLINE);
  lcd.print("TEST");
  //Serial.println("Test");
  while(digitalRead(BTN1)||digitalRead(BTN2)){}
  
}

void loop() {
  //Serial.println("Start");
  int T = digitalRead(BTN1);
  int H = digitalRead(BTN2);
  //    Serial.println("TEST");
  //    if (WiFi.status() == WL_DISCONNECTED) {
  //      Serial.println("Starting WiFi Manager");
  //      res = wm.autoConnect("ESP32Test", "123456789");
  //      if (!res) {
  //        Serial.println("Failed");
  //      }
  //      else {
  //        Serial.println("COnnected");
  //      }
  //    }
  if (BTNMODE == 0) {
    int n = 0;
    lcd.clear();
    lcd.print("Waiting request");
    lcd.setCursor(0, 1);
    while (digitalRead(BTN1) == 0 && digitalRead(BTN2) == 0) {
      if (n == 16) {
        for (int i = 0 ; i < 16; i++) {
          lcd.setCursor(i, 1);
          lcd.println(" ");
        }
        lcd.setCursor(0, 1);
        n = 0;
      }
      lcd.print(".");
      n = n + 1;
      delay(200);
    }
  }

  if (T) {
    DHTSENSOR();
    DHTLCDPRINT_TEMP();
    BTNMODE = 1;
  }
  else if (H) {
    DHTSENSOR();
    DHTLCDPRINT_ALL();
    BTNMODE = 1;
  }
  else {
    return;
  }



  //    READ = LOW;
  //  }
  //Enter web server if a client connects, writes to the client and then exit upon fnishing writing
  //  WEBSERVER();


}


void DHTLCDPRINT_TEMP() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(t) + char(223) + "C ");
  delay(100);
}
void DHTLCDPRINT_ALL() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(t) + char(223) + "C ");
  lcd.setCursor(0, 1);
  lcd.print("Humid: " + String(h) + "% ");
  delay(100);
}



void DHTSENSOR() {
  // Wait a few seconds between measurements.
  delay(2000);
  //  lcd

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t) || isnan(f)) {
  //   //Serial.println(F("Failed to read from DHT sensor!"));
  //   return;
  // }
  //
  //  Serial.print(F("Humidity: "));
  //  Serial.print(h);
  //  Serial.print(F("%  Temperature: "));
  //  Serial.print(t);
  //  Serial.print(F("°C "));
  //  Serial.print(f);
  //  Serial.print(F("°F\n"));
}
//
//void WEBSERVER() {
//  WiFiClient client = server.available();   // Listen for incoming clients
//
//  if (client) {                             // If a new client connects,
//    //    currentTime = millis();
//    //    previousTime = currentTime;
//    Serial.println("New Client.");          // print a message out in the serial port
//    String currentLine = "";                // make a String to hold incoming data from the client
//    while (client.connected()) {  // loop while the client's connected
//      //currentTime = millis();
//      if (client.available()) {             // if there's bytes to read from the client,
//        char c = client.read();             // read a byte, then
//        Serial.write(c);                    // print it out the serial monitor
//        HTTP += c;
//        if (c == '\n') {                    // if the byte is a newline character
//          // if the current line is blank, you got two newline characters in a row.
//          // that's the end of the client HTTP request, so send a response:
//          if (currentLine.length() == 0) {
//            // HTTP HTTPs always start with a response code (e.g. HTTP/1.1 200 OK)
//            // and a content-type so the client knows what's coming, then a blank line:
//            client.println("HTTP/1.1 200 OK");
//            client.println("Content-type:text/html");
//            client.println("Connection: close");
//            client.println();
//            // turns the GPIOs on and off
//            if (HTTP.indexOf("refresh1") >= 0 || HTTP.indexOf("refresh2") >= 0) {
//              Serial.println("READ DHT");
//              //READ = HIGH;
//              DHTSENSOR();
//              HTTP = "";
//              ref = !ref;
//            }
//
//            // Display the HTML web page
//            client.println("<!DOCTYPE html><html>");
//            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
//            client.println("<link rel=\"icon\" href=\"data:,\">");
//            // CSS to style the on/off buttons
//            // Feel free to change the background-color and font-size attributes to fit your preferences
//            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
//            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
//            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
//            client.println(".button2 {background-color: #555555;}</style></head>");
//
//            // Web Page Heading
//            client.println("<body><h1>Temp & Humid Sensor</h1>");
//
//            // Display current state, and ON/OFF buttons for GPIO 26
//            client.print("<p>Humidity:  ");
//            client.print(h);
//            client.print("%</p>");
//
//            // Display current state, and ON/OFF buttons for GPIO 27
//            client.print("<p>Temperature:  ");
//            client.print(t);
//            client.print("°C ");
//            client.print(f);
//            client.print("°F</p>");
//            if (ref)
//            {
//              client.println("<p><a href=\"/refresh2\"><button class=\"button\">REFRESH</button></a></p>");
//            }
//            else
//            {
//              client.println("<p><a href=\"/refresh1\"><button class=\"button\">REFRESH</button></a></p>");
//            }
//            client.println("</body></html>");
//
//            // The HTTP response ends with another blank line
//            client.println();
//            // Break out of the while loop
//            break;
//          } else { // if you got a newline, then clear currentLine
//            currentLine = "";
//          }
//        } else if (c != '\r') {  // if you got anything else but a carriage return character,
//          currentLine += c;      // add it to the end of the currentLine
//        }
//      }
//    }
//    // Clear the HTTP variable
//    HTTP = "";
//    // Close the connection
//    client.stop();
//    Serial.println("Client disconnected.");
//    Serial.println("");
//  }
//}
