#define FRONT_TIMEOUT 30  //сколько времени ждем фронт сигнала

// состояния обработчика енкодера
enum en_modes_t {
  EN_WAIT_LEFT,   //поймали левый фронт. ждем
  EN_WAIT_RIGHT,  //поймали правый фронт. ждем
  EN_WAIT,        //ждем фронт
  EN_HOLD         //поймали нажатие. надо дождаться перехода в норму
};

class ENCODER {
public:
  //конструктор класса:
  ENCODER(byte pin_l, byte pin_r) {
    _pin_l = pin_l;
    _pin_r = pin_r;
    pinMode(_pin_l, INPUT_PULLUP);
    pinMode(_pin_r, INPUT_PULLUP);
    en_mode = EN_WAIT;
    //_сount = 0;
    //_result = 0;
    L_pred = false;
    R_pred = false;
    _time_stamp = millis();
  }

  //проверить статус и вернуть значение
  //забранное значение сбросить, т.е. забираем один раз
  signed char check_and_get();

private:
  byte _pin_l;
  byte _pin_r;
  en_modes_t en_mode;    //текущее состояние конечного автомата
  uint32_t _time_stamp;  //время последней смены статуса конечного автомата
  boolean L_pred;
  boolean R_pred;
  //
  //signed char: представляет один символ. Занимает в памяти 1 байт (8 бит).
  //Может хранить любой значение из диапазона от -128 до 127
  //signed char _result;  //-1 = влево; 0 = нет состояния; +1 = вправо
};

signed char ENCODER::check_and_get() {

  //прочитаем кнопки
  //если были смены статусов небыло то конец, если была,
  //то начинаем смотреть статус автомата
  boolean L = digitalRead(_pin_l);
  boolean R = digitalRead(_pin_r);
  boolean L_change = (!(L == L_pred));
  boolean R_change = (!(R == R_pred));
  signed char rez = 0;
  //Serial.print(L_pred); Serial.print(','); Serial.println(L);
  //Serial.print(L); Serial.print(','); Serial.println(R);

  if ((millis() - _time_stamp) > (10 * FRONT_TIMEOUT)) {
    en_mode = EN_WAIT;
    _time_stamp = millis();
  };


  //Serial.print(en_mode); Serial.print(','); Serial.print(L_change); Serial.print(','); Serial.println(R_change);

  if (L_change | R_change) {
    //были изменения
    //
    if (en_mode == EN_WAIT) {
      //проверим какой фронт
      if (L_change & !R_change) {
        en_mode = EN_WAIT_LEFT;
        _time_stamp = millis();
      };
      if (R_change & !L_change) {
        en_mode = EN_WAIT_RIGHT;
        _time_stamp = millis();
      };
    } else if (en_mode == EN_WAIT_LEFT) {
      //;
      if (L_change & !R_change) {
        if ((millis() - _time_stamp) > FRONT_TIMEOUT) {
          rez = -1;
          en_mode = EN_HOLD;
          _time_stamp = millis();
        };
      };
    } else if (en_mode == EN_WAIT_RIGHT) {
      //;
      if (R_change & !L_change) {
        if ((millis() - _time_stamp) > FRONT_TIMEOUT) {
          rez = +1;
          en_mode = EN_HOLD;
          _time_stamp = millis();
        };
      };
    } else if (en_mode == EN_HOLD) {
      //;
      if (R & L) {
        if ((millis() - _time_stamp) > FRONT_TIMEOUT) {
          en_mode = EN_WAIT;
          _time_stamp = millis();
        };
      };
    } else {
      //;
    };
    L_pred = L;
    R_pred = R;
  };
  return rez;
}
