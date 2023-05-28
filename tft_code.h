#include <TFT.h>  // Hardware-specific library
#include <SPI.h>
#include "Sensor.h"


#define screen_refresh_time 250                      //промежуток между обновлениями экрана, в миллисекундах (глобальный)
#define otladka_red_rectangle true                   //отладочный флаг для включения границ полей вывода символов для динамических данных
#define otladka_heater_on true                       //отладочный флаг для включения отображения состояния датчика CO2 (on/off)
#define get_sensors_value getValue                   //(как называется метод, отвечающий за получение значения)
#define get_sensors_value_LOW_border getСomfort_min  //(как называется метод, отвечающий за получение нижней границы оптимального значения)
#define get_sensors_value_HI_border getСomfort_max   //(как называется метод, отвечающий за получение верхней границы оптимального значения)
#define get_sensors_is_danger getDanger              //(как называется метод, отвечающий за получение состояния сенсора (тревога или нет))

//массив значений периодов обновлений страниц в миллисекундах:
//нулевой страницы нет.
//(1я и 2я страницы - по 3 параметра, далее - по одному пораметру на каждой из 6ти страниц)
int page_refresh_time[9] = { 0, 500, 2000, 250, 250, 500, 2000, 2000, 2000 };
char printout[5];  //нужная переменная для вывода значений параметров на экран (что-то вроде массива символов)

//массив, который хранит номера сенсоров в состоянии игнорирования:
//(именно сенсоров, а не страниц, так как обращаться к списку игнорированных параметров нужно будет чаще)
bool ignored_sensors_array[7] = { 0, false, false, false, false, false, false };


//======================================РАЗДЕЛ_ОПИСАНИЯ_КЛАССА_SCREEN:====================================== :



class SCREEN {
public:
  //КОНСТРУКТОР КЛАССА:
  SCREEN(byte page = 1, TFT* s = 0) {
    _p_TFT = s;
    _page = page;
    _time_stamp = millis();
  }


  //передача ссылок на объекты датчиков в память нашего объекта screen:
  void SET_SENSORS_DATA(SENSOR* SENSOR_ARRAY[7]) {
    _p_LUX = SENSOR_ARRAY[1];
    _p_PULS = SENSOR_ARRAY[2];
    _p_NOISE = SENSOR_ARRAY[3];
    _p_TEMP = SENSOR_ARRAY[4];
    _p_HUM = SENSOR_ARRAY[5];
    _p_CO2 = SENSOR_ARRAY[6];
  };


  //процедура проверки и обновления (или экстренного вывода) текущей страницы:
  void refresh();

  //установка страницы:
  void nextpage(byte direction);

  float getCurrentPage() {
    return _page;
  };

  ///отображение значка игнорирования значений текущего параметра (только для страниц 3-8):
  void Draw_ignor_sign(byte current_page, bool draw);


private:
  byte _page = 1;                         //текущая страница
  bool _nextpage = true;                  //флаг была ли изменена страница (было ли переключение на другую страницу)
  uint32_t _time_stamp = millis();        //время подследнего обновления для динамических параметров
  uint32_t _time_stamp_ignor = millis();  //время подследнего обновления для отображения значка игнорирования
  TFT* _p_TFT;                            //указатель на класс TFT, чтобы использовать его команды

  //ссылки на объекты датчиков для хранения данных с них:
  SENSOR* _p_LUX;
  SENSOR* _p_PULS;
  SENSOR* _p_NOISE;
  SENSOR* _p_TEMP;
  SENSOR* _p_HUM;
  SENSOR* _p_CO2;

  //переменные, которые будут использоваться внутри процедуры refresh для реализации отображения страниц экрана:
  String v_LUX, v_PULS, v_SHUM;
  String v_TEMP, v_HUM, v_GAS;

  //процедуры, которые будут использоваться внутри процедуры refresh для реализации отображения страниц экрана:

  void Static_PageDraw_1();  //отображение статической (текст) части страницы 1 (3 параметра: освещённость, коэф. пульсаций, уровень шума)
  void PageDraw_1();         //отображение динамической (значения) части страницы 1
  void Static_PageDraw_2();  //--- (3 параметра: температура, влажность, уровень CO2)
  void PageDraw_2();

  void Static_PageDraw_3();  //отображение статической части страницы 3 (1 параметр: освещенность)
  void PageDraw_3();         //отображение динамической части страницы 3
  void Static_PageDraw_4();  //--- (1 параметр: коэф. пульсаций)
  void PageDraw_4();         //---
  void Static_PageDraw_5();  //уровень шума
  void PageDraw_5();
  void Static_PageDraw_6();  //температура
  void PageDraw_6();
  void Static_PageDraw_7();  //влажность
  void PageDraw_7();
  void Static_PageDraw_8();  //уровень CO2
  void PageDraw_8();


  void Draw_value_borders();  //визуальное отображение положения текущего значения относительно установленных оптимальных границ

  void Draw_danger_sign(byte pos_x, byte pos_y);  //отображение значка того, что значение параметра вышло за установленные оптимальные границы
};



//======================================РАЗДЕЛ_ОСНОВНОЙ_ПРОЦЕДУРЫ_refresh()====================================== :



//процедура проверки и обновления (или экстренного вывода) текущей страницы:
void SCREEN::refresh() {
  if ((millis() - _time_stamp) < screen_refresh_time) {
    return;
  };

  switch (_page) {      //переход на текущую страницу для проверки необходимости обновления
    case 1:             //страница с 3 параметрами: освещённость, коэф.пульсаций, уровень шума.
      if (_nextpage) {  //если мы только что перешли на эту страницу, то 1 раз отрисовываем статичный текст:
        Static_PageDraw_1();
        _nextpage = false;
      }
      //переход в процедуру работы со страницей 1 для отрисовки динамического текста поверх статического:
      PageDraw_1();
      break;

    case 2:  //страница с 3 параметрами: температура, влажность, CO2.
      if (_nextpage) {
        Static_PageDraw_2();
        _nextpage = false;
      }
      PageDraw_2();
      break;

    case 3:  //страница с параметром 1: освещённость.
      if (_nextpage) {
        Static_PageDraw_3();
        _nextpage = false;
      }
      PageDraw_3();
      break;

    case 4:  //страница с параметром 2: коэффициент пульсаций.
      if (_nextpage) {
        Static_PageDraw_4();
        _nextpage = false;
      }
      PageDraw_4();
      break;

    case 5:  //страница с параметром 3: уровень шума.
      if (_nextpage) {
        Static_PageDraw_5();
        _nextpage = false;
      }
      PageDraw_5();
      break;

    case 6:  //страница с параметром 4: температура.
      if (_nextpage) {
        Static_PageDraw_6();
        _nextpage = false;
      }
      PageDraw_6();
      break;

    case 7:  //страница с параметром 5: влажность.
      if (_nextpage) {
        Static_PageDraw_7();
        _nextpage = false;
      }
      PageDraw_7();
      break;

    case 8:  //страница с параметром 6: CO2.
      if (_nextpage) {
        Static_PageDraw_8();
        _nextpage = false;
      }
      PageDraw_8();
      break;

    default:
      //выход по умолчанию, на всякий случай
      break;
  };
};



//======================================РАЗДЕЛ_ОСНОВНЫХ_ПРОЦЕДУР_И_ФУНКЦИЙ_(не refresh())====================================== :



//ПРОЦЕДУРА УСТАНОВКИ СТРАНИЦЫ:
void SCREEN::nextpage(byte direction) {
  _page = _page + direction;  //для простоты реализации, движение может быть как влево, так и вправо, direction = 1 или -1
  if (_page >= 9) {
    _page = 1;
  };
  if (_page <= 0) {
    _page = 8;
  };
  _nextpage = true;
  (*_p_TFT).background(0, 0, 0);  //очистка дисплея нужна только при переходе на новую страницу
};



//ПРОЦЕДУРА ОТРИСОВКИ ЗНАЧКА, ОТОБРААЖЮЩЕГО ТЕКУЩЕЕ ЗНАЧЕНИЕ И ЕГО ПОЛОЖЕНИЕ ОТНОСИТЕЛЬНО КОМФОРТНЫХ ГРАНИЦ:
void SCREEN::Draw_value_borders() {
  //код процедуры
  return;
};



//ПРОЦЕДУРА ОТРИСОВКИ ЗНАЧКА ТРЕВОГИ(выход значения параметра за пределы комфортной зоны):
void SCREEN::Draw_danger_sign(byte pos_x, byte pos_y) {
  byte tr_side = 20;
  (*_p_TFT).stroke(255, 255, 0);
  byte x = pos_x;
  byte y = pos_y;
  /*
  while (x<(pos_x+tr_side)){
    (*_p_TFT).line(x, y+tr_side, x+(tr_side/2), y);
    x = x + 1;
    y = y + 1;
  };
  */
  (*_p_TFT).line(x, y + tr_side, x + (tr_side / 2), y);
  (*_p_TFT).line(x + (tr_side / 2), y, x + tr_side, y + tr_side);
  (*_p_TFT).line(x + tr_side, y + tr_side, x, y + tr_side);
  (*_p_TFT).stroke(255, 0, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("!", x + 5, y + 5);
  (*_p_TFT).text("!", x + 6, y + 5);
  return;
}



//ПРОЦЕДУРА ОТОБРАЖЕНИЯ ЗНАЧКА ИГНОРИРОВАНИЯ ЗНАЧЕНИЙ ТЕКУЩЕГО ПАРАМЕТРА (только для страниц 3-8 (только ли?) ):
void SCREEN::Draw_ignor_sign(byte current_page, bool draw) {
  if ((millis() - _time_stamp_ignor) < screen_refresh_time) {
    return;
  };
  if (current_page == _page) {
    if (draw) {
      //рисуем красный восклицательный знак:
      (*_p_TFT).stroke(255, 0, 0);
      (*_p_TFT).setTextSize(3);
      (*_p_TFT).text("!", 140, 100);

      //меняем цвет текста обратно на белый:
      (*_p_TFT).stroke(255, 255, 255);

      ignored_sensors_array[current_page - 2] = true;  //не выводить сенсор, страницу которого заблокировали
    } else {
      (*_p_TFT).stroke(0, 0, 0);
      (*_p_TFT).setTextSize(3);
      (*_p_TFT).text("!", 140, 100);

      //меняем цвет текста обратно на белый:
      (*_p_TFT).stroke(255, 255, 255);

      ignored_sensors_array[current_page - 2] = false;  //снятие блокировки вывода сенсора, страницу которого заблокировали
    }
  }
  _time_stamp_ignor = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_1================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 1:
void SCREEN::Static_PageDraw_1() {
  //Теория для используемых команд TFT:
  //Текст рисуется в прямоугольной области. Область выводимого окна текста рисуется слева направо сверху вниз.
  //Эта левая верхняя опорная точка указывается относительно координат экрана (левый верхний угол x;y = 0;0)).

  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("1", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);            //шрифт текста дальше =фиолетовый
  (*_p_TFT).setTextSize(1);                 //размер текста дальше =1
  (*_p_TFT).text("Osveschennost`:", 0, 0);  //вывод текста "Osveschennost`:" с отступом от левого края = 0 и от верхнего = 0
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("Lux", 78, 18);  //вывод текста "Lux" с отступом от левого края = 78 и от верхнего = 20
  Draw_danger_sign(138, 15);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Koefficient Pul`saciy:", 0, 45);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 78, 64);
  Draw_danger_sign(138, 60);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Uroven` Shuma:", 0, 90);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("dB", 78, 109);
  Draw_danger_sign(138, 105);


  //При первой отрисовке статичного текста нужно начать рисовать динамический, так как иначе для медленных параметров
  //будет долгое ожидание появления динамических параметров:
  _time_stamp = page_refresh_time[_page] + 100;  //а это чтобы, попав в процедуру, текст рисовался сразу (условие соблюдено было сразу)
  PageDraw_1();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 1:
void SCREEN::PageDraw_1() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);      //цвет заливки - чёрный
  (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };


  //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
  if (ignored_sensors_array[1] == false) {  //если сенсор не в списке игнорируемых, то тогда очищаем
    (*_p_TFT).rect(0, 15, 69, 21);          //для параметра 1
  }
  if (ignored_sensors_array[2] == false) {
    (*_p_TFT).rect(0, 60, 69, 21);  //для параметра 2
  }
  if (ignored_sensors_array[3] == false) {
    (*_p_TFT).rect(0, 105, 69, 21);  //для параметра 3
  }



  (*_p_TFT).setTextSize(3);
  if (ignored_sensors_array[1] == false) {  //если сенсор не в списке игнорируемых, то тогда получаем новое значение
    //создание строковой переменной с указанием количества знаков после запятой (0 - нет.):
    v_LUX = String((*_p_LUX).get_sensors_value(), 0);
  }
  if (ignored_sensors_array[2] == false) {
    v_PULS = String((*_p_PULS).get_sensors_value(), 1);
  }
  if (ignored_sensors_array[3] == false) {
    v_SHUM = String((*_p_NOISE).get_sensors_value(), 1);
  }



  (*_p_TFT).stroke(255, 255, 255);  //"включаем" (цвет текста =белый) весь динамический текст

  //значение сенсора выводим в любом случае, независимо от игнорирования параметра.
  //таким образом будет выводиться последнее полученное значение
  v_LUX.toCharArray(printout, 5);   //5 - количество символов (5-1=4, 4 символа выводится)
  (*_p_TFT).text(printout, 0, 15);  //вывести text по координатам x=0 от левого края и y=15 от верхнего
  v_PULS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 60);
  v_SHUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 105);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_2================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 2:
void SCREEN::Static_PageDraw_2() {

  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("2", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Temperatura:", 0, 0);
  //сначала рисуем градусы ("°"). Так как такой символ не поддерживается, рисуем универсально - с помощью буквы "о" наименьшего размера:
  (*_p_TFT).setTextSize(0);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("o", 78, 17);
  //далее рисуем символ "C" большего размера. В итоге получается "°C":
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("C", 84, 19);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Vlazjnost`:", 0, 45);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 78, 64);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Uroven` CO2:", 0, 90);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("ppm", 78, 107);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_2();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 2:
void SCREEN::PageDraw_2() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);      //цвет заливки - чёрный
  (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };


  //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
  if (ignored_sensors_array[4] == false) {
    (*_p_TFT).rect(0, 15, 69, 21);  //для параметра 1
  }
  if (ignored_sensors_array[5] == false) {
    (*_p_TFT).rect(0, 60, 69, 21);  //для параметра 2
  }
  if (ignored_sensors_array[6] == false) {
    (*_p_TFT).rect(0, 105, 69, 21);  //для параметра 3
  }



  (*_p_TFT).setTextSize(3);
  if (ignored_sensors_array[4] == false) {
    //создание строковой переменной с указанием количества знаков после запятой:
    v_TEMP = String((*_p_TEMP).get_sensors_value(), 1);
  }
  if (ignored_sensors_array[5] == false) {
    v_HUM = String((*_p_HUM).get_sensors_value(), 1);
  }
  if (ignored_sensors_array[6] == false) {
    v_GAS = String((*_p_CO2).get_sensors_value(), 0);
  }



  (*_p_TFT).stroke(255, 255, 255);  //"включаем" (цвет текста =белый) весь динамический текст

  //значение сенсора выводим в любом случае, независимо от игнорирования параметра.
  //таким образом будет выводиться последнее полученное значение
  v_TEMP.toCharArray(printout, 5);  //5 - количество символов (5-1=4, 4 символа выводится)
  (*_p_TFT).text(printout, 0, 15);  //вывести text по координатам x=0 от левого края и y=15 от верхнего
  v_HUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 60);
  v_GAS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 105);



  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_3================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 3:
void SCREEN::Static_PageDraw_3() {
  //теория для используемых команд TFT:
  //Текст рисуется в прямоугольной области. Область выводимого окна текста рисуется слева направо сверху вниз.
  //Эта левая верхняя опорная точка указывается относительно координат экрана (левый верхний угол x;y = 0;0)).

  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("3", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Osveschen-", 0, 0);
  (*_p_TFT).text("nost`:", 0, 17);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("Lux", 100, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_3();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 3:
void SCREEN::PageDraw_3() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);      //цвет заливки - чёрный
  (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[1] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[1] == false) {
    v_LUX = String((*_p_LUX).get_sensors_value(), 0);
  }


  (*_p_TFT).stroke(255, 255, 255);
  v_LUX.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_4================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 4:
void SCREEN::Static_PageDraw_4() {
  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("4", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Koefficient", 0, 0);
  (*_p_TFT).text("Pul`saciy:", 0, 17);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 100, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_4();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 4:
void SCREEN::PageDraw_4() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);
  (*_p_TFT).stroke(255, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[2] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[2] == false) {
    v_PULS = String((*_p_PULS).get_sensors_value(), 1);
  }



  (*_p_TFT).stroke(255, 255, 255);
  v_PULS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_5================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 5:
void SCREEN::Static_PageDraw_5() {
  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("5", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Uroven`", 0, 0);
  (*_p_TFT).text("Shuma:", 0, 17);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 100, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_5();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 5:
void SCREEN::PageDraw_5() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);
  (*_p_TFT).stroke(255, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[3] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[3] == false) {
    v_SHUM = String((*_p_NOISE).get_sensors_value(), 1);
  }


  (*_p_TFT).stroke(255, 255, 255);
  v_SHUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_6================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 6:
void SCREEN::Static_PageDraw_6() {
  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("6", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Temperatura:", 0, 17);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("o", 100, 39);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).text("C", 111, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_6();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 6:
void SCREEN::PageDraw_6() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);
  (*_p_TFT).stroke(255, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[4] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }



  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[4] == false) {
    v_TEMP = String((*_p_TEMP).get_sensors_value(), 1);
  }



  (*_p_TFT).stroke(255, 255, 255);
  v_TEMP.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_7================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 7:
void SCREEN::Static_PageDraw_7() {
  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("7", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Vlazjnost`:", 0, 17);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 100, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_7();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 7:
void SCREEN::PageDraw_7() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);
  (*_p_TFT).stroke(255, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[5] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[5] == false) {
    v_HUM = String((*_p_HUM).get_sensors_value(), 1);
  }


  (*_p_TFT).stroke(255, 255, 255);
  v_HUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//================================================РАЗДЕЛ_ОТРИСОВКИ_СТРАНИЦЫ_8================================================ :



//ПРОЦЕДУРА ОТРИСОВКИ СТАТИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 8:
void SCREEN::Static_PageDraw_8() {
  //зелёная цифра текущей страницы в правом верхнем углу:
  (*_p_TFT).stroke(0, 255, 0);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("8", 150, 0);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).text("Uroven` CO2:", 0, 17);
  (*_p_TFT).setTextSize(3);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("ppm", 100, 43);

  _time_stamp = page_refresh_time[_page] + 100;
  PageDraw_8();
};



//ПРОЦЕДУРА ОТРИСОВКИ ДИНАМИЧЕСКОГО ТЕКСТА СТРАНИЦЫ 8:
void SCREEN::PageDraw_8() {
  if ((millis() - _time_stamp) < page_refresh_time[_page]) {  //если ещё не пришло время рисовать, выходим
    return;
  };

  (*_p_TFT).fill(0, 0, 0);
  (*_p_TFT).stroke(255, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[6] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[6] == false) {
    v_GAS = String((*_p_CO2).get_sensors_value(), 0);
  }


  (*_p_TFT).stroke(255, 255, 255);
  v_GAS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);


  if (otladka_heater_on) {          //если включён вывод состояния датчика CO2 (on/off)
    if ((*_p_CO2).getHeaterON()) {  //если датчик включён
      //рисуем красный круг:
      (*_p_TFT).fill(255, 0, 0);
      (*_p_TFT).stroke(255, 0, 0);
      (*_p_TFT).circle(140, 90, 10);
      //белый текст внутри: OFF:
      (*_p_TFT).stroke(255, 255, 255);
      (*_p_TFT).setTextSize(1);
      (*_p_TFT).text("OFF", 132, 88);
    } else {  //если датчик выключен
      //рисуем зелёный круг:
      (*_p_TFT).fill(0, 255, 0);
      (*_p_TFT).stroke(0, 255, 0);
      (*_p_TFT).circle(140, 90, 10);
      //белый текст внутри: ON:
      (*_p_TFT).stroke(255, 255, 255);
      (*_p_TFT).setTextSize(1);
      (*_p_TFT).text("ON", 135, 88);
    };
    (*_p_TFT).fill(0, 0, 0);
    (*_p_TFT).stroke(255, 255, 255);
  }


  //здесь должен быть вызов процедуры отрисовки значка тревоги, если параметр вышел за оптимальные границы значений.
  //здесь должен быть вызов процедуры отрисовки значка уровня отклонения параметра от оптимального.

  _time_stamp = millis();
};



//=========================================================КОНЕЦ_РАЗДЕЛОВ=========================================================
