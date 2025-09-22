#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "time.h"

float usd_rub = 0;
float usd_eur = 0;
unsigned long lastUpdate = 0;

// --- WiFi ---
const char* ssid     = "YOUR_WIFI_SSID"; // Имя вашей сети
const char* password = "YOUR_WIFI_PASSWORD";  // Пароль вашей сети

// --- Telegram ---
String botToken = "YOUR_TELEGRAM_BOT_TOKEN";
String chatID1   = "YOUR_TG_CHANNEL_1";  //Название вашего Telegram канала, например @NikitaGut
String chatID2   = "YOUR_TG_CHANNEL_2";

// --- OpenWeather ---
float temperature = 0;
float windspeed = 0;
String weather_desc = "";

// --- LCD 16x2 ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- NTP ---
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;  // Москва = GMT+3
const int daylightOffset_sec = 0;

// --- День рождения ---
int bDay = 16;    // День и месяц рождения меняем на свой, мой 16 марта, можете поздравить, если что :)
int bMonth = 3;

// --- Таймер ---
unsigned long lastSwitch = 0;
int page = 0;

// --- Свои символы ---
byte sun[] = {
  B10001,
  B10001,
  B01110,
  B11111,
  B01110,
  B01110,
  B11011,
  B10001
};

byte rain[] = {
  B00000,
  B01110,
  B11111,
  B00010,
  B01010,
  B01000,
  B00010,
  B00010
};

byte cloudly[] = {
  B00000,
  B01110,
  B11111,
  B00000,
  B00000,
  B01110,
  B11111,
  B00000
};

byte fog[] = {
  B10101,
  B01010,
  B10101,
  B01010,
  B10101,
  B01010,
  B10101,
  B01010
};

byte drizzle[] = {
  B10111,
  B11111,
  B10101,
  B01110,
  B10111,
  B11010,
  B10101,
  B01110
};

byte snow[] = {
  B00001,
  B01000,
  B00000,
  B10100,
  B01110,
  B00100,
  B10000,
  B00001
};

byte thunder[] = {
  B01110,
  B11111,
  B00000,
  B00010,
  B00100,
  B01110,
  B00100,
  B01000
};

byte whatahe[] = {
  B00000,
  B01110,
  B10001,
  B00001,
  B00110,
  B00100,
  B00000,
  B00100
};

void setup() {

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  // --- Создание символов ---
  lcd.createChar(0, sun);
  lcd.createChar(1, rain);
  lcd.createChar(2, cloudly);
  lcd.createChar(3, fog);
  lcd.createChar(4, drizzle);
  lcd.createChar(5, snow);
  lcd.createChar(6, thunder);
  lcd.createChar(7, whatahe);

  lcd.setCursor(0,0);
  lcd.print("Wait...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.setCursor(0,0);
  lcd.print("WiFi connected");
  digitalWrite(2, HIGH);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000);
}

void loop() {

  getCurrency();
  getWeather();

  if (millis() - lastSwitch > 5000) { // каждые 5 секунд переключаем экран
    lcd.clear();
    switch(page) {
      case 0: showSubscribers(); break;
      case 1: showDateTime(); break;
      case 2: showCountdowns(); break;
      case 3: showCurrency(); break;
      case 4: showWeather(); break;
    }
    page = (page + 1) % 5; 
    lastSwitch = millis();
  }
}

// ======= ЭКРАНЫ =======

void showSubscribers() {
  lcd.setCursor(0,0); lcd.print("Telegram");
  int subs1 = getTelegramSubs1();
  int subs2 = getTelegramSubs2();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("YourTG1:"); //Ваши названия каналов
  lcd.setCursor(11,0); lcd.print(subs1);
  lcd.setCursor(0,1); lcd.print("YourTG2:");
  lcd.setCursor(10,1); lcd.print(subs2);
}

void showDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    lcd.setCursor(0,0); lcd.print("Time error");
    return;
  }
  char buf1[16], buf2[16];
  for(int k = 0; k < 10; k++){
    strftime(buf1, sizeof(buf1), "%d.%m.%Y", &timeinfo);
    strftime(buf2, sizeof(buf2), "%H:%M:%S", &timeinfo);
    lcd.setCursor(0,0); lcd.print(buf1);
    lcd.setCursor(0,1); lcd.print(buf2);
    delay(500);
  }
}

void showCountdowns() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;
  int daysBDay = daysToBirthday(timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900);
  int daysNY   = daysToNewYear(timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year+1900);

  lcd.setCursor(0,0);
  lcd.print("BDay in:");
  lcd.print(daysBDay);
  lcd.print(" d");

  lcd.setCursor(0,1);
  lcd.print("NY in:");
  lcd.print(daysNY);
  lcd.print(" d");
}

void showCurrency() {
  lcd.setCursor(0,0); lcd.print("USD->RUB:");
  lcd.setCursor(0,1); lcd.print(usd_rub);
}

void showWeather() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Don");
  lcd.setCursor(0,1);
  lcd.print(temperature);
  lcd.print((char)223); // символ °
  lcd.print("C ");
  lcd.setCursor(8,1);

  if (weather_desc == "Clear sky"){
    lcd.write(0);
  }
  else if (weather_desc == "Partly cloudy"){
    lcd.write(2);
  }
  else if (weather_desc == "Drizzle"){
    lcd.write(4);
  }
  else if (weather_desc == "Rain"){
    lcd.write(1);
  }
  else if (weather_desc == "Snowfall"){
    lcd.write(5);
  }
  else if (weather_desc == "Thunderstorm"){
    lcd.write(6);
  }
  else if (weather_desc == "Unknown"){
    lcd.write(7);
  }
}

// ======= ВСПОМОГАТЕЛЬНЫЕ =======

int getTelegramSubs1() {
  if(WiFi.status()==WL_CONNECTED){
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getChatMemberCount?chat_id=" + chatID1;
    http.begin(url);
    int code = http.GET();
    if(code>0){
      String payload = http.getString();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);
      return doc["result"];
    }
    http.end();
  }
  return -1;
}

int getTelegramSubs2() {
  if(WiFi.status()==WL_CONNECTED){
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getChatMemberCount?chat_id=" + chatID2;
    http.begin(url);
    int code = http.GET();
    if(code>0){
      String payload = http.getString();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);
      return doc["result"];
    }
    http.end();
  }
  return -1;
}

int daysToBirthday(int d, int m, int y) {
  struct tm bday = {0};
  bday.tm_mday = bDay;
  bday.tm_mon  = bMonth - 1;
  bday.tm_year = y - 1900;
  time_t now = time(NULL);
  time_t t_bday = mktime(&bday);

  if (difftime(t_bday, now) < 0) {
    bday.tm_year++;
    t_bday = mktime(&bday);
  }
  return (int)(difftime(t_bday, now) / 86400);
}

int daysToNewYear(int d, int m, int y) {
  struct tm ny = {0};
  ny.tm_mday = 1;
  ny.tm_mon  = 0;
  ny.tm_year = y + 1 - 1900; 
  time_t now = time(NULL);
  time_t t_ny = mktime(&ny);
  return (int)(difftime(t_ny, now) / 86400);
}

void getCurrency() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://open.er-api.com/v6/latest/USD";
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        usd_rub = doc["rates"]["RUB"].as<float>();
        usd_eur = doc["rates"]["EUR"].as<float>();

        Serial.print("USD->RUB: ");
        Serial.println(usd_rub);
        Serial.print("USD->EUR: ");
        Serial.println(usd_eur);

        //lastUpdate = millis();  // обновили таймер

        //return usd_rub;

      } else {
        Serial.println("JSON parse error!");
      }
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
}

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Запрос Open-Meteo меняем координаты на свои, типо latitude=00.00&longitude=00.00
    String url = "https://api.open-meteo.com/v1/forecast?latitude=30.10&longitude=30.10&current_weather=true";
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonObject current = doc["current_weather"];

        temperature = current["temperature"];   // температура в °C
        windspeed = current["windspeed"];       // скорость ветра м/с
        int wcode = current["weathercode"];     // код погоды

        // Переводим weathercode в описание
        switch (wcode) {
          case 0: weather_desc = "Clear sky"; break;
          case 1: case 2: case 3: weather_desc = "Partly cloudy"; break;
          case 45: case 48: weather_desc = "Fog"; break;
          case 51: case 53: case 55: weather_desc = "Drizzle"; break;
          case 61: case 63: case 65: weather_desc = "Rain"; break;
          case 71: case 73: case 75: weather_desc = "Snowfall"; break;
          case 95: weather_desc = "Thunderstorm"; break;
          default: weather_desc = "Unknown"; break;
        }

        Serial.print("Temp: ");
        Serial.println(temperature);
        Serial.print("Wind: ");
        Serial.println(windspeed);
        Serial.print("Weather: ");
        Serial.println(weather_desc);
      } else {
        Serial.println("JSON parse error!");
      }
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
}