#include "DHT.h"
//#include <TroykaMQ.h>
#include <MQ135.h>

#define TimeToHeaterON 60000
#define PIN_MQ135_HEATER 2

#define SEN_LUX_analog_pin A0      //пин, откуда мы будем читать недозначения освещённости датчика TEMT6000
#define time_stamp_minmax_lux 100  //период обновления данных для коэффициента пульсаций. 100 - оптимальное значение

//для функции расчёта lux на основании входного значения ацп:
float const AREF = 5.0;  // set for 5.0 or 3.3 depending on voltage of uC



// создаём перечисление sensor_type. Это будут виды наших датчиков
enum SENSOR_t {
  SEN_LUX,
  SEN_PULSE,
  SEN_NOISE,
  SEN_TEMP,
  SEN_HUM,
  SEN_CO2
};



//======================================РАЗДЕЛ_ОПИСАНИЯ_КЛАССА_SENSOR:====================================== :



//Класс Датчика
//получает данные с физического датчика
//анализирует
//выставляет флаги
//реди = готов к работе  есть ощущение что аларм лишний. мы либо доверяем данным датчика либо нет ... реди или НЕ реди
//данжер = если параметр вышел из комфортных пределов
class SENSOR {
public:
  //Конструктор
  //  тип датчика, ссылка на объект обслуживающим физический датчик, время обновления, комфортный интервал
  SENSOR(SENSOR_t _sen_t, int _time = 1000, float delta = 0, float comfort_min = 0, float comfort_max = 0.0,
         DHT* dht = 0, MQ135* mq135 = 0, SENSOR* p = 0, SENSOR* t = 0, SENSOR* h = 0) {
    _SENSOR_t = _sen_t;
    //для ссылок на существующие объекты НЕродственники
    _p_dht = dht;
    _p_mq = mq135;
    _p_puls = p;  //для передачи ссылки на экземпляр SEN_PULSE класса SENSOR в экземпляр SEN_LUX этого же класса
    _p_temp = t;
    _p_hum = h;


    //период обновления датчика + cтамп, когда в последний раз датчик дергали
    _refreshPeriod = _time;
    _time_stamp = 0;
    _time_stamp_ForHeater = 0;
    _time_stamp_min_lux = millis();  //сколько времени прошло с момента рассчёта _min_lux
    _time_stamp_max_lux = millis();  //сколько времени прошло с момента рассчёта _max_lux

    //диапазон оптимальных значений + дельта для метода CheckRead();
    _comfort_min = comfort_min;
    _comfort_max = comfort_max;
    _Delta = delta;

    //поля готовности датчика (можно ли доверять данным, выхода за дипазон + полученное значение + предыдущ значение)
    _Ready = false;
    _Danger = false;
    _Value = 0;
    _predValue = _predValue + 2 * _Delta;

    //установка полей минимального и максимального значения освещённости в начальные значения:
    _max_lux = -1;     //хранит последнее максимальное значение освещённости в люксах
    _min_lux = 20000;  //хранит последнее минимальное значение освещённости в люксах
  }

  //процедура обновления сведений
  void refresh();

  // Функция Ready - определяет, готово ли устройство к работе. За текущее значение отвечает _Value, _Delta - значение ∆
  // когда разница между двумя соседними измерениями будет меньше ∆, тогда считается, что устройство готово
  // _predValue  - предыдущее значение.
  bool getReady() {
    return _Ready;
  };
  bool getDanger() {
    return _Danger;
  };
  float getValue() {
    return _Value;
  };
  float getlux() {  //для получения значения освещённости сенсором "освещенность" из сенсора "пульсации" по ссылке.
    return _lux;
  };
  bool getHeaterON() {
    return _HeaterON;
  };

  SENSOR_t getType() {  //функция получения типа текущего сенсора из перечисления, объявленного выше. Зачем?
    return _SENSOR_t;
  };


private:
  SENSOR_t _SENSOR_t;
  DHT* _p_dht;
  MQ135* _p_mq;
  SENSOR* _p_puls;
  SENSOR* _p_temp;
  SENSOR* _p_hum;

  int _refreshPeriod;    //период обновления значения величины
  uint32_t _time_stamp;  //время подследнего обновления
  uint32_t _time_stamp_ForHeater;
  uint32_t _time_stamp_max_lux;  //сколько времени прошло с момента рассчёта _max_lux
  uint32_t _time_stamp_min_lux;  //сколько времени прошло с момента рассчёта _min_lux
  float _comfort_min;            //интервал комфортных значений
  float _comfort_max;            //интервал комфортных значений
  float _Value;

  float _lux;      //хранит текущее значение освещённости в люксах
  float _max_lux;  //хранит текущее максимальное значение освещённости в люксах
  float _min_lux;  //хранит текущее минимальное значение освещённости в люксах

  bool _Ready;   //признак того что данным моно доверять. датчик = готов
  bool _Danger;  //признак выхода значения за комфортные границы.
  float _predValue, _Delta;

  bool _HeaterON = true;

  void Danger();
  void CheckRead();

  void refresh_SEN_LUX();
  void refresh_SEN_PULSE();
  void refresh_SEN_SHUM();
  void refresh_SEN_TEMP();
  void refresh_SEN_HUM();
  void refresh_SEN_CO2();
  void OnLine();


protected:
};



//======================================РАЗДЕЛ_ОСНОВНОЙ_ПРОЦЕДУРЫ_refresh()====================================== :



void SENSOR::refresh() {
  //кейс по состояниям; в зависимости от состояния выполняем то или иное изменение светодиода ! ЕСЛИ НАСТАЛО ВРЕМЯ!
  switch (_SENSOR_t) {
    case SEN_PULSE:
      //это датчик пульсаций
      refresh_SEN_PULSE();
      break;

    case SEN_LUX:
      //это датчик освещенности
      refresh_SEN_LUX();
      break;

    case SEN_NOISE:
      //это датчик шума
      //здесь должна быть процедура refresh() для датчика шума.
      break;

    case SEN_TEMP:
      //это датчик температуры
      refresh_SEN_TEMP();
      break;

    case SEN_HUM:
      refresh_SEN_HUM();
      break;

    case SEN_CO2:
      //это датчик СО2
      refresh_SEN_CO2();
      break;

    default:
      //выход по умолчанию, на всякий случай
      break;
  }
}



//======================================РАЗДЕЛ_ОСНОВНЫХ_ПРОЦЕДУР_И_ФУНКЦИЙ_(не refresh())====================================== :



//ПРОЦЕДУРА ПРОВЕРКИ ВЫХОДА ЗНАЧЕНИЯ ПАРАМЕТРА ЗА ГРАНИЦЫ КОМФОРТНОЙ ЗОНЫ:
void SENSOR::Danger() {
  if (_Value > _comfort_max) {
    _Danger = true;
    //Serial.print(_Value);
    //Serial.println(" HUM. Get out of the comfort zone. To HIGH");
  } else {
    if (_Value < _comfort_min) {
      _Danger = true;
      //Serial.print(_Value);
      //Serial.println(" HUM. Get out of the comfort zone. To LOW");
    } else {
      _Danger = false;
    }
  }
};



//ПРОЦЕДУРА ПРОВЕРКИ ГОТОВНОСТИ ДАТЧИКА ДЛЯ ВЫДАЧИ КОРРЕКТНЫХ ПАРАМЕТРОВ:
void SENSOR::CheckRead() {
  //Serial.print(_predValue); Serial.print("; "); Serial.print(_Value); Serial.print("; "); Serial.println(analogRead(A5));
  //Serial.println(analogRead(A5));

  if (abs(_predValue - _Value) < _Delta) {
    _Ready = true;
  } else {
    _Ready = false;
  }
};



//========================================РАЗДЕЛ_ДОПОЛНИТЕЛЬНЫХ_ПРОЦЕДУР_И_ФУНКЦИЙ======================================== :



//ФУНКЦИЯ РАСЧЁТА ЗНАЧЕНИЯ, ВЫРАЖЕННОГО В LUX НА ОСНОВАНИИ ВХОДНОГО ЗНАЧЕНИЯ С АЦП:
float adc_to_lux(float adc_lux) {

  //float volts = adc_lux * AREF / 1024.0;    // Convert reading to voltage
  //float amps = volts / 10000.0;             // Convert to amps across 10K resistor
  //float microamps = amps * 1000000.0;       // Convert amps to microamps
  //float lux = microamps * 2.0;              // 2 microamps = 1 lux
  //
  //более сокращённо:
  //float lux = ((((adc_lux * AREF) / 1024.0) / 10000.0) * 1000000.0) * 2.0;
  //lux = 3.56 * lux + 1.7;  //экспериментально
  //lux = 5.72 + 0.943 * lux + 0.000197 * lux * lux;
  //
  //ещё более сокращённо:
  float lux = ((89.0 * adc_lux * AREF) / 128) + 1.7;
  //lux = 5.72 + 0.943 * lux + 0.000197 * lux * lux;
  //
  //
  //
  //убираем промежуточную переменную lux и сразу пишем итоговое выражение в return:

  return 5.72 + 0.943 * lux + 0.000197 * lux * lux;
}



//ПРОЦЕДУРА ! (а что за процедура и нужна ли она - потом уточнить)
void SENSOR::OnLine() {

  //а первое включение??
  if ((millis() - _time_stamp_ForHeater) < TimeToHeaterON) {
    //(*_p_mq).heaterPwrOff();
    if (_Ready) {
      _HeaterON = false;
    }
    //return;  //выходим из процедуры
  } else {
    _HeaterON = true;
    //(*_p_mq).heaterPwrHigh();

    _time_stamp_ForHeater = millis();
  }

  //Как это связать с _Ready
  if (_HeaterON) {
    //(*_p_mq).heaterPwrHigh();
    digitalWrite(PIN_MQ135_HEATER, HIGH);
  }
  if (!_HeaterON) {
    //(*_p_mq).heaterPwrOff();
    digitalWrite(PIN_MQ135_HEATER, LOW);
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_КОЭФФИЦИЕНТА_ПУЛЬСАЦИЙ======================================== :



//int count=0;

void SENSOR::refresh_SEN_PULSE() {

  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {
    //float l = analogRead(SEN_LUX_analog_pin);
    //Serial.println(l);
    //_lux = adc_to_lux(l);
    //
    //упрощение:
    _lux = adc_to_lux(analogRead(SEN_LUX_analog_pin));


    bool uslovie_for_max = ((_time_stamp - _time_stamp_max_lux) > time_stamp_minmax_lux);  //проверка условия для _max_lux
    bool uslovie_for_min = ((_time_stamp - _time_stamp_min_lux) > time_stamp_minmax_lux);  //проверка условия для _min_lux

    //вариант 1:
    /*
    if (uslovie_for_max) {
      _max_lux = _lux;
      _time_stamp_max_lux = millis();
      _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;
    } else if ((!uslovie_for_max) && (_lux > _max_lux)) {
      _max_lux = _lux;
      _time_stamp_max_lux = millis();
      _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;
    } else if (uslovie_for_min) {
      _min_lux = _lux;
      _time_stamp_min_lux = millis();
      _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;
    } else if ((!uslovie_for_min) && (_lux < _min_lux)) {
      _min_lux = _lux;
      _time_stamp_min_lux = millis();
      _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;
    };
    //на случай если выполнены оба условия (uslovie_for_max и uslovie_for_min) надо проверять uslovie_for_min снова отдельно:
    if (uslovie_for_min) {
      _min_lux = _lux;
      _time_stamp_min_lux = millis();
      _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;
    };
    //Serial.print("_max_lux = ");
    //Serial.println(_max_lux);
    //Serial.print("_min_lux = ");
    //Serial.println(_min_lux);
    */


    //вариант 2 (на всякий случай):
    if (uslovie_for_max) {
      _max_lux = _lux;
      _time_stamp_max_lux = millis();
    } else if ((!uslovie_for_max) && (_lux > _max_lux)) {
      _max_lux = _lux;
      _time_stamp_max_lux = millis();
    };

    if (uslovie_for_min) {
      _min_lux = _lux;
      _time_stamp_min_lux = millis();
    } else if ((!uslovie_for_min) && (_lux < _min_lux)) {
      _min_lux = _lux;
      _time_stamp_min_lux = millis();
    };
    //Serial.print("_max_lux = ");
    //Serial.println(_max_lux);
    //Serial.print("_min_lux = ");
    //Serial.println(_min_lux);



    _Value = ((_max_lux - _min_lux) / (_max_lux + _min_lux)) * 100;


    if (_Value < 0) {
      _Value = 0;
    };


    Danger();

    //count=count+1;

    _time_stamp = millis();
    return;
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_ОСВЕЩЁННОСТИ======================================== :



void SENSOR::refresh_SEN_LUX() {
  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {

    _Value = (*_p_puls).getlux();

    Danger();

    _time_stamp = millis();
    return;
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_ШУМА======================================== :



void SENSOR::refresh_SEN_SHUM() {
  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {

    //пока нет датчика шума, зададим ему значение напрямую:
    _Value = 12.3;

    Danger();

    _time_stamp = millis();
    return;
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_ТЕМПЕРАТУРЫ======================================== :



void SENSOR::refresh_SEN_TEMP() {
  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {
    _Value = (*_p_dht).readTemperature();
    if (isnan(_Value)) {
      _Ready = false;
      //Serial.println("TEMP. Can't read the temp DATA");
    } else {
      _Ready = true;
      Danger();  // проверим прочитанное значение на предмет попадания в комфортную зону
    }
    _time_stamp = millis();
    return;
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_ВЛАЖНОСТИ======================================== :



void SENSOR::refresh_SEN_HUM() {
  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {
    _Value = (*_p_dht).readHumidity();
    if (isnan(_Value)) {
      _Ready = false;
      //Serial.println("HUM. Can't read the humid DATA");
    } else {
      _Ready = true;
      Danger();
    }
    _time_stamp = millis();
    return;
  }
};



//========================================РАЗДЕЛ_СЕНСОРА_CO2======================================== :



void SENSOR::refresh_SEN_CO2() {

  if ((millis() - _time_stamp) < _refreshPeriod) {
    return;  //выходим из процедуры
  } else {

    OnLine();

    //if (!isnan((*_p_dht).getValue())&&!isnan((*_p_dht).getValue())){

    //}
    //_Value = (*_p_mq).getCorrectedPPM(temperature, humidity);
    //временно так:
    _Value = (*_p_mq).getCorrectedPPM((*_p_temp).getValue(), (*_p_hum).getValue());

    CheckRead();
    _predValue = _Value;

    Danger();
    _time_stamp = millis();
  }
};



//=========================================================КОНЕЦ_РАЗДЕЛОВ=========================================================
