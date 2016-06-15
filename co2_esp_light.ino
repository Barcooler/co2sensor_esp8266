#define PIN_CLOCK  14 //Пин, к которому подключен контакт "С" — тактовый сигнал
#define PIN_DATA   12 //Пин, к которому подключен контакт "D" — данные

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "mt8060_decoder.h"

const char* ssid = "MikroTik-951"; //Имя сети
const char* password = "FERG2N76"; //Пароль сети
String co2_value = ""; //Значение содержания углекислоты в ppm
String tmp_value = ""; //Значение температуры в градусах цельсия
String hum_value = ""; //Значение влажности воздуха в процентах
int error_count = 0; //Счетчик ошибок контрольной суммы


ESP8266WebServer server(80); //Создаем обьект сервера

void setup()
{
  Serial.begin(115200); //Настраиваем UART
  WiFi.begin(ssid, password); //Подключаемся к WiFi

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected, IP ");
  Serial.println(WiFi.localIP());

  server.on("/co2", co2_show);
  server.on("/tmp", tmp_show);
  server.on("/hum", hum_show);
  server.on("/json", json_show);
  server.onNotFound(NotFound_show);
  server.begin();

  pinMode(PIN_CLOCK, INPUT); //Настраиваем порты на вход
  pinMode(PIN_DATA, INPUT); //Настраиваем порты на вход
  attachInterrupt(digitalPinToInterrupt(PIN_CLOCK), interrupt, FALLING); //Включаем прерывание на пине с тактовым сигналом
}


void interrupt()
{
  bool dataBit = (digitalRead(PIN_DATA) == HIGH);
  unsigned long ms = millis();
  
  mt8060_message* message = mt8060_process(ms, dataBit);

  if (message) {
    if (!message->checksumIsValid)
    {
      error_count++;
    }
    else
    {
      switch (message->type)
      {
        case HUMIDITY:
          hum_value = String((double)message->value / 100, 0);
          Serial.print("HUM:");
          Serial.println(hum_value);
          break;

        case TEMPERATURE:
          tmp_value = String((double)message->value / 16 - 273.15, 1);
          Serial.print("TMP:");
          Serial.println(tmp_value);
          break;

        case CO2_PPM:
          co2_value = String(message->value, DEC);
          Serial.print("CO2:");
          Serial.println(co2_value);
          break;

        default:
          break;
      }
    }
  }
}

void co2_show() {
  server.send(200, "text/plain", co2_value);
}

void tmp_show() {
  server.send(200, "text/plain", tmp_value);
}

  void hum_show() {
  server.send(200, "text/plain", hum_value);
}

void NotFound_show() {
  String form = "<!DOCTYPE html><html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"><title>Dadget МТ8060 CO2 monitor</title><style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;}</style><style type=\"text/css\"></style></head><body><div style=\"text-align:left;display:inline-block;min-width:260px;\"><h1>Dadget МТ8060 CO2 monitor</h1><form action=\"/co2\" method=\"get\"><button>CO2 value ([co2] ppm now)</button></form><br><form action=\"/tmp\" method=\"get\"><button>Temperature value ([tmp]&deg;С now)</button></form><br><form action=\"/hum\" method=\"get\"><button>Humidity value ([hum]% now)</button></form><br><form action=\"/json\" method=\"post\"><button>JSON(all values)</button></form></div></body></html>";
  form.replace("[co2]", co2_value);
  form.replace("[hum]", hum_value);
  form.replace("[tmp]", tmp_value);
  server.send(200, "text/html", form);
}

void json_show() {
  String json = "[{\"co2\":";
  json += co2_value;
  json += ",\"tmp\":";
  json += tmp_value;
  json += ",\"hum\":";
  json += hum_value;
  json += ",\"serial\":";
  json += ESP.getChipId();
  json += ",\"errors\":";
  json += error_count;
  json += ",\"uptime_min\":";
  json += String(millis()/60000);
  json += "}]";

  server.send(200, "application/json", json);
}

void loop()
{
  server.handleClient();
}

