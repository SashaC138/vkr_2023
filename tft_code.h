#include <TFT.h>  // Hardware-specific library
#include <SPI.h>
#include "Sensor.h"


#define screen_refresh_time 250                      //промежуток между обновлениями экрана, в миллисекундах (глобальный)
#define otladka_red_rectangle true                   //отладочный флаг для включения границ полей вывода символов для динамических данных
#define otladka_heater_on true                       //отладочный флаг для включения отображения состояния датчика CO2 (on/off)
#define get_sensors_value getValue                   //(как называется метод, отвечающий за получение значения)
#define get_sensors_value_LOW_border getComfortMin   //(как называется метод, отвечающий за получение нижней границы оптимального значения)
#define get_sensors_value_HIGH_border getComfortMax  //(как называется метод, отвечающий за получение верхней границы оптимального значения)
#define get_sensors_is_danger getDanger              //(как называется метод, отвечающий за получение состояния сенсора (тревога или нет))

//массив значений периодов обновлений страниц в миллисекундах:
//нулевой страницы нет.
//(1я и 2я страницы - по 3 параметра, далее - по одному пораметру на каждой из 6ти страниц)
int page_refresh_time[9] = { 0, 500, 2000, 250, 250, 500, 2000, 2000, 2000 };
char printout[5];  //нужная переменная для вывода значений параметров на экран (что-то вроде массива символов)


//массив, хранящий верхние границы оптимальных значений сенсоров:
float sensors_high_border[7] = { -0, 0, 0, 0, 0, 0, 0 };

//массив, хранящий нижние границы оптимальных значений сенсоров:
float sensors_low_border[7] = { -0, 0, 0, 0, 0, 0, 0 };


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

    //заполнение массивов верхних и нижних границ оптимальных значений параметров:
    for (byte i = 1; i <= 6; i++) {
      sensors_high_border[i] = (*SENSOR_ARRAY[i]).get_sensors_value_HIGH_border();
      sensors_low_border[i] = (*SENSOR_ARRAY[i]).get_sensors_value_LOW_border();
    }
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
  float v_LUX, v_PULS, v_NOISE;
  float v_TEMP, v_HUM, v_CO2;

  String str_v_LUX, str_v_PULS, str_v_NOISE;
  String str_v_TEMP, str_v_HUM, str_v_CO2;

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


  void Draw_value_borders(byte pos_x, byte pos_y, float comfort_min, float comfort_max, float value, byte direction_number);  //визуальное отображение положения текущего значения относительно установленных оптимальных границ

  void Draw_danger_sign(byte pos_x, byte pos_y, bool draw_sign);  //отображение значка того, что значение параметра вышло за установленные оптимальные границы
};



//======================================РАЗДЕЛ_ОСНОВНОЙ_ПРОЦЕДУРЫ_refresh()====================================== :



//процедура проверки и обновления (или экстренного вывода) текущей страницы:
void SCREEN::refresh() {
  if ((millis() - _time_stamp) < screen_refresh_time) {
    return;
  };
  //Serial.println("Refresh time!");

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



//ПРОЦЕДУРА ОТРИСОВКИ ЗНАЧКА, ОТОБРАЖАЮЩЕГО ТЕКУЩЕЕ ЗНАЧЕНИЕ И ЕГО ПОЛОЖЕНИЕ ОТНОСИТЕЛЬНО ОПТИМАЛЬНЫХ ГРАНИЦ:
void SCREEN::Draw_value_borders(byte pos_x, byte pos_y, float comfort_min, float comfort_max, float value, byte direction_number) {

  byte tr_side = 20;

  byte x = 0;
  byte y = 0;
  byte step = 0;
  byte color_g = 250;
  byte color_r = 0;
  float center = 0;

  if (_page >= 3) {
    tr_side = 40;
  }

  (*_p_TFT).fill(0, 0, 0);                                     //цвет заливки - чёрный
  (*_p_TFT).stroke(0, 0, 0);                                   //цвет границы - чёрный
  (*_p_TFT).rect(pos_x - 1, pos_y, tr_side + 3, tr_side + 1);  //чёрный квадрат на месте знака danger

  //отрисовка точки посередине:
  (*_p_TFT).fill(255, 255, 255);
  (*_p_TFT).stroke(255, 255, 255);
  if (direction_number == 1) {
    (*_p_TFT).circle(pos_x + (tr_side / 2), pos_y + tr_side, 2);
  } else if (direction_number == 2) {
    (*_p_TFT).circle(pos_x + (tr_side / 2), pos_y + (tr_side / 2), 2);
  }

  if (direction_number == 2) {  //если будет рисоваться иконка "двунаправленной" возможности изменения
    center = (comfort_min + comfort_max) / 2;
  } else if (direction_number == 1) {  //если будет рисоваться иконка "одноправленной" возможности изменения (нижняя граница всегда 0)
    center = comfort_min;
  }


  if (value > center) {
    step = ceil((value - center) / ((comfort_max - center) / (tr_side / 2)));

    if (direction_number == 1) {
      if (step > (tr_side / 2)) {
        step = (tr_side / 2);
      };
      while (x <= (step - 1)) {
        (*_p_TFT).stroke(color_r, color_g, 0);
        if (x < (5 * (tr_side / 20))) {
          color_r = color_r + (25 * (40 / tr_side));  //за 5(10) шагов до 250
        } else {
          color_g = color_g - (25 * (40 / tr_side));  //за 5(10) шагов до 0
        };
        (*_p_TFT).line(pos_x + (tr_side / 2) - 1 - x, pos_y + tr_side - y, pos_x + (tr_side / 2) + 2 + x, pos_y + tr_side - y);
        y = y + 1;
        x = x + 1;
        (*_p_TFT).stroke(color_r, color_g, 0);
        (*_p_TFT).line(pos_x + (tr_side / 2) - 1 - x, pos_y + tr_side - y, pos_x + (tr_side / 2) + 2 + x, pos_y + tr_side - y);
        y = y + 1;
      }
    } else if (direction_number == 2) {
      if (step > (tr_side / 2)) {
        step = (tr_side / 2);
      };
      while (y <= step) {
        (*_p_TFT).stroke(color_r, color_g, 0);
        if (y < (5 * (tr_side / 20))) {
          color_r = color_r + (25 * (40 / tr_side));  //за 5(10) шагов до 250
        } else {
          color_g = color_g - (25 * (40 / tr_side));  //за 5(10) шагов до 0
        };
        (*_p_TFT).line(pos_x + (tr_side / 2) - 1 - x, pos_y + (tr_side / 2) - y, pos_x + (tr_side / 2) + 2 + x, pos_y + (tr_side / 2) - y);
        y = y + 1;
        x = x + 1;
      };
    };
  } else if (value < center) {
    step = ceil((center - value) / ((center - comfort_min) / (tr_side / 2)));
    if ((direction_number == 2) && (step > (tr_side / 2))) {
      step = (tr_side / 2);
    };
    while (y <= step) {
      (*_p_TFT).stroke(color_r, color_g, 0);
      if (y < (5 * (tr_side / 20))) {
        color_r = color_r + (25 * (40 / tr_side));  //за 5(10) шагов до 250
      } else {
        color_g = color_g - (25 * (40 / tr_side));  //за 5(10) шагов до 0
      };
      (*_p_TFT).line(pos_x + (tr_side / 2) - 1 - x, pos_y + (tr_side / 2) + y, pos_x + (tr_side / 2) + 2 + x, pos_y + (tr_side / 2) + y);
      y = y + 1;
      x = x + 1;
    };
  }


  //меняем цвет текста обратно на белый:
  (*_p_TFT).stroke(255, 255, 255);
  return;
};



//ПРОЦЕДУРА ОТРИСОВКИ ЗНАЧКА ТРЕВОГИ(выход значения параметра за пределы комфортной зоны):
void SCREEN::Draw_danger_sign(byte pos_x, byte pos_y, bool draw_sign) {
  byte tr_side = 20;

  if (_page >= 3) {
    tr_side = 40;
  }

  if (draw_sign == true) {
    (*_p_TFT).stroke(255, 255, 0);
    byte x = 1;
    byte y = 1;

    (*_p_TFT).line(pos_x, pos_y + tr_side, pos_x + tr_side, pos_y + tr_side);
    while (x <= ((tr_side) / 2)) {
      (*_p_TFT).line(pos_x + x, pos_y + tr_side - y, pos_x + tr_side - x, pos_y + tr_side - y);
      y = y + 1;
      (*_p_TFT).line(pos_x + x, pos_y + tr_side - y, pos_x + tr_side - x, pos_y + tr_side - y);
      y = y + 1;
      x = x + 1;
    };

    /*
  (*_p_TFT).line(x, y + tr_side, x + tr_side, y + tr_side);
  (*_p_TFT).line(x+1, y + tr_side-1, x + tr_side-1, y + tr_side-1);
  (*_p_TFT).line(x+1, y + tr_side-2, x + tr_side-1, y + tr_side-2);
  (*_p_TFT).line(x+2, y + tr_side-3, x + tr_side-2, y + tr_side-3);
  (*_p_TFT).line(x+2, y + tr_side-4, x + tr_side-2, y + tr_side-4);



  (*_p_TFT).line(x, y + tr_side, x + (tr_side / 2), y);
  (*_p_TFT).line(x + (tr_side / 2), y, x + tr_side, y + tr_side);
  (*_p_TFT).line(x + tr_side, y + tr_side, x, y + tr_side);
  */
    (*_p_TFT).stroke(255, 0, 0);
    if (_page <= 2) {
      (*_p_TFT).setTextSize(2);
      (*_p_TFT).text("!", pos_x + 4, pos_y + 6);
      (*_p_TFT).text("!", pos_x + 6, pos_y + 6);
    } else if (_page >= 3) {
      (*_p_TFT).setTextSize(4);
      (*_p_TFT).text("!", pos_x + 8, pos_y + 12);
      (*_p_TFT).text("!", pos_x + 12, pos_y + 12);
    }

  } else {
    (*_p_TFT).fill(0, 0, 0);                             //цвет заливки - чёрный
    (*_p_TFT).stroke(0, 0, 0);                           //цвет границы - чёрный
    (*_p_TFT).rect(pos_x, pos_y, tr_side, tr_side + 2);  //чёрный квадрат на месте знака danger
  }
  //меняем цвет текста обратно на белый и размер на "3":
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).setTextSize(3);
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

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Koefficient Pul`saciy:", 0, 45);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("%", 78, 64);

  (*_p_TFT).stroke(255, 0, 255);
  (*_p_TFT).setTextSize(1);
  (*_p_TFT).text("Uroven` Shuma:", 0, 90);
  (*_p_TFT).setTextSize(2);
  (*_p_TFT).stroke(255, 255, 255);
  (*_p_TFT).text("dB", 78, 109);


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

  (*_p_TFT).fill(0, 0, 0);    //цвет заливки - чёрный
  (*_p_TFT).stroke(0, 0, 0);  //цвет границы - чёрный
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




  if (ignored_sensors_array[1] == false) {  //если сенсор не в списке игнорируемых, то тогда получаем новое значение
    //создание строковой переменной с указанием количества знаков после запятой (0 - нет.):
    v_LUX = (*_p_LUX).get_sensors_value();
  }
  if (ignored_sensors_array[2] == false) {
    v_PULS = (*_p_PULS).get_sensors_value();
  }
  if (ignored_sensors_array[3] == false) {
    v_NOISE = (*_p_NOISE).get_sensors_value();
  }



  (*_p_TFT).stroke(255, 255, 255);  //"включаем" (цвет текста =белый) весь динамический текст
  (*_p_TFT).setTextSize(3);
  //значение сенсора выводим в любом случае, независимо от игнорирования параметра.
  //таким образом будет выводиться последнее полученное значение
  if (ignored_sensors_array[1] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 15, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 15, sensors_low_border[1], sensors_high_border[1], v_LUX, 2);
  str_v_LUX = String(v_LUX, 0);
  str_v_LUX.toCharArray(printout, 5);  //5 - количество символов (5-1=4, 4 символа выводится)
  (*_p_TFT).text(printout, 0, 15);     //вывести text по координатам x=0 от левого края и y=15 от верхнего
  if (ignored_sensors_array[1] == false) {
    Draw_danger_sign(139, 15, (*_p_LUX).get_sensors_is_danger());
  }


  if (ignored_sensors_array[2] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 60, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 60, sensors_low_border[2], sensors_high_border[2], v_PULS, 1);
  str_v_PULS = String(v_PULS, 1);
  str_v_PULS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 60);
  if (ignored_sensors_array[2] == false) {
    Draw_danger_sign(139, 60, (*_p_PULS).get_sensors_is_danger());
  }


  if (ignored_sensors_array[3] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 105, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 105, sensors_low_border[3], sensors_high_border[3], v_NOISE, 1);
  str_v_NOISE = String(v_NOISE, 1);
  str_v_NOISE.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 105);
  if (ignored_sensors_array[3] == false) {
    Draw_danger_sign(139, 105, (*_p_NOISE).get_sensors_is_danger());
  }


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

  (*_p_TFT).fill(0, 0, 0);    //цвет заливки - чёрный
  (*_p_TFT).stroke(0, 0, 0);  //цвет границы - чёрный
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



  if (ignored_sensors_array[4] == false) {  //если сенсор не в списке игнорируемых, то тогда получаем новое значение
    v_TEMP = (*_p_TEMP).get_sensors_value();
  }
  if (ignored_sensors_array[5] == false) {
    v_HUM = (*_p_HUM).get_sensors_value();
  }
  if (ignored_sensors_array[6] == false) {
    v_CO2 = (*_p_CO2).get_sensors_value();
  }



  (*_p_TFT).stroke(255, 255, 255);  //"включаем" (цвет текста =белый) весь динамический текст
  (*_p_TFT).setTextSize(3);
  //значение сенсора выводим в любом случае, независимо от игнорирования параметра.
  //таким образом будет выводиться последнее полученное значение
  if (ignored_sensors_array[4] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 15, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 15, sensors_low_border[4], sensors_high_border[4], v_TEMP, 2);
  //создание строковой переменной с указанием количества знаков после запятой (0 - нет.):
  str_v_TEMP = String(v_TEMP, 1);
  str_v_TEMP.toCharArray(printout, 5);  //5 - количество символов (5-1=4, 4 символа выводится)
  (*_p_TFT).text(printout, 0, 15);      //вывести text по координатам x=0 от левого края и y=15 от верхнего
  if (ignored_sensors_array[4] == false) {
    Draw_danger_sign(139, 15, (*_p_TEMP).get_sensors_is_danger());
  }


  if (ignored_sensors_array[5] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 60, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 60, sensors_low_border[5], sensors_high_border[5], v_HUM, 1);
  str_v_HUM = String(v_HUM, 1);
  str_v_HUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 60);
  if (ignored_sensors_array[5] == false) {
    Draw_danger_sign(139, 60, (*_p_HUM).get_sensors_is_danger());
  }


  if (ignored_sensors_array[6] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 105, 69, 21);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(115, 105, sensors_low_border[6], sensors_high_border[6], v_CO2, 1);
  str_v_CO2 = String(v_CO2, 0);
  str_v_CO2.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 105);
  if (ignored_sensors_array[6] == false) {
    Draw_danger_sign(139, 105, (*_p_CO2).get_sensors_is_danger());
  }


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

  (*_p_TFT).fill(0, 0, 0);    //цвет заливки - чёрный
  (*_p_TFT).stroke(0, 0, 0);  //цвет границы - чёрный
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[1] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[1] == false) {
    v_LUX = (*_p_LUX).get_sensors_value();
  }

  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[1] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[1], sensors_high_border[1], v_LUX, 2);
  str_v_LUX = String(v_LUX, 0);
  str_v_LUX.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[1] == false) {
    Draw_danger_sign(50, 80, (*_p_LUX).get_sensors_is_danger());
  }


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
  (*_p_TFT).stroke(0, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[2] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[2] == false) {
    v_PULS = (*_p_PULS).get_sensors_value();
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[2] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[2], sensors_high_border[2], v_PULS, 1);
  str_v_PULS = String(v_PULS, 1);
  str_v_PULS.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[2] == false) {
    Draw_danger_sign(50, 80, (*_p_PULS).get_sensors_is_danger());
  }


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
  (*_p_TFT).stroke(0, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[3] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[3] == false) {
    v_NOISE = (*_p_NOISE).get_sensors_value();
  }

  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[3] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[3], sensors_high_border[3], v_NOISE, 1);
  str_v_NOISE = String(v_NOISE, 1);
  str_v_NOISE.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[3] == false) {
    Draw_danger_sign(50, 80, (*_p_NOISE).get_sensors_is_danger());
  }


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
  (*_p_TFT).stroke(0, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[4] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[4] == false) {
    v_TEMP = (*_p_TEMP).get_sensors_value();
  }


  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[4] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[4], sensors_high_border[4], v_TEMP, 1);
  str_v_TEMP = String(v_TEMP, 1);
  str_v_TEMP.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[4] == false) {
    Draw_danger_sign(50, 80, (*_p_TEMP).get_sensors_is_danger());
  }


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
  (*_p_TFT).stroke(0, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[5] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[5] == false) {
    v_HUM = (*_p_HUM).get_sensors_value();
  }

  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[5] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[5], sensors_high_border[5], v_HUM, 1);
  str_v_HUM = String(v_HUM, 1);
  str_v_HUM.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[5] == false) {
    Draw_danger_sign(50, 80, (*_p_HUM).get_sensors_is_danger());
  }


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
  (*_p_TFT).stroke(0, 0, 0);
  //показ границ прямоугольника по умолчанию
  if (otladka_red_rectangle == false) {
    (*_p_TFT).noStroke();  //выключение границ прямоугольника, если не нужна отладка
  };

  if (ignored_sensors_array[6] == false) {
    //очищаем текст, нарисовав чёрный прямоугольник (область закраски: граница+внутри):
    (*_p_TFT).rect(0, 39, 92, 28);
  }


  if (ignored_sensors_array[6] == false) {
    v_CO2 = (*_p_CO2).get_sensors_value();
  }

  (*_p_TFT).setTextSize(4);
  if (ignored_sensors_array[6] == true) {
    (*_p_TFT).noFill();           //без заливки
    (*_p_TFT).stroke(255, 0, 0);  //цвет границы - красный
    (*_p_TFT).rect(0, 39, 92, 28);
    (*_p_TFT).fill(0, 0, 0);          //цвет заливки - чёрный
    (*_p_TFT).stroke(255, 255, 255);  //цвет границы - белый
  }
  Draw_value_borders(1, 80, sensors_low_border[6], sensors_high_border[6], v_CO2, 1);
  str_v_CO2 = String(v_CO2, 0);
  str_v_CO2.toCharArray(printout, 5);
  (*_p_TFT).text(printout, 0, 39);
  if (ignored_sensors_array[6] == false) {
    Draw_danger_sign(50, 80, (*_p_CO2).get_sensors_is_danger());
  }

  //Serial.print("getHeaterON()=");
  //Serial.println((*_p_CO2).getHeaterON());
  //Serial.print("otladka_heater_on=");
  //Serial.println(otladka_heater_on);

  if (otladka_heater_on) {           //если включён вывод состояния датчика CO2 (on/off)
    if (!(*_p_CO2).getHeaterON()) {  //если датчик включён
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
