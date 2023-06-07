#include "tft_code.h"  //пользовательская библиотека для работы с дисплеем
//
//в tht_code.h также входят:
//#include <TFT.h>  // Hardware-specific library
//#include <SPI.h>
//#include "Sensor.h" //пользовательская библиотека работы со всеми экземплярами класса "сенсор"
//
//
//переменные для дисплея:
#define CS 10
#define DC 9
#define RESET 8
#define screen_page 1  //номер страницы, с которой инициализируется дисплей
//
//инициализация дисплея:
TFT newScreen = TFT(CS, DC, RESET);
SCREEN myScreen(screen_page, &newScreen);


//Подключение датчика DHT 11:
#include "DHT.h"
#define DHTPIN 5       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);



//Подключение датчика MQ135:
#include "MQ135plus.h"  // имя для пина, к которому подключен нагреватель датчика:
#define PIN_MQ135_HEATER 2
//#define RZERO_VALUE_MQ135 30.66
#define RZERO_VALUE_MQ135 150
// имя для пина, к которому подключен датчик:
#define PIN_MQ135 A5
//инициализация датчика:
MQ135plus mq135(PIN_MQ135, RZERO_VALUE_MQ135);




//подключение пользовательской библиотеки работы со светодиодом. Инициализация светодиода:
#include "led.h"
#define led_modes_refresh_time 500  //как часто способны меняться режимы светодиода
#define led_sin_time 30 * 1000      //время режима "синусоида" или полное время загрузки устройства - за 30 сек. в рабочее состояние переходит mq135
LED myLed(6, LED_SIN);
uint32_t time_stamp_led = millis();
uint32_t time_stamp_led_sin = millis();
byte danger_counter = 0;  //сколько danger-ов накопилось среди сенсоров
byte danger_counter_save = 0;



//для работы кнопки:
#define KEY_PRESSED 3  //кнопка для переключения страниц
#define set_button_short_time 100
#define set_button_long_time 2000  //из-за задержек при обработке получается на секунду больше, поэтому ставим 2 секунды
#define button_release_time 100
#define button_can_set_next_page true  // включить(true)/выключить(false) переключение страниц по короткому нажатию кнопки
//
bool flag = false;
uint32_t btnTimer = 0;
bool flag_short = false;  //флаг "было быстрое нажатие"
bool flag_long = false;   //флаг "было долгое нажатие"


//для работы энкодера:
#include "keys_control.h"
#define L A1
#define R A2

ENCODER encoder_1(L, R);


//Общие переменные и массивы:

uint32_t time_stamp = millis();


SENSOR mySensPULSE(SEN_PULSE, 1, 5, 0, 15, 0, 0, 0, 0, 0);
SENSOR* P = &mySensPULSE;
SENSOR mySensLUX(SEN_LUX, 2, 100, 200, 400, 0, 0, P, 0, 0);
SENSOR mySensNOISE(SEN_NOISE, 500, 0, 0, 80, 0, 0, 0, 0, 0);
SENSOR mySensTEMP(SEN_TEMP, 2000, 0, 21.0, 25.0, &dht, 0, 0, 0, 0);
SENSOR* T = &mySensTEMP;
SENSOR mySensHUM(SEN_HUM, 2000, 0, 15.0, 75.0, &dht, 0, 0, 0, 0);
SENSOR* H = &mySensHUM;
SENSOR mySensCO2(SEN_CO2, 2000, 5, 380.0, 1000.0, 0, &mq135, 0, T, H);

SENSOR* SENSOR_ARRAY[] = { 0, &mySensLUX, &mySensPULSE, &mySensNOISE, &mySensTEMP, &mySensHUM, &mySensCO2 };
//SENSOR* SENSOR_ARRAY[7];




//значения каких страниц игнорируются:
//нулевой страницы не существует;
//1я и 2я страницы не блокируются
bool ignored_pages_array[9] = { 0, 0, false, false, false, false, false, false, false };
bool flag_screen_off = false;  //для блокировки экрана в целом (чёрный экран)
byte current_page = 1;





void setup() {

  newScreen.begin();
  //нам нужно 1 раз взять сслыки на датчики, чтобы потом обращаться к ним за данными:
  myScreen.SET_SENSORS_DATA(SENSOR_ARRAY);
  newScreen.background(0, 0, 0);

  dht.begin();

  pinMode(PIN_MQ135_HEATER, OUTPUT);

  pinMode(KEY_PRESSED, INPUT_PULLUP);

  //вручную выставляем флаг нагрева на датчик в "1"(нада греца):
  pinMode(PIN_MQ135_HEATER, OUTPUT);
  digitalWrite(PIN_MQ135_HEATER, HIGH);

  //для "пробуждения" порта для чтения

  byte i = 0;
  for (i = 0; i < 100; i++) {
    short int bufer = analogRead(PIN_MQ135);
  };

  Serial.begin(9600);
}



//Процедура для проверки состояния кнопки и работы с страницами (переключение+игнор страниц)
void checkButton_and_setPage(bool btnState, byte current_page) {

  if ((btnState) && (!flag) && ((millis() - btnTimer) > set_button_short_time)) {
    flag = true;
    flag_short = true;
    btnTimer = millis();
    //Serial.println("press");
  }
  if ((btnState) && (flag) && (flag_long) && ((millis() - btnTimer) > set_button_long_time)) {
    flag_short = false;
    if ((ignored_pages_array[2] == true)) {
      ignored_pages_array[2] = false;
      flag_screen_off = false;
      myScreen.nextpage(1);
      myScreen.nextpage(-1);
    } else if ((current_page >= 3) && ((ignored_pages_array[current_page]) == false)) {
      ignored_pages_array[current_page] = true;
    } else if ((current_page >= 3) && ((ignored_pages_array[current_page]) == true)) {
      ignored_pages_array[current_page] = false;
    } else if ((current_page == 2) && ((ignored_pages_array[2]) == false)) {
      ignored_pages_array[2] = true;
    }
    flag_long = false;
    btnTimer = millis();
    //Serial.println("press hold");
  }

  if ((!btnState) && (flag) && ((millis() - btnTimer) > button_release_time)) {
    flag = false;
    if (flag_short) {
      //"резервное" переключение страниц:
      if (button_can_set_next_page) {  //если флагом разрешили переключать страницы кнопкой
        myScreen.nextpage(1);
      }
      flag_short = false;
    };
    flag_long = true;
    btnTimer = millis();
    //Serial.println("release");
  };
}





long count1 = 0;
long timestamp1 = millis();

void loop() {

  if ((ignored_pages_array[current_page] == 1) && (current_page >= 3) && ((ignored_pages_array[2] == 0))) {
    myScreen.Draw_ignor_sign(current_page, 1);  //рисуем значок игнора
  } else if ((ignored_pages_array[current_page] == 0) && (current_page >= 3)) {
    myScreen.Draw_ignor_sign(current_page, 0);  //убираем значок игнора
  } else if ((ignored_pages_array[current_page] == 1) && (current_page == 2) && (flag_screen_off == false)) {
    flag_screen_off = true;
  }


  if ((millis() - timestamp1) < 1000) {
    count1 = count1 + 1;
  } else {
    Serial.println(count1);
    count1 = 0;
    timestamp1 = millis();
  };


  for (byte i = 1; i <= 6; i++) {
    SENSOR* t = SENSOR_ARRAY[i];
    (*t).refresh();  //запускает работу текущего сенсора
  };

  //обработка переключения состояний светодиода:
  if ((millis() - time_stamp_led) < led_modes_refresh_time) {
    //не надо слишком часто менять режимы светодиода
  } else {

    for (byte i = 1; i <= 6; i++) {  //считаем количество сенсоров, у которых значение вышло за допустимые пределы
      if ((*SENSOR_ARRAY[i]).getDanger()) {
        if (!(ignored_pages_array[i + 2]) && ((*SENSOR_ARRAY[i]).getReady())) {
          danger_counter = danger_counter + 1;
        }
      }
    };
    danger_counter_save = danger_counter;

    if (danger_counter > 0) {  //если насчитали больше нуля, то переводим светодиод в режим DANGER
      myLed.setMode(LED_DANGER, danger_counter);
    }

    if (danger_counter == 0) {
      myLed.setMode(LED_BLINK);
    };

    if ((millis() - time_stamp_led_sin) < led_sin_time) {
      myLed.setMode(LED_SIN);
    } else if (danger_counter == 0) {
      myLed.setMode(LED_BLINK);
    };


    danger_counter = 0;  //счётчик обнуляем, зайдём проверить снова через время led_modes_refresh_time
    time_stamp_led = millis();
  };

  myLed.refresh();


  if (ignored_pages_array[2] == false) {
    myScreen.refresh();
  } else if ((ignored_pages_array[2] == true) && (danger_counter_save == 0)) {
    if (flag_screen_off == true) {
      newScreen.background(0, 0, 0);
      flag_screen_off = false;
    }
  } else if ((ignored_pages_array[2] == true) && (danger_counter_save > 0)) {
    ignored_pages_array[2] = false;
    flag_screen_off = false;
    myScreen.nextpage(1);
    myScreen.nextpage(-1);
    myScreen.refresh();
  }

  //условия проверки состояний и времени кнопки для переключения страниц:
  bool btnState = !digitalRead(KEY_PRESSED);
  current_page = myScreen.getCurrentPage();
  checkButton_and_setPage(btnState, current_page);


  signed char t = encoder_1.check_and_get();

  if (!(t == 0)) {
    myScreen.nextpage(t);
  };
}
