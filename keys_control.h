#define FRONT_TIMEOUT 50  //сколько времени ждем фронт сигнала
#define USER_TIMEOUT 350  //сколько времени ждем фронт сигнала

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

};

signed char ENCODER::check_and_get() {//signed char _result;  //-1 = влево; 0 = нет состояния; +1 = вправо
  //прочитаем кнопки
  //если были смены статусов небыло то конец, если была,
  //то начинаем смотреть статус автомата
  boolean L = digitalRead(_pin_l);
  boolean R = digitalRead(_pin_r);
  boolean L_change = (!(L == L_pred));
  boolean R_change = (!(R == R_pred));
  signed char rez = 0;

  if (L_change | R_change) {
    //были изменения
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
      if ((R==1) & (L==1)) {
        if ((millis() - _time_stamp) > FRONT_TIMEOUT) {
          en_mode = EN_WAIT;
          _time_stamp = millis();
        };
      };
    } else {
    };
    L_pred = L;
    R_pred = R;
  };
  return rez;
}
