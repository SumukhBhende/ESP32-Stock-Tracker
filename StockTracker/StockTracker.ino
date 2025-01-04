#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h> // For storing WiFi credentials

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences preferences; // For saving WiFi credentials

const char* default_ssid = "Universal LAN";
const char* default_password = "Connecto Patronum";

WebServer server(80); // Web server for the captive portal

const char* stocks1[] = {"SENSEX", "PFIZER", "ULTRATECH", "RELIANCE", "ADI-BIRLA", "GRASIM", "BOM-DYEING", "RAYMOND", "LICI", "TATA STEEL", "MAHINDRA", "JIOFIN", "RAYLFSTYL"};
const char* ticks1[] = {"SENSEX:INDEXBOM", "PFIZER:NSE", "ULTRACEMCO:NSE", "RELIANCE:NSE", "ABCAPITAL:NSE", "GRASIM:NSE", "BOMDYEING:NSE", "RAYMOND:NSE", "LICI:NSE", "TATASTEEL:NSE", "M%26M:NSE", "JIOFIN:NSE", "RAYMONDLSL:NSE"};
int units[] = {0, 50, 15, 144, 133, 95, 60, 28, 15, 335, 48, 72, 22};
float trprices[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void startHotspot() {
  WiFi.softAP("Stock Tracker");

  IPAddress IP = WiFi.softAPIP();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Hotspot Active");
  display.println(IP);
  display.display();

  server.on("/", []() {
    server.send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><title>WiFi Setup</title></head>
      <body>
        <h1>Enter WiFi Credentials</h1>
        <form action="/connect" method="POST">
          SSID: <input type="text" name="ssid"><br>
          Password: <input type="password" name="password"><br>
          <input type="submit" value="Submit">
        </form>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/connect", []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (ssid.length() > 0 && password.length() > 0) {
      preferences.begin("WiFiCreds", false);
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.end();

      server.send(200, "text/plain", "WiFi credentials saved. Restarting...");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "Invalid input. Please try again.");
    }
  });

  server.begin();
  while (true) {
    server.handleClient();
  }
}

void connectWiFi() {
  preferences.begin("WiFiCreds", true);
  String ssid = preferences.getString("ssid", default_ssid);
  String password = preferences.getString("password", default_password);
  preferences.end();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connected!");
    display.display();
    delay(2000);
  } else {
    startHotspot();
  }
}

void resetDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
}

void fetchStockPrice(int i) {
  String stockSymbol = ticks1[i];
  String url = "https://stockfin-api.vercel.app/stock/" + stockSymbol;

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      resetDisplay();
      display.setTextSize(1);
      display.println("JSON Parsing Failed");
      display.display();
      http.end();
      return;
    }

    if (doc.containsKey("cp")) {
      float price = doc["cp"].as<float>();
      trprices[i]=price;
      String name = stocks1[i];

      resetDisplay();
      display.setTextSize(2);
      display.println(name);
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print("Price: ");
      display.print(price, 2);
      display.println(" INR");
    } else {
      resetDisplay();
      display.setTextSize(1);
      display.println("No data available");
    }
  } else {
    resetDisplay();
    display.setTextSize(1);
    display.println("HTTP Error:");
    display.println(httpCode);
  }

  display.display();
  http.end();
}



void displaySumOnScreen(int k) {
  float sum=0;
  int i=0;
  for(i=0;i<k;i++){
    sum+=(units[i] * trprices[i]);
  }
  resetDisplay(); // Clear the display and set cursor to top-left
  display.setTextSize(2); // Set larger text size for better visibility
  display.setCursor(0, 0); // Position cursor
  display.println("Total = "); // Title for the sum
  display.setCursor(0, 20); // Move cursor below the title
  display.print(sum); // Display the calculated sum
  display.println(" INR"); // Append currency units
  display.display(); // Push changes to the display
}


void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Stock Prices Tracker");
  display.display();

  connectWiFi();
}

void loop() {
  float sum=0;
  int i=0;
  while(i<=13){
    if(i<13){
      fetchStockPrice(i);
      delay(10000);
      i++;
    }
    else{
      displaySumOnScreen(13);
      delay(20000);
    }
  }
}

