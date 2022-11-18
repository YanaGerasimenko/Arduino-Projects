#include <SoftwareSerial.h>
#include <CTBot.h>

#define RECIEVEFROMMEGA  19 //Объявление RX пина для приёма с меги

String ssid  = "Fresh"    ; //Название WiFi
String pass  = "OKM960wsx"; //Пароль от WiFI
String token = "1488985666:AAEKvHlYkXuX0D5bE8uIDNVqXd0VHjFGDbg"   ; //Токен бота

CTBot myBot;//Объявление бота
TBMessage msg; //Переменная с данными сообщений

CTBotReplyKeyboard myKbd;   // reply keyboard object helper
bool isKeyboardActive;      // store if the reply keyboard is shown

String temperature = "0", humidity = "0";//Переменные датчика климата

byte systemName = 0, systemSituation = 0, systemMsg = 0;
String systemSend = " ";

String sender;

void setup() {
  Serial.begin(115200);

  //Подключение к wifi
  myBot.wifiConnect(ssid, pass);

  //Запуск бота
  myBot.setTelegramToken(token);

  //Объявление кнопок
  myKbd.addButton("Send info");
  //keyboard_one.addRow(row_one, 1);
  //bot.begin();
  // resize the keyboard to fit only the needed space
  myKbd.enableResize();
  isKeyboardActive = true;
}

void loop() {
  readInfo();
  sendInfo();
}

//Чтение с Mega
void readInfo() {
  while (Serial.available()) {
    systemSituation = Serial.read();
    switch (systemSituation) {
      default:
        break;
      case 0:
        temperature = Serial.read();
        humidity = Serial.read();
        break;
      case 1:
        systemName = Serial.read();
        switch (systemName) {
          case 1:
            systemMsg = Serial.read();
            switch (systemMsg) {
              case 1:
                systemSend = "error: Out of water";
                break;
              case 2:
                systemSend = "error: Pump not working";
                break;
            }
            break;
          case 2:
          systemMsg = Serial.read();
            switch (systemMsg) {
              case 1:
                systemSend = "error: Light can't be turned on";
                break;
              case 2:
                systemSend = "error: Light can't be turned off";
                break;
            }
          break;
        }
        break;
      case 2:
        systemName = Serial.read();
        switch (systemName) {
          case 1:
            systemMsg = Serial.read();
            switch (systemMsg) {
              case 1:
                systemSend = "Watering started";
                break;
              case 2:
                systemSend = "Watering ended";
                break;
            }
          case 2:
            systemMsg = Serial.read();
            switch (systemMsg) {
              case 1:
                systemSend = "Light on";
                break;
              case 2:
                systemSend = "Light off";
                break;
            }
        }
    }
  }
}

//Отправка в телеграм
void sendInfo() {

  String outMsg = "Temperature: " + temperature + "\n" + "Humidity: " + humidity;

  //Проверка наличия новых сообщений
  if (myBot.getNewMessage(msg)) {
    sender = msg.sender.id;
    if (msg.messageType == CTBotMessageText) {
      // received a text message
      if (msg.text.equalsIgnoreCase("show keyboard")) {
        // the user is asking to show the inline keyboard --> show it
        myBot.sendMessage(msg.sender.id, "Keyboard openned", myKbd);
      }
      else if (msg.text.equalsIgnoreCase("Send info")) {
        // the user is asking to show the reply keyboard --> show it
        myBot.sendMessage(msg.sender.id, outMsg);//отправка сообщения
        isKeyboardActive = true;
      }
      else {
        // the user write anithing else --> show a hint message
        myBot.sendMessage(msg.sender.id, "Try 'show keyboard'");
      }
    }
  }
  if (systemSend != " ") {
    myBot.sendMessage(msg.sender.id, systemSend);
                      systemSend = " ";
                    }
                      delay(500);
                    }
