//Подключение библиотек
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RTC.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <avr/eeprom.h>

#define DHTPIN 30 // Объявление пина датчика климата
#define DHTTYPE DHT22 // Объявление типа датчика климата
#define LIQUIDPIN1 28 // Объявление пина датчика жидкости
#define LIQUIDPIN2 29 // Объявление пина датчика жидкости
#define SOUNDPIN A0 // Объявление пина сирены
#define LIGHTRELAYPIN 8 // Объявление пина реле на свете (ближняя к чёрной затычке, In1)
#define PUMPRELAYPIN 9 // Объявление пина реле на насосе
#define LIGHTSENSORPIN A1 // Объявление пина датчика света
#define SENDTOESP 18 //  Объявление TX пина для отправки на esp

DHT dht(DHTPIN, DHTTYPE); //Инициализация датчика темпиратуры и влажности
LiquidCrystal_I2C lcd(0x27, 16, 2); //Инициализвция дисплея
iarduino_RTC time(RTC_DS1307); //Инициализвция часов

float temperature = 0, humidity = 0;
const short tByte = 0, hByte = 1;//Переменные для датчика климата

unsigned long long last_display_switch = 0;
short cur_display_state = 1; //Переменные для дисплея

int liquid_level = 0; //Переменные для датчика жидкости

short is_light_on = 0, should_light_be_on = 0, last_light_animation = 0, light_was_written = 0;
const short BRIGHNTESS = 200, LIGHTANIMATIONBYTE = 255, lightByte = 2;
int light_diagnostics = 0, light = 0;
const int TIMEON = 6 * 60, TIMEOFF = 22 * 60; //Переменные для систесы освещения

struct watering_struct {
  int time_on = 0;
  short flag = 0;
  short Byte = 0;
}; //Структура для полива

short is_pump_on = 0, should_pump_be_on = 0, last_watering_animation = 0, was_pipe_filled = 0, water_was_written = 0;
const short wateringByte = 3;
int work_start_time = 0, work_start_day = 0, time_from_last_check = 0, filling_start_time = 0;
watering_struct watering_start[3];
const int TIMETOFILL = 5, TIMETOSTOPWATERING = 6;// Переменные полива

int current_mins = 0, current_day = 0;//Переменные времени

byte i = 0;//Переменная для энергонезависимой памяти

//собственные символы
byte lightBulb[8] = {
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b01110,
  0b01110,
  0b00100
};

byte lightParticles1[8] = {
  0b00100,
  0b01010,
  0b00000,
  0b00100,
  0b01010,
  0b10001,
  0b00100,
  0b01010
};

byte lightParticles2[8] = {
  0b00100,
  0b01010,
  0b10001,
  0b00100,
  0b01010,
  0b00000,
  0b00100,
  0b01010
};

byte darkBulb[8] = {
  0b01110,
  0b10001,
  0b10001,
  0b10001,
  0b01010,
  0b01110,
  0b01110,
  0b00100
};

byte shower[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00000
};

byte drop1[8] = {
  0b01000,
  0b01000,
  0b01000,
  0b00010,
  0b00010,
  0b00010,
  0b00000,
  0b00000
};

byte drop2[8] = {
  0b00001,
  0b00001,
  0b01000,
  0b01000,
  0b01000,
  0b00010,
  0b00010,
  0b00010
};

byte drop3[8] = {
  0b00000,
  0b00001,
  0b00001,
  0b00001,
  0b01000,
  0b01000,
  0b01000,
  0b00010
};

int notes[] = {
  392, 392, 392, 311, 466, 392, 311, 466, 392
};
int times[] = {
  350, 350, 350, 250, 100, 350, 250, 100, 700
};

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  dht.begin(); //Запуск датчика климата
  lcd.begin(); //Запуск дисплея
  lcdCustomChrars();//Создание символов для анимаций
  timeBegin(); //Запуск часов
  pinMode(LIGHTSENSORPIN, INPUT);//Установка пина датчика света на вход
  pinMode(LIQUIDPIN1, INPUT); //Установка пина датчика жидкости 1 на вход
  pinMode(LIQUIDPIN2, INPUT); //Установка пина датчика жидкости 2 на вход
  pinMode(SOUNDPIN, OUTPUT); //Установка пина сирены на выход
  pinMode(PUMPRELAYPIN, OUTPUT); //Установка пина реле помпы на выход
  digitalWrite(PUMPRELAYPIN, HIGH); //Установка изначальной позиции реле помпы на выключенный
  pinMode(LIGHTRELAYPIN, OUTPUT); //Установка пина реле света на выход
  digitalWrite(LIGHTRELAYPIN, HIGH); //Установка изначальной позиции реле света на выключенный
  startingScreen();//Запуск экрана загруски
  startingMelody();//Запуск мелодии загрузки
  setWatteringStartStruct();//Запись времени проивов
  readFromEEPROM();//Чтение значений с энергонезависиммой памяти
  lcd.clear();//Очистка дисплея
}

//Запись времени проивов
void setWatteringStartStruct() {
  watering_start[0].time_on = 19 * 60 + 45;
  watering_start[1].time_on = 12 * 60;
  watering_start[2].time_on = 18 * 60;
  watering_start[0].Byte = 253;
  watering_start[1].Byte = 252;
  watering_start[2].Byte = 251;
}

//Чтение с энергонезависимой памяти
void readFromEEPROM() {
  EEPROM.get(watering_start[0].Byte, watering_start[0].flag);
  EEPROM.get(watering_start[1].Byte, watering_start[1].flag);
  EEPROM.get(watering_start[2].Byte, watering_start[2].flag);
}

//объявления собственных символов
void lcdCustomChrars() {
  lcd.createChar(0, darkBulb);//Погасшая лампочка
  lcd.createChar(1, lightBulb);//Зажжённая лампочка
  lcd.createChar(2, shower);//Душ для полива
  lcd.createChar(3, drop1);//Капля воды 1
  lcd.createChar(4, drop2);//Капля воды 2
  lcd.createChar(5, drop3);//Капля воды 3
  lcd.createChar(6, lightParticles1);//Частицы света 1
  lcd.createChar(7, lightParticles2);//Частицйы света 2
}

//Включение экрана загрузки
void startingScreen() {
  lcd.setCursor(2, 0);
  lcd.print("Starting....");
}

//включение мелодии при запуске
void startingMelody() {
  for (int i = 0; i < 9; i++) {
    tone(SOUNDPIN, notes[i], times[i] * 2);
    delay(times[i] * 2);
    noTone(SOUNDPIN);
  }
}

//начало работы часов
void timeBegin() {
  time.begin();
}







///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////







void loop() {
  readDHT(); //Чтение с датчика климата
  readTime(); // Чтение с часов
  lightSensorRead(); // Чтение с фоторезистора
  changeLightState(); // Изменение положение реле на свете
  liquidSensorRead(); // Чтение с датчиков влажности
  changePumpState(); // Изменение положения реле
  //Ожидание без остановки программы
  if (millis() - last_display_switch >= 5000) {
    lcd.clear();
    last_display_switch = millis();
    changeDisplayState();
  }
  writeDisplay(); // Запись на дисплей
  sendToEsp();// Отправка сообщений на esp для телеграм бота
  delay(100);
}

//Запись в энергонезависимую память
void writeEEPROM(short adress, String msg) {
  eeprom_update_byte(i, temperature);
  eeprom_update_byte(i, humidity);
  if (i <= 255) {
    i++;
  }
  else {
    i = 0;
  }
}

//Считывание с датчика темпиратуры и влажности
void readDHT() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  writeEEPROM(tByte, (String)temperature);
  writeEEPROM(hByte, (String)humidity);
}

//Считывание времени
void readTime() {
  time.gettime();
  current_mins = time.Hours * 60 + time.minutes;
  current_day = time.day;
}

//Смена состояний дисплея
void changeDisplayState() {
  switch (cur_display_state) {
    default:
    case 1:
      cur_display_state = 2;
      break;
    case 2:
      cur_display_state = 3;
      break;
    case 3:
      cur_display_state = 1;
      break;
  }
}

//Запись на дисплей
void writeDisplay() {
  switch (cur_display_state) {
    default:
    case 1: //Климат

      //Вывод температуры
      lcd.setCursor(0, 0);
      lcd.print("Temp = ");
      lcd.print(temperature);
      lcd.print("*C");
      //Вывод влажности
      lcd.setCursor(0, 1);
      lcd.print("Hum = ");
      lcd.print(humidity);
      lcd.print("%");
      break;
    case 2: //Время
      //Вывод даты
      lcd.setCursor(0, 0);
      lcd.print("Date: " + (String)(time.day / 10) + (String)(time.day % 10) + "."
                + (String)(time.month / 10) + (String)(time.month % 10) + "."
                + (String)(time.year / 10) + (String)(time.year % 10));
      //Вывод времени
      lcd.setCursor(0, 1);
      lcd.print("Time: " + (String)(time.Hours / 10) + (String)(time.Hours % 10) + ":"
                + (String)(time.minutes / 10) + (String)(time.minutes  % 10) + ":"
                + (String)(time.seconds / 10) + (String)(time.seconds % 10));
      break;
    case 3: //Планы
      lcd.setCursor(0, 0);
      if (is_pump_on == 1) {
        lcd.print("Watering now");
      }
      else {
        if (current_mins <= watering_start[0].time_on || current_mins > watering_start[2].time_on) {
          lcd.print("Watering " + (String)(watering_start[1].time_on / 60 / 10) +  (String)(watering_start[1].time_on / 60 % 10) +
                    ":" + (String)(watering_start[1].time_on % 60 / 10) +  (String)(watering_start[1].time_on % 60 % 10));
        }
        else if (current_mins <= watering_start[1].time_on) {
          lcd.print("Watering 6:00" + String(watering_start[2].time_on / 60 / 10) +  (String)(watering_start[2].time_on / 60 % 10) +
                    ":" + (String)(watering_start[2].time_on % 60 / 10) +  (String)(watering_start[2].time_on % 60 % 10));
        }
        else if (current_mins <= watering_start[2].time_on) {
          lcd.print("Watering " + (String)(watering_start[0].time_on / 60 / 10) + (String)(watering_start[0].time_on / 60 % 10) +
                    ":" + (String)(watering_start[0].time_on % 60 / 10) +  (String)(watering_start[0].time_on % 60 % 10));
        }
      }
      lcd.setCursor(0, 1);
      if (should_light_be_on == 1) {
        lcd.print("light off 22:00");
      }
      else {
        lcd.print("light on 06:00");
      }
      break;
  }
}

//Чтение датчика света
void lightSensorRead() {
  light = analogRead(LIGHTSENSORPIN);
  is_light_on = light < BRIGHNTESS;
  //Serial2.println("Mega: light = " + (String)light);
}

//Изменение положения реле на свете
void changeLightState() {
  //Проверка по времени
  if (current_mins > TIMEOFF || current_mins <= TIMEON) {
    should_light_be_on = 0;
    digitalWrite(LIGHTRELAYPIN, HIGH);
    Serial3.write(2); // Инфа о системе
    Serial3.write(2); // 1-я Система
    Serial3.write(2); // Свет выкл
    if (last_light_animation == 1) {
      light_animation(0);
      last_light_animation = 0;
    }
  }
  else if (current_mins > TIMEON && current_mins <= TIMEOFF) {
    should_light_be_on = 1;
    digitalWrite(LIGHTRELAYPIN, LOW);
    Serial3.write(2); // Инфа о системе
    Serial3.write(2); // 1-я Система
    Serial3.write(1); // Свет Вкл
    if (last_light_animation == 0) {
      light_animation(1);
      last_light_animation = 1;
    }
  }

  //Проверка ошибки по освещённости
  if (abs(millis() - light_diagnostics) >= 10000) {
    light_diagnostics = millis();
    if (should_light_be_on == 1) {
      if (is_light_on == 0) {
        digitalWrite(LIGHTRELAYPIN, LOW);
        Serial2.println("Mega: Light error: should be on");
        lightErrorSound();
        Serial3.write(1); // Ошибка
        Serial3.write(2); // 2-я Система
        Serial3.write(1); // не включается
        if (light_was_written == 0) {
          writeEEPROM(lightByte, "Light error");
          light_was_written = 1;
        }
      }
      else if (light_was_written == 0) {
        writeEEPROM(lightByte, "Light was turned on");
      }
    }
    else {
      if (is_light_on == 1) {
        digitalWrite(LIGHTRELAYPIN, HIGH);
        Serial2.println("Mega: Light error: should be off");
        lightErrorSound();
        Serial3.write(1); // Ошибка
        Serial3.write(2); // 2-я Система
        Serial3.write(2); // не выключается
        if (light_was_written == 0) {
          writeEEPROM(lightByte, "Light error");
          light_was_written = 1;
        }
      }
      else if (light_was_written == 0) {
        writeEEPROM(lightByte, "Light was turned off");
      }
    }
  }
}

//Анимация освещения
void light_animation(short type) { // 1 - on, 0 - off
  switch (type) {
    default:
    case 0:
      lcd.clear();
      lcd.setCursor(0, 1);
      for (int i = 0; i < 8; i++) {
        lcd.write(byte(0));
        lcd.print(" ");
      }
      delay(5000);
      break;
    case 1:
      lcd.clear();
      lcd.setCursor(0, 1);
      for (int i = 0; i < 8; i++) {
        lcd.write(byte(6));
        lcd.print(" ");
      }
      lcd.setCursor(0, 1);
      for (int i = 0; i < 8; i++) {
        lcd.write(byte(1));
        lcd.print(" ");
      }
      for (int i = 0; i < 5; i++) {
        delay(500);
        lcd.setCursor(0, 0);
        for (int i = 0; i < 8; i++) {
          lcd.write(byte(7));
          lcd.print(" ");
        }
        delay(500);
        lcd.setCursor(0, 0);
        for (int i = 0; i < 8; i++) {
          lcd.write(byte(6));
          lcd.print(" ");
        }
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Completion code:");
      break;
  }
}

//Звук ошибки света
void lightErrorSound() {
  for (int i = 0; i < 9; i++) {
    tone(SOUNDPIN, 783, 50);
    delay(50);
    tone(SOUNDPIN, 830, 50);
    delay(50);
    tone(SOUNDPIN, 880, 50);
    delay(50);
  }
  tone(SOUNDPIN, 880, 200);
}

//Чтение датчика влажности
void liquidSensorRead() {
  liquid_level = !digitalRead(LIQUIDPIN1);// Чтение наполненности труб
  //Serial2.println("liquid level = " + (String)liquid_level);
}

//Изменение положения реле на насосе
void changePumpState() {
  //проверка времени включения
  for (int i = 0; i < 3; i++) {
    if (current_mins >= watering_start[i].time_on && watering_start[i].flag == 0 && is_pump_on == 0) {
      if (!digitalRead(LIQUIDPIN2) == 1) {
        watering_start[i].flag = 1;
        should_pump_be_on = 1;
        Serial3.write(2); // Инфа о системе
        Serial3.write(1); // 1-я Система
        Serial3.write(1); // Начало полива
        digitalWrite(PUMPRELAYPIN, LOW);
        filling_start_time = millis();
        if (last_watering_animation == 0) {
          watering_animation(1);
          last_watering_animation = 1;
        }
      }
      else {
        pumpErrorSound();
        writeEEPROM(wateringByte, "Out of water");
        Serial3.write(1); // Ошибка
        Serial3.write(1); // 1-я Система
        Serial3.write(1); // 1-я Ошибка
      }
    }
  }


  if (is_pump_on == 1 && should_pump_be_on == 1 && liquid_level == 1 && was_pipe_filled == 0) {
    digitalWrite(PUMPRELAYPIN, HIGH);
    was_pipe_filled = 1;
    work_start_day = current_day;
    work_start_time = current_mins;
    Serial.println("Filled in " + (String)filling_start_time);
  }
  //проверка времени выключения
  short time_from_last_start = abs(current_mins - work_start_time);
  if (time_from_last_start >= 6 && is_pump_on == 1) {
    should_pump_be_on = 0;
    was_pipe_filled = 0;
    is_pump_on = 0;
    Serial3.write(2); // Инфа о системе
    Serial3.write(1); // 1-я Система
    Serial3.write(2); // Конец полива
    digitalWrite(PUMPRELAYPIN, HIGH);
    if (last_watering_animation == 1) {
      watering_animation(0);
      last_watering_animation = 0;
    }
    writeEEPROM(wateringByte, "Watering ended");
  }

  //проверка заполненности
  if (should_pump_be_on == 1) {
    if (abs(millis() - time_from_last_check) >= 10000) {
      time_from_last_check = millis();
      if (liquid_level == 0) {
        digitalWrite(PUMPRELAYPIN, LOW);
      }
      else {
        digitalWrite(PUMPRELAYPIN, HIGH);
      }
    }
  }

  //проверка ошибки
  if (should_pump_be_on == 1 && time_from_last_start >= 1 && liquid_level == 0) {
    is_pump_on = 0;
    Serial.println("Mega: Pump error");
    pumpErrorSound();
    Serial3.write(1); // Ошибка
    Serial3.write(1); // 1-я Система
    Serial3.write(2); // 2-я Ошибка
  }

  //проверка обнуления
  if (time_from_last_start >= TIMETOSTOPWATERING && current_day != work_start_day) {
    for (int i = 0; i < 3; i++) {
      watering_start[i].flag = 0;
    }
  }
}

//Анимация полива
void watering_animation(short type) { // 1 - on, 0 - off
  switch (type) {
    default:
    case 0:
      lcd.setCursor(0, 0);
      lcd.write(byte(2) * 16);
      delay(5000);
      break;
    case 1:
      lcd.setCursor(0, 0);
      for (int j = 0; j < 16; j++) {
        lcd.write(byte(2));
      }
      for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, 1);
        for (int j = 0; j < 16; j++) {
          lcd.write(byte(3));
        }
        delay(1000);
        lcd.setCursor(0, 1);
        for (int j = 0; j < 16; j++) {
          lcd.write(byte(4));
        }
        delay(1000);
        lcd.setCursor(0, 1);
        for (int j = 0; j < 16; j++) {
          lcd.write(byte(5));
        }
        delay(1000);
      }
      lcd.clear();//Очистка дисплея
      break;
  }
}

void pumpErrorSound() {
  for (int i = 0; i < 9; i++) {
    tone(SOUNDPIN, 659, 50);
    delay(50);
    tone(SOUNDPIN, 659, 50);
    delay(50);
    tone(SOUNDPIN, 587, 50);
    delay(50);
    tone(SOUNDPIN, 587, 50);
    delay(50);
    tone(SOUNDPIN, 523, 300);
    delay(50);
  }
}

void sendToEsp() {
  Serial3.write(0); //Отправка данных о климате
  Serial3.write((byte)temperature);
  Serial3.write((byte)humidity);
}
