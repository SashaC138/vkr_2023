const char* get_rus_word_number(int word_number) {
  switch (word_number) {
    case 1:
      return "������������:";  //"Osveschennost`:"
    case 2:
      return "����������� ���������:";  //"Koefficient pul`saciy:"
    case 3:
      return "������� ����:";  //"Uroven` shuma:"
    case 4:
      return "�����������:";  //"Temperatura:"
    case 5:
      return "���������:";  //"Vlajnost`:"
    case 6:
      return "������� CO2:";  //"Uroven` CO2:"

    case 7:
      return "�������-";  //"Osveschen-"
    case 8:
      return "�����:";  //"nost`:"

    case 9:
      return "�����������";  //"Koefficient"
    case 10:
      return "���������:";  //"pul`saciy:"

    case 11:
      return "�������";  //"Uroven`"
    case 12:
      return "����:";  //"shuma:"

    case 13:
      return "�����";  //"Bloki"
    case 14:
      return "�����";  //"rovan"

    default:
      break;
  }
}