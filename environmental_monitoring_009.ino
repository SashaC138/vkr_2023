#define otladka_serial_print false            //включение вывода отладочной информации по всем сенсорам в "serial print".
#define otladka_serial_print_ignor false      //включение вывода отладочной информации по всем сенсорам в "serial print ignor".
#define otladka_serial_print_button false     //включение вывода отладочной информации о кнопке и страницах в "serial print".
#define otladka_serial_print_time_stamp 3000  //константа времени вывода отладочной информации по всем сенсорам в "serial print".
uint32_t time_stamp_otladka = millis();       //время между выводами отладочной информации по всем сенсорам в "serial print".
uint32_t time_stamp_otladka_ignor = millis(); //время между выводами отладочной информации по всем сенсорам в "serial print ignor".


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
//#include <TroykaMQ.h>
#include <MQ135.h>  // имя для пина, к которому подключен нагреватель датчика:
#define PIN_MQ135_HEATER 2
#define RZERO_VALUE_MQ135 30.66
// имя для пина, к которому подключен датчик:
#define PIN_MQ135 A5
//инициализация датчика:
MQ135 mq135(PIN_MQ135, RZERO_VALUE_MQ135);




//подключение пользовательской библиотеки работы со светодиодом. Инициализация светодиода:
#include "led.h"
#define led_modes_refresh_time 500  //как часто способны меняться режимы светодиода
LED myLed(6, LED_SIN);
uint32_t time_stamp_led = millis();
byte danger_counter = 0;  //сколько danger-ов накопилось среди сенсоров



//для работы кнопки:
#define KEY_PRESSED 3  //кнопка для переключения страниц
#define set_button_short_time 100
#define set_button_long_time 3000
#define button_release_time 100
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
bool ignored_pages_array[9] = { 0, 0, 0, false, false, false, false, false, false };

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

  //для "пробуждения" порта для чтения (ещё нужно?):

  byte i = 0;
  for (i = 0; i < 100; i++) {
    short int bufer = analogRead(PIN_MQ135);
  };

  Serial.begin(9600);
}






//процедура вывода отладочной информации по сенсорам:
void print_otladka_info_sensors(SENSOR* obj) {

  //отладка:
  Serial.print("Type = ");
  byte type = (*obj).getType();
  switch (type) {
    case 0:
      Serial.print("SEN_PULSE");
      break;

    case 1:
      Serial.print("SEN_LUX  ");
      break;

    case 2:
      Serial.print("SEN_NOISE");
      break;

    case 3:
      Serial.print("SEN_TEMP ");
      break;

    case 4:
      Serial.print("SEN_HUM  ");
      break;

    case 5:
      Serial.print("SEN_CO2  ");
      break;

    default:
      Serial.print("(UNKNOWN)");
      break;
  };
  Serial.print(" Ready = ");
  Serial.print((*obj).getReady());
  Serial.print(" Danger = ");
  Serial.print((*obj).getDanger());
  Serial.print(" Value = ");
  Serial.println((*obj).getValue());


  if ((*obj).getType() == SEN_CO2) {
    /*
    float rzero = mq135_sensor.getRZero();
    float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, humidity);
    float resistance = mq135_sensor.getResistance();
    float ppm = mq135_sensor.getPPM();
    float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);
  
    Serial.print("MQ135 RZero: ");
    Serial.print(rzero);
    Serial.print("\t Corrected RZero: ");
    Serial.print(correctedRZero);
    Serial.print("\t Resistance: ");
    Serial.print(resistance);
    Serial.print("\t PPM: ");
    Serial.print(ppm);
    Serial.print("ppm");
    Serial.print("\t Corrected PPM: ");
    Serial.print(correctedPPM);
    Serial.println("ppm");
    */
  }
}




//Процедура для проверки состояния кнопки и работы с страницами (переключение+игнор страниц)
void checkButton_and_setPage(bool btnState, byte current_page) {

  /*
  if ((otladka_serial_print_button == true) && (millis() - time_stamp_otladka > otladka_serial_print_time_stamp)) {
    //отладка работы кнопки:
    Serial.print("btnState = ");
    Serial.print(btnState);
    Serial.print("; flag = ");
    Serial.print(flag);
    Serial.print("; flag_short = ");
    Serial.print(flag_short);
    Serial.print("; flag_long = ");
    Serial.print(flag_long);
    Serial.print("; (millis() - btnTimer) = ");
    Serial.println((millis() - btnTimer));
    Serial.print("1current_page = ");
    Serial.println(current_page);
    Serial.println("");
    time_stamp_otladka = millis();
  }
  */

  if ((btnState) && (!flag) && ((millis() - btnTimer) > set_button_short_time)) {
    flag = true;
    flag_short = true;
    btnTimer = millis();
    //Serial.println("press");
  }
  if ((btnState) && (flag) && (flag_long) && ((millis() - btnTimer) > set_button_long_time)) {
    flag_short = false;
    if ((current_page >= 3) && ((ignored_pages_array[current_page]) == false)) {
      ignored_pages_array[current_page] = true;
    } else {
      ignored_pages_array[current_page] = false;
    };
    flag_long = false;
    btnTimer = millis();
    //Serial.println("press hold");
  }



  if ((!btnState) && (flag) && ((millis() - btnTimer) > button_release_time)) {
    flag = false;
    if (flag_short) {
      //временно, для переключения страниц:
      myScreen.nextpage(1);
      flag_short = false;
    };
    flag_long = true;
    btnTimer = millis();
    //Serial.println("release");
  };
}










void loop() {

  if (ignored_pages_array[current_page] == 1) {
    myScreen.Draw_ignor_sign(current_page, 1);  //рисуем значок игнора
  } else {
    myScreen.Draw_ignor_sign(current_page, 0);  //убираем значок игнора
  }
  myScreen.refresh();

  for (byte i = 1; i <= 6; i++) {
    SENSOR* t = SENSOR_ARRAY[i];
    (*t).refresh();  //запускает работу текущего сенсора
  };

  
  //вывод отладочной информации по всем сенсорам:
  if ((otladka_serial_print == true) && (millis() - time_stamp_otladka > otladka_serial_print_time_stamp)) {
    for (byte i = 1; i <= 6; i++) {
      print_otladka_info_sensors(SENSOR_ARRAY[i]);
    };
    Serial.println("-------------------------------------------------------");
    Serial.println("");
    time_stamp_otladka = millis();
  }


  //обработка переключения состояний светодиода:
  if ((millis() - time_stamp_led) < led_modes_refresh_time) {
    //return;  //не надо слишком часто менять режимы светодиода
  } else {

    for (byte i = 1; i <= 6; i++) {  //считаем количество сенсоров, у которых значение вышло за допустимые пределы
      if ((*SENSOR_ARRAY[i]).getDanger()) {
        //Serial.print("1ignored_pages_array[i+2]: ");
        //Serial.println(ignored_pages_array[i + 2]);
        if (!(ignored_pages_array[i + 2]) && ((*SENSOR_ARRAY[i]).getReady())) {
          //Serial.print("2ignored_pages_array[i+2]: ");
          //Serial.println(ignored_pages_array[i + 2]);
          danger_counter = danger_counter + 1;
        }
      }
    };
    
    
    if ((otladka_serial_print_ignor == true) && (millis() - time_stamp_otladka_ignor > otladka_serial_print_time_stamp)) {
    for (byte i = 1; i <= 8; i++) {
      Serial.print(i);
      Serial.print(": ignor:");
      Serial.print(ignored_pages_array[i]);
      Serial.print(" ready:");
      Serial.print((*SENSOR_ARRAY[i]).getReady());
      Serial.print(" danger:");
      Serial.println((*SENSOR_ARRAY[i]).getDanger());
    };
    Serial.println("///////////");
    Serial.print("danger_counter: ");
    Serial.println(danger_counter);
    Serial.println("/////////////////////////////////");

    time_stamp_otladka_ignor = millis();
    }
    
    
    if (danger_counter > 0) {  //если насчитали больше нуля, то переводим светодиод в режим DANGER
      myLed.setMode(LED_DANGER, danger_counter);
    } else {
      myLed.setMode(LED_BLINK);
    };

    danger_counter = 0;  //счётчик обнуляем, зайдём проверить снова через время led_modes_refresh_time
    time_stamp_led = millis();
  };

  myLed.refresh();



  //условия проверки состояний и времени кнопки для переключения страниц:
  bool btnState = !digitalRead(KEY_PRESSED);
  current_page = myScreen.getCurrentPage();
  checkButton_and_setPage(btnState, current_page);


  signed char t = encoder_1.check_and_get();
  if (!(t == 0)) {
    //Serial.println(t);
    myScreen.nextpage(t);
  };
}
