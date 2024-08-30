#include <Arduino.h>
#include <Wire.h>

// Для WiFi
#include <WiFi.h>
// Для веб-сервера
#include <WebServer.h>
// Смены имени в сети
#include <ESPmDNS.h>

// Including our html, css, js code from other <...>.h file
#include "page.h"

/* 
 * Макрос для подключение сервера в роли клента сети WiFi к маршрутизатору
 * и также изменением URL с цифрового IP на mDNS для подключения
 */
#define ESP_STATION_MODE

#ifdef ESP_CLIENT_MODE

/* SSID & Password */

// SSID существующей сети WiFI
const char* ssid = "ESP32_S3";     // Enter your exist WiFi ssid here
// Password существующей сети WiFI
const char* password = "esp32_s3"; // Enter your exist WiFi password here
// DNS для переименования URL сервера в составе существующей сети WiFI в виде <DNS>.local
const char* nameDNS = "esp32-dashboard"; //

#endif

#ifdef ESP_STATION_MODE

// SSID & Password
const char *ssid = "ESP32_S3";     // Enter your SSID here
const char *password = "esp32_s3"; // Enter your Password here

// IP Address details
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

#endif


// Object of WebServer(HTTP port, 80 is defult)
WebServer server(80);

// Последние значение таймера
unsigned long time_p = 0;
// Значение температуры
int8_t temp = 0;
String temp2;

// Обновление данных о температуре
void update_temp_data()
{
    // Текущее значение по таймеру в мс
    unsigned long time_n = millis();

    if((time_n - time_p) > 3000 )
    {
        temp = (int8_t)(temperatureRead());
        time_p = time_n;
    }
}

/**
 * \brief Функциия для фреймворка Ардуино, предназначенная преобразовавать промежуток из строки в целочисленное значение.
 *
 * \param [in] color - Название переменной типа строка, в которой нужно находить символы цифр
 * \param [in] start_ch - Символ, с которого начинатся поиск, предыдущий перед нужной цифрой
 * \param [in] end_ch - Символ, на который оканчивается интервал поиска, после интервала поиска цифр
 * \param [in] row_ch - Максимальное количество цифр, которое нужно преобразовать в тип Integer (макс. значение 255)
 * 
 * \return Значение типа uint8_t - это целое число в диапазоне от 0 до 255, записывается в заранее объявленную переменную такого же типа
 */
uint8_t str2int_color(String color, char start_ch, char end_ch, uint8_t row_ch)
{
    char ch_array[row_ch];
    /*
        Строка на вход выглядит, например, так: "(r123,". Индекс первого символа string[1] = 'r';
    индекс последнего символа string[5] = ','. А на выходе нужно получить десятичное число
    разрядносью от 1 до 3 (от единиц до сотен). т.е. при записи в пустой массив максимальное значение индекса может быть [2],
    а минимальное [0]. Поэтому уменьшим число символов на 2.
    */
    uint8_t count_ch = color.indexOf(end_ch, color.indexOf(start_ch)) - color.indexOf(start_ch) - 2;

    for (uint8_t k = color.indexOf(start_ch) + 1; k < color.indexOf(end_ch, k); k++)
    {
        uint8_t idx_ch = k - color.indexOf(start_ch) - 1;
        ch_array[idx_ch] = color[k];
    }
    return atoi(ch_array);
}

// Handle root url (/)
void handle_root()
{
    server.send(200, "text/html", HTML);
    // Время обновления веб-страницы
    delay(200);
}

void sending_rgb(){
    String rgb = server.arg("color"); // rgb = "(r123, g123, b123)"
    Serial.println("String data color from server: " + rgb);

    uint8_t r = str2int_color(rgb, 'r', ',', 3);
    uint8_t g = str2int_color(rgb, 'g', ',', 3);
    uint8_t b = str2int_color(rgb, 'b', ')', 3);

    String colors = "{\"id\":\"device_1\",\"data_type\":\"String\",\"Data\":\"rgb\",\"Value\": {\"r\":" + String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + "}}";
    server.send(200, "application/json", colors);
}

void sending_temp2() {
    String message = server.arg("plain"); // Получение данных из тела запроса
    if (message != ""){
        temp2 = message;
    }
    String response = "{\"id\":\"device_2\", \"data_type\":\"String\", \"Data\": \"temperature\", \"Value\":\"" + String(temp2) + "\"}";
    Serial.println(response);
    server.send(200, "application/json", response);
    delay(5000);
}

void sending_temp(){
    String temp = "{\"id\":\"device_1\", \"data_type\":\"String\", \"Data\": \"temperature\", \"Value\":\"" + String(temperatureRead()) + "\"}";
    server.send(200, "application/json", temp);
}


void setup()
{
    // Передача данных по Serial
    Serial.begin(115200);
    
#ifdef ESP_CLIENT_MODE
    // Подключение к существующей сети WiFI
    WiFi.begin(ssid, password);
    Serial.println("Connection to WiFi is completed");

    // Ручная конфигурация подключения по WiFi для изменения DNS
    MDNS.begin(nameDNS);
#endif

#ifdef ESP_STATION_MODE
    // Подключение wifi в режиме Station
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    // Вывод IP микроконтроллера в сети
    Serial.println("Create wifi station: " + String(ssid));
#endif
    // Полный код веб-страницы
    server.on("/", handle_root);

    // Регистрация событий (event)
    server.on("/data_rgb_1", sending_rgb);
    server.on("/data_temp_1", sending_temp);
    server.on("/data_temp_2",sending_temp2);
    
    // Запуск сервера
    server.begin();

#ifdef ESP_STATION_MODE
    Serial.println("Server was started");
#endif


}

void loop()
{
    // Принимать действия пользователя
    server.handleClient();
    update_temp_data();
}