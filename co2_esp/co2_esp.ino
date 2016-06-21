#define PIN_CLOCK  14 // Пин, к которому подключен контакт "С" — тактовый сигнал
#define PIN_DATA   12 // Пин, к которому подключен контакт "D" — данные

#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include "mt8060_decoder.h"

const char* ssid = "MikroTik-951"; //Имя сети
const char* password = "FAKEPASSWORD"; //Пароль сети
String co2_value = ""; //Значение содержания углекислоты в ppm
String tmp_value = ""; //Значение температуры в градусах цельсия
String hum_value = ""; //Значение влажности воздуха в процентах
int error_count = 0; //Счетчик ошибок контрольной суммы(кстати, можно не считать, неделями работает без ошибок)


ESP8266WebServer server(80); //Создаем обьект сервера

void setup()
{
  Serial.begin(115200); //Настраиваем UART
  WiFi.begin(ssid, password); //Подключаемся к WiFi

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print("."); // Усердно показываем в UART, что мы заняты делом
  }
  Serial.println("");
  Serial.print("WiFi connected, IP ");
  Serial.println(WiFi.localIP());

  server.on("/co2", co2_show); // Устанавливаем адреса страниц и функции, этим адресам соотвествующие
  server.on("/tmp", tmp_show);
  server.on("/hum", hum_show);
  server.on("/json", json_show);
  server.onNotFound(NotFound_show); // Главная страница
  server.begin();

  pinMode(PIN_CLOCK, INPUT); //Настраиваем порты на вход
  pinMode(PIN_DATA, INPUT); //Настраиваем порты на вход
  attachInterrupt(digitalPinToInterrupt(PIN_CLOCK), interrupt, FALLING); //Включаем прерывание на пине с тактовым сигналом
}


void interrupt() // По каждому прерыванию начинаем собирать данные
{
  bool dataBit = (digitalRead(PIN_DATA) == HIGH);
  unsigned long ms = millis();
  
  mt8060_message* message = mt8060_process(ms, dataBit); //Отправляем текущее время и текущий бит в функцию, взамен получаем восхитительное ничего, если битов еще не набралось до полного сообщение, и расшифрованный пакет, если битов достаточно.

  if (message) { //если в ответ получен пакет
    if (!message->checksumIsValid) //то проверяем, правильно ли рассчитана контрольная сумма
    {
      error_count++;
    }
    else // и если правильно...
    {
      switch (message->type) //...то в зависимости от типа пакета...
      {
        case HUMIDITY: 
          hum_value = String((double)message->value / 100, 0); // сохраняем значение влажности
          Serial.print("HUM:"); 
          Serial.println(hum_value); // Выводим в UART
          break;

        case TEMPERATURE:
          tmp_value = String((double)message->value / 16 - 273.15, 1); // конвертируем и сохраняем значение температуры
          Serial.print("TMP:"); // Смотрите-ка! Сиськи! -> (. )(. )
          Serial.println(tmp_value); // Выводим в UART
          break;

        case CO2_PPM:
          co2_value = String(message->value, DEC); // сохраняем значение количества CO₂
          Serial.print("CO2:");
          Serial.println(co2_value); // Выводим в UART
          break;

        default:
          break;
      }
    }
  }
}

void co2_show() { //Функция, выводящая CO₂ простым текстом
  server.send(200, "text/plain", co2_value);
}

void tmp_show() { //Функция, выводящая температуру простым текстом
  server.send(200, "text/plain", tmp_value);
}

  void hum_show() { //Функция, выводящая влажность простым текстом
  server.send(200, "text/plain", hum_value);
}

void NotFound_show() { //Функция, выводящая красивую стартовую страницу с кнопочками
  String form = "<!DOCTYPE html><html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"><title>Dadget МТ8060 CO₂ monitor</title><style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;}</style><style type=\"text/css\"></style></head><body><div style=\"text-align:left;display:inline-block;min-width:260px;\"><h1>Dadget МТ8060 CO₂ monitor</h1><form action=\"/co2\" method=\"get\"><button>CO₂ value ([co2] ppm now)</button></form><br><form action=\"/tmp\" method=\"get\"><button>Temperature value ([tmp]°С now)</button></form><br><form action=\"/hum\" method=\"get\"><button>Humidity value ([hum]% now)</button></form><br><form action=\"/json\" method=\"post\"><button>JSON(all values)</button></form></div></body></html>";
  form.replace("[co2]", co2_value); //подставляем текущие значения
  form.replace("[hum]", hum_value);
  form.replace("[tmp]", tmp_value);
  server.send(200, "text/html", form); //отправляем форму клиенту
}

void json_show() { //Функция, выводящая все данные в виде JSON
  String json = "[{\"co2\":"; //Формируем строку c JSON данными
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

  server.send(200, "application/json", json); //Отправляем
}

void loop()
{
  server.handleClient(); //Ждем подключения клиентов по HTTP
}
