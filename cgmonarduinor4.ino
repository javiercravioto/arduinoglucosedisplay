#include <WiFi.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoJson.h>

const char ssid[] = "########";   //wifi name
const char pass[] = "#########";  // wifi password

const char host[] = "testing.com";  //domain for mid processor
const int httpPort = 80;
const char resource[] = "/dbplayground/midreader.php";  // path to 

WiFiClient client;
ArduinoLEDMatrix matrix;
const int buzzerPin = 7;  // Change if needed

const uint8_t digits[10][7][5] = {
  {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,1,1},{1,0,1,0,1},{1,1,0,0,1},{1,0,0,0,1},{1,1,1,1,0}}, // 0
  {{0,0,1,0,0},{0,1,1,0,0},{1,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{1,1,1,1,1}}, // 1
  {{1,1,1,1,0},{0,0,0,0,1},{0,0,0,0,1},{0,0,1,1,0},{0,1,0,0,0},{1,0,0,0,0},{1,1,1,1,1}}, // 2
  {{1,1,1,1,0},{0,0,0,0,1},{0,0,0,1,0},{0,0,1,1,0},{0,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}}, // 3
  {{0,0,0,1,0},{0,0,1,1,0},{0,1,0,1,0},{1,0,0,1,0},{1,1,1,1,1},{0,0,0,1,0},{0,0,0,1,0}}, // 4
  {{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{0,0,0,0,1},{0,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}}, // 5
  {{0,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}}, // 6
  {{1,1,1,1,1},{0,0,0,0,1},{0,0,0,1,0},{0,0,1,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,0,0,0}}, // 7
  {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}}, // 8
  {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{0,1,1,1,0}}  // 9
};

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  matrix.begin();
  connectWiFi();
}

void loop() {
  int glucose = fetchGlucose();
  if (glucose > 0) {
    displayGlucose(glucose);
    alertGlucose(glucose);
  } else {
    displayError();
  }
  delay(60000);  // 1 minute
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected!");
}

int fetchGlucose() {
  Serial.println("Fetching glucose data...");
  IPAddress ip;
  Serial.print("Resolving host... ");
  if (!WiFi.hostByName(host, ip)) {
    Serial.println("DNS failed!");
    return -1;
  }
  Serial.println(ip);

  Serial.print("Connecting to host... ");
  if (!client.connect(host, httpPort)) {
    Serial.println("❌ Failed to connect to host");
    return -1;
  }
  Serial.println("✅ Connected!");

  client.print(String("GET ") + resource + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: CGM-Arduino\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  String response = "";
  bool jsonStarted = false;
  while (client.available()) {
    char c = client.read();
    if (c == '{') jsonStarted = true;
    if (jsonStarted) response += c;
  }
  client.stop();
  response.trim();
  Serial.println("Cleaned JSON: " + response);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.println("JSON parse error");
    return -1;
  }

  return doc["glucose"];
}

void displayGlucose(int glucose) {
  Serial.println("Displaying glucose: " + String(glucose));
  uint8_t frame[8][12] = {0};

  int hundreds = glucose / 100;
  int tens = (glucose / 10) % 10;
  int units = glucose % 10;

  int col = 0;
  if (glucose >= 100) {
    copyDigitToFrame(digits[hundreds], frame, col); col += 4;
  } else {
    col += 2;
  }

  copyDigitToFrame(digits[tens], frame, col); col += 4;
  copyDigitToFrame(digits[units], frame, col);

  matrix.renderBitmap(frame, 8, 12);
}

void copyDigitToFrame(const uint8_t digit[7][5], uint8_t frame[8][12], int xOffset) {
  for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 5; x++) {
      int destX = xOffset + x;
      if (destX < 12) {
        frame[y][destX] = digit[y][x];
      }
    }
  }
}

void displayError() {
  Serial.println("Displaying error face");
  uint8_t error_face[8][12] = {
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,0,1,1,1,1,1,1,0,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,0,1,1,1,1,1,1,0,1,1},
    {1,1,1,0,1,1,1,1,0,1,1,1},
    {0,1,1,1,0,0,0,0,1,1,1,0},
    {0,0,1,1,1,1,1,1,1,1,0,0}
  };
  matrix.renderBitmap(error_face, 8, 12);
}

void alertGlucose(int glucose) {
  if (glucose > 180) {
    tone(buzzerPin, 1000, 500);
    delay(600);
  } else if (glucose < 90) {
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 1000, 300);
      delay(400);
    }
  }
}
