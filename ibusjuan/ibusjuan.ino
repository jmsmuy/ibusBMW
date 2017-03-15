#include <SoftwareSerial.h>

#define MY_ID 0x18

#define SLEEP_LED 13
#define SEND_LED 13
#define READ_LED 13
#define RADIO_LED 13

#define BT_BUTTON1 6
#define BT_BUTTON2 7
#define BT_BUTTON3 8

#define LEN_COMMAND 20
#define COMMAND_FROM_INDEX 0
#define COMMAND_LEN_INDEX 1
#define COMMAND_FOR_INDEX 2
#define COMMAND_START_INDEX 3
#define COMMAND_CDC_DISC_INDEX 9
#define COMMAND_CDC_TRACK_INDEX 10

#define DEBUG 0
#define VERBOSE 0
#define ECHO 0
#define OTHER_MESSAGES 0

#define AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_BROADCAST 2
#define AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_TO_RADIO 12
#define AMOUNT_REGISTERED_MESSAGES_FROM_RADIO_TO_CDC 9

#define RADIO_COMMAND_POLL 0
#define RADIO_COMMAND_REQUEST_CURRENT_STATUS 1
#define RADIO_COMMAND_STOP_PLAYING 2
#define RADIO_COMMAND_START_PLAYING 3
#define RADIO_COMMAND_FAST_SCAN 4
#define RADIO_COMMAND_CHANGE_CD 5
#define RADIO_COMMAND_SCAN 6
#define RADIO_COMMAND_RANDOM 7
#define RADIO_COMMAND_CHANGE_TRACK 8

#define CDC_COMMAND_CD_TRACK_STATUS_NP 0
#define CDC_COMMAND_CD_TRACK_STATUS_PLAY 1
#define CDC_COMMAND_CD_TRACK_STATUS_PAUSE 2
#define CDC_COMMAND_TRACK_STARTED_PLAYING 3
#define CDC_COMMAND_CD_STATUS_SCAN_FORWARD 4
#define CDC_COMMAND_CD_STATUS_SCAN_BACKWARD 5
#define CDC_COMMAND_MODE_5 6
#define CDC_COMMAND_MODE_6 7
#define CDC_COMMAND_TRACK_END_PLAYING 8
#define CDC_COMMAND_CD_SEEKING 9
#define CDC_COMMAND_CD_CHECK 10
#define CDC_COMMAND_NO_MAGAZINE 11

#define CDC_BROADCAST_COMMAND_ANNOUNCE 0
#define CDC_BROADCAST_COMMAND_POLL_RESPONSE 1

#define CDC 0x18
#define RADIO 0x68
#define BROADCAST 0xFF
#define STEERING_WHEEL 0x50

#define MAGAZINE_CDS B00111111

#define CDC_STATE_STOP 0
#define CDC_STATE_PLAY 1
#define CDC_STATE_PAUSE 2
#define CDC_STATE_END 3




int incomingByte = 0;   // for incoming serial data
long counter = 0;
int command[LEN_COMMAND];
int topeCommand = 0;
int radioHasResponded = 0;

int cdcState = CDC_STATE_PLAY;
int cdcCurrentTrack = 1;
int cdcCurrentDisc = 1;

int lastState = LOW;

int broadcastMessages[AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_BROADCAST][LEN_COMMAND] = {{0x02, 0x01}, // Announce
                                                                             {0x02, 0x00}};// Poll Response
int broadcastMessagesLength[AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_BROADCAST] = {2, 2};

int messagesToBeSentToRadio[AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_TO_RADIO][LEN_COMMAND] = {{0x39, 0x00, 0x02, 0x00, MAGAZINE_CDS, 0x00, -1, -1},  // CD and Track status not playing response (dd tt)
                                                                                  {0x39, 0x00, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // CD and Track status playing response (dd tt)
                                                                                  {0x39, 0x01, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // CD and Track status paused response (dd tt)
                                                                                  {0x39, 0x02, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // Track started Playing (dd tt)
                                                                                  {0x39, 0x03, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // CD Status Scan Forward (dd tt)
                                                                                  {0x39, 0x04, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // CD Status Scan Backward (dd tt)
                                                                                  {0x39, 0x05, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // Mode 5 (dd tt) ??
                                                                                  {0x39, 0x06, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // Mode 6 (dd tt) ??
                                                                                  {0x39, 0x07, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},   // Track End Playing (dd tt)
                                                                                  {0x39, 0x08, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},  // CD Seeking (dd tt)
                                                                                  {0x39, 0x09, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1},  // CD Check
                                                                                  {0x39, 0x0A, 0x09, 0x00, MAGAZINE_CDS, 0x00, -1, -1}};  // No Magazine
int messagesToBeSentToRadioLength[AMOUNT_REGISTERED_MESSAGES_TO_BE_SENT_TO_RADIO] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

int messagesFromRadioToCDC[AMOUNT_REGISTERED_MESSAGES_FROM_RADIO_TO_CDC][LEN_COMMAND] = {{0x01},  // Radio Poll To CDC
                                                                                {0x38, 0x00, 0x00},  // Request current cd and track status
                                                                                {0x38, 0x01, 0x00},  // Stop Playing
                                                                                {0x38, 0x03, 0x00},  // Start Playing
                                                                                {0x38, 0x04, -1},  // Fast Scan (n 0x01 Backward n 0x00 Forward)
                                                                                {0x38, 0x06, -1},  // Change CD (n is CD Number)
                                                                                {0x38, 0x07, -1}, // Scan Intro Mode (n 0x01 Scan Mode ON, n 0x00 Scan Mode OFF)
                                                                                {0x38, 0x08, -1},  // Random Mode (n 0x01 Random Mode ON, n 0x00 Random Mode OFF)
                                                                                {0x38, 0x05, -1}}; // Change Track (n 0x01 Previous Track , n 0x00 Next Track)
int messagesFromRadioToCDCLength[AMOUNT_REGISTERED_MESSAGES_FROM_RADIO_TO_CDC] = {1, 3, 3, 3, 3, 3, 3, 3, 3};

int valueFromRadioToCDC = 0;

long millisSinceLastPoll = 0;

SoftwareSerial mySerial(2, 3);

void startBTModule(){
  digitalWrite(BT_BUTTON2, HIGH);
  digitalWrite(SLEEP_LED, HIGH);
  
  delay(5000);
  digitalWrite(BT_BUTTON2, LOW);
  digitalWrite(RADIO_LED, LOW);
  digitalWrite(BT_BUTTON1, HIGH);
  digitalWrite(RADIO_LED, HIGH);
  delay(5000);
  digitalWrite(BT_BUTTON1, LOW);
  digitalWrite(RADIO_LED, LOW);
  digitalWrite(BT_BUTTON3, HIGH);
  digitalWrite(RADIO_LED, HIGH);
  delay(5000);
  digitalWrite(BT_BUTTON3, LOW);
  digitalWrite(RADIO_LED, LOW);
}

void setup() {

  pinMode(SLEEP_LED, OUTPUT);
  pinMode(SEND_LED, OUTPUT);
  pinMode(READ_LED, OUTPUT);
  pinMode(RADIO_LED, OUTPUT);

  pinMode(BT_BUTTON1, OUTPUT);
  pinMode(BT_BUTTON2, OUTPUT);
  pinMode(BT_BUTTON3, OUTPUT);

//  startBTModule();
  
  // inicializo ambos puertos
  Serial.begin(9600, SERIAL_8E1);
  mySerial.begin(9600);
  millisSinceLastPoll = -30001;
  radioHasResponded = 0;
  mySerial.println("Modulo BT JM");
  digitalWrite(SLEEP_LED, HIGH);
  delay(100);
  digitalWrite(SLEEP_LED, LOW);
}

int calcCheckSum (int* pack, int packSize) {
 byte sum = 0;
 for (int i=0; i < (packSize); i++) sum = sum ^ pack[i];
 if(DEBUG){
  mySerial.print("checkeo checksum");
  mySerial.println(sum,HEX);
 }
 return sum;
}

void resetCommand(){
  if(topeCommand > 0){
    for(int i = 0; i < LEN_COMMAND; i++){
      command[i] = 0;
    }
    topeCommand = 0;
  }
}

void sendRadioMessage(int messageType){
  digitalWrite(SEND_LED, HIGH);
  delay(1);
  digitalWrite(SEND_LED, LOW);
  // primero recorremos el msj a ver que largo tiene
  int counter = messagesToBeSentToRadioLength[messageType];
  // ahora armamos el mensaje
  int toSend[counter + 4];
  toSend[COMMAND_FROM_INDEX] = MY_ID;
  toSend[COMMAND_LEN_INDEX] = counter + 2;
  toSend[COMMAND_FOR_INDEX] = RADIO;
  for(int i = COMMAND_START_INDEX; i < counter + 3; i++){
    toSend[i] = messagesToBeSentToRadio[messageType][i - 3];
  }
  toSend[COMMAND_CDC_DISC_INDEX] = cdcCurrentDisc;
  toSend[COMMAND_CDC_TRACK_INDEX] = cdcCurrentTrack;
  
  toSend[counter + 3] = calcCheckSum(toSend, counter + 3);
  // para estos mensajes, es necesario setear canción y disco
  // van en los indices COMMAND_CDC_DISC_INDEX y COMMAND_CDC_TRACK_INDEX
  
  for(int i = 0; i < counter + 3; i++){
    Serial.write(toSend[i]);
  }
  Serial.write(toSend[counter + 3]);

  mySerial.print("Message from CDChanger to Radio: ");
  if(messageType == CDC_COMMAND_CD_TRACK_STATUS_NP){
    mySerial.println("CDC_COMMAND_CD_TRACK_STATUS_NP");
  } else if(messageType == CDC_COMMAND_CD_TRACK_STATUS_PLAY){
    mySerial.println("CDC_COMMAND_CD_TRACK_STATUS_PLAY");
  } else if(messageType == CDC_COMMAND_CD_TRACK_STATUS_PAUSE){
    mySerial.println("CDC_COMMAND_CD_TRACK_STATUS_PAUSE");
  } else if(messageType == CDC_COMMAND_TRACK_STARTED_PLAYING){
    mySerial.println("CDC_COMMAND_TRACK_STARTED_PLAYING");
  } else if(messageType == CDC_COMMAND_CD_STATUS_SCAN_FORWARD){
    mySerial.println("CDC_COMMAND_CD_STATUS_SCAN_FORWARD");
  } else if(messageType == CDC_COMMAND_CD_STATUS_SCAN_BACKWARD){
    mySerial.println("CDC_COMMAND_CD_STATUS_SCAN_BACKWARD");
  } else if(messageType == CDC_COMMAND_MODE_5){
    mySerial.println("CDC_COMMAND_MODE_5");
  } else if(messageType == CDC_COMMAND_MODE_6){
    mySerial.println("CDC_COMMAND_MODE_6");
  } else if(messageType == CDC_COMMAND_TRACK_END_PLAYING){
    mySerial.println("CDC_COMMAND_TRACK_END_PLAYING");
  } else if(messageType == CDC_COMMAND_CD_SEEKING){
    mySerial.println("CDC_COMMAND_CD_SEEKING");
  } else if(messageType == CDC_COMMAND_CD_CHECK){
    mySerial.println("CDC_COMMAND_CD_CHECK");
  } else if(messageType == CDC_COMMAND_NO_MAGAZINE){
    mySerial.println("CDC_COMMAND_NO_MAGAZINE");
  } else {
    mySerial.println("UNKNOWN COMMAND");
  }

  if(ECHO){
    for(int i = 0; i < counter + 3; i++){
      mySerial.print(toSend[i], HEX);
      mySerial.print("-");
    }
    mySerial.println(toSend[counter + 3], HEX);
  }
}

void sendBroadcastMessage(int messageType){
  digitalWrite(SEND_LED, HIGH);
  delay(1);
  digitalWrite(SEND_LED, LOW);
  // primero recorremos el msj a ver que largo tiene
  int counter = broadcastMessagesLength[messageType];
  // ahora armamos el mensaje
  int toSend[counter + 4];
  toSend[COMMAND_FROM_INDEX] = MY_ID;
  toSend[COMMAND_LEN_INDEX] = counter + 2;
  toSend[COMMAND_FOR_INDEX] = BROADCAST;
  for(int i = COMMAND_START_INDEX; i < counter + 3; i++){
    toSend[i] = broadcastMessages[messageType][i - 3];
  }
  toSend[counter + 3] = calcCheckSum(toSend, counter + 3);
  for(int i = 0; i < counter + 3; i++){
    Serial.write(toSend[i]);
  }
  Serial.write(toSend[counter + 3]);

  mySerial.print("Broadcast Message from CDC: ");
  if(messageType == CDC_BROADCAST_COMMAND_POLL_RESPONSE){
    mySerial.println("CDC_BROADCAST_COMMAND_POLL_RESPONSE");
  } else if(messageType == CDC_BROADCAST_COMMAND_ANNOUNCE){
    mySerial.println("CDC_BROADCAST_COMMAND_ANNOUNCE");
  } else {
    mySerial.println("UNKNOWN COMMAND");
  }

  if(ECHO){
    for(int i = 0; i < counter + 3; i++){
      mySerial.print(toSend[i], HEX);
      mySerial.print("-");
    }
    mySerial.println(toSend[counter + 3], HEX);
  }
}

int detectMessageTypeRadioToCDC(){
  int messageLength = command[COMMAND_LEN_INDEX] - 2;
  for(int p = 0; p < AMOUNT_REGISTERED_MESSAGES_FROM_RADIO_TO_CDC; p++){
    // primero purgamos lista por tamaño
    if(messagesFromRadioToCDCLength[p] == messageLength){
      int incorrectByteNOTDetected = 1;
      for(int i = 0; i < messageLength && incorrectByteNOTDetected; i++){
        if(messagesFromRadioToCDC[p][i] == -1){
          // analizo caso
          valueFromRadioToCDC = command[COMMAND_START_INDEX + i];
        } else if (messagesFromRadioToCDC[p][i] == command[COMMAND_START_INDEX + i]){
          // sigo parseando correctamente
        } else {
          // marco el msj como seguro incorrecto
          incorrectByteNOTDetected = 0;
        }
      }
      // ahora chequeo por que razón salí del for
      if(incorrectByteNOTDetected){
        return p;
      }
    }
  }
  return -1;
}

void printInSerialCommand(int startIndex, int stopIndex){
  for(int i = startIndex; i < stopIndex - 1; i++){
      mySerial.print(command[i], HEX);
      mySerial.print("-");
  }
  mySerial.print(command[stopIndex - 1], HEX);
  mySerial.println("");
}

void analyzeSteeringWheelCommand(){
//  Serial.println("NOT YET IMPLEMENTED");
}

void analyzeCDCCommand(){
//  Serial.println("NOT YET IMPLEMENTED");
}

void analyzeRadioCommand(){
  digitalWrite(RADIO_LED, HIGH);
  delay(1);
  digitalWrite(RADIO_LED, LOW);
  byte to = command[COMMAND_FOR_INDEX];
  if(to == BROADCAST){
    mySerial.print("Broadcast Message from Radio: ");
    printInSerialCommand(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
  } else if (to == CDC) {
    mySerial.print("Message from Radio to CDChanger: ");
    int message = detectMessageTypeRadioToCDC();
    if(VERBOSE){
      printInSerialCommand(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
    }
    switch (message){
      case RADIO_COMMAND_POLL:
        mySerial.println("RADIO_COMMAND_POLL");
        sendBroadcastMessage(CDC_BROADCAST_COMMAND_POLL_RESPONSE);
        radioHasResponded = 1;
        break;
      case RADIO_COMMAND_REQUEST_CURRENT_STATUS:
        mySerial.println("RADIO_COMMAND_REQUEST_CURRENT_STATUS");
        break;
      case RADIO_COMMAND_STOP_PLAYING:
        mySerial.println("RADIO_COMMAND_STOP_PLAYING");
        cdcState = CDC_STATE_STOP;
        sendRadioMessage(CDC_COMMAND_TRACK_END_PLAYING);
        break;
      case RADIO_COMMAND_START_PLAYING:
        mySerial.println("RADIO_COMMAND_START_PLAYING");
        cdcState = CDC_STATE_PLAY;
        sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_NP);
        sendRadioMessage(CDC_COMMAND_TRACK_END_PLAYING);
        break;
      case RADIO_COMMAND_FAST_SCAN:
        mySerial.println("RADIO_COMMAND_FAST_SCAN");
        // aquí tengo en valueFromRadioToCDC = 0 si es forward y valueFromRadioToCDC = 1 si es backwards
        sendRadioMessage(CDC_COMMAND_CD_STATUS_SCAN_FORWARD);
        break;
      case RADIO_COMMAND_CHANGE_CD:
        mySerial.println("RADIO_COMMAND_CHANGE_CD");
        // en este caso tengo en valueFromRadioToCDC el cd al cual cambiar
        cdcCurrentDisc = valueFromRadioToCDC;
        break;
      case RADIO_COMMAND_SCAN:
        mySerial.println("RADIO_COMMAND_SCAN");
        // aquí tengo en valueFromRadioToCDC = 0 si se deshabilita scan y valueFromRadioToCDC = 1 si se habilita
        break;
      case RADIO_COMMAND_RANDOM:
        mySerial.println("RADIO_COMMAND_RANDOM");
        // aquí tengo en valueFromRadioToCDC = 0 si random está deshabilitado y valueFromRadioToCDC = 1 si se habilita
        break;
      case RADIO_COMMAND_CHANGE_TRACK:
        mySerial.println("RADIO_COMMAND_CHANGE_TRACK");
        // aquí tengo en valueFromRadioToCDC = 0 si es la next y valueFromRadioToCDC = 1 si es la anterior
        if(valueFromRadioToCDC == 0x00){
          if(cdcCurrentTrack < 100){
            cdcCurrentTrack = cdcCurrentTrack + 1;
          }
        } else {
          if(cdcCurrentTrack > 1){
            cdcCurrentTrack = cdcCurrentTrack - 1;
          }
        }
        break;
      default:
        mySerial.println("UNKNOWN");
        break;
    }
    if(cdcState == CDC_STATE_PLAY){
      sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_PLAY);
    } else {
      sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_NP);
    }
  } else {
    if(OTHER_MESSAGES){
      mySerial.print("Message from Radio to ");
      mySerial.print(command[COMMAND_FOR_INDEX], HEX);
      mySerial.print(": ");
    }
    printInSerialCommand(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
  }
  
}

void analyzeCommand(){
  byte from = command[COMMAND_FROM_INDEX];
  if(from == RADIO){
    analyzeRadioCommand();
  }
  if(from == CDC){
    analyzeCDCCommand();
  }
  if(from == STEERING_WHEEL){
    analyzeSteeringWheelCommand();
  }
}

void checkIfCommandValid(){
  if(topeCommand > 2){
    //chequeamos largo
    int largo = command[COMMAND_LEN_INDEX];
    if(largo + 2 == topeCommand){
      //ahora verificamos checksum
      int checksum = calcCheckSum(command, topeCommand - 1);
      if(DEBUG){
        mySerial.print("paso check de largo, checksum a verificar: ");
        mySerial.print(checksum, HEX);
        mySerial.print(" vs ");
        mySerial.println(command[topeCommand-1], HEX);
      }
      if(checksum == command[topeCommand - 1]){
        // el comando parece valido!
        if(VERBOSE){
          mySerial.print("Se recibio comando desde: ");
          mySerial.println(command[COMMAND_FROM_INDEX], HEX);
          mySerial.print("Se recibio comando para:  ");
          mySerial.println(command[COMMAND_FOR_INDEX], HEX);
          mySerial.print("De largo: ");
          mySerial.println(command[COMMAND_LEN_INDEX], HEX);
          mySerial.print("Conteniendo: ");
          for(int i = 3; i < topeCommand - 1; i++){
            if(i == topeCommand - 1){
              mySerial.println(command[i], HEX);
            } else {
              mySerial.print(command[i], HEX);
              mySerial.print(" ");
            }
          }
          mySerial.print("Con checksum: ");
          mySerial.println(command[topeCommand - 1], HEX);
        }
        analyzeCommand();
      } else {
        // Descartamos comando parece incorrecto
        mySerial.println("Comando incorrecto!");
        mySerial.print("Se recibio comando desde: ");
        mySerial.println(command[COMMAND_FROM_INDEX], HEX);
        mySerial.print("Se recibio comando para:  ");
        mySerial.println(command[COMMAND_FOR_INDEX], HEX);
        mySerial.print("De largo: ");
        mySerial.println(command[COMMAND_LEN_INDEX], HEX);
        mySerial.print("Conteniendo: ");
        for(int i = 3; i < topeCommand - 1; i++){
          if(i == topeCommand - 1){
            mySerial.println(command[i], HEX);
          } else {
            mySerial.print(command[i], HEX);
            mySerial.print(" ");
          }
        }
        mySerial.print("Con checksum: ");
        mySerial.println(command[topeCommand - 1], HEX);
        while(Serial.available() > 0) {
          char t = Serial.read();
        }
        while(mySerial.available() > 0) {
          char t = mySerial.read();
        }
      } 
      resetCommand();
    }
  }
}

void sendPoll(){
  // si pasaron 30 segundos mando poll
  long currentMillis = millis();
  if(currentMillis - millisSinceLastPoll > 30000){
    if(!radioHasResponded){
      if(VERBOSE){
        mySerial.println("Polling! (Announce)");
      }
      sendBroadcastMessage(CDC_BROADCAST_COMMAND_ANNOUNCE);
    }
    millisSinceLastPoll = currentMillis;
  }
}

void addByteToCommand(int incomingByte){
  command[topeCommand] = incomingByte;
  if(DEBUG){
    mySerial.print("agrego byte ");
    mySerial.println(incomingByte, HEX);
  }
  topeCommand++;
  checkIfCommandValid();
}

//void sendButtonPush(){
//  Serial.write(0x50);
//  Serial.write(0x04);
//  Serial.write(0x68);
//  Serial.write(0x3b);
//  Serial.write(0x01);
//  Serial.write(0x06);
//  mySerial.println("button push");
//}
//
//void sendButtonRelease(){
//  Serial.write(0x50);
//  Serial.write(0x04);
//  Serial.write(0x68);
//  Serial.write(0x3b);
//  Serial.write(0x21);
//  Serial.write(0x26);
//  mySerial.println("button release");
//}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    addByteToCommand(incomingByte);
    digitalWrite(READ_LED, HIGH);
    delay(1);
    digitalWrite(READ_LED, LOW);
    counter = 0;
  } else {
    counter = counter + 1;
  }
    
  if (counter > 300000) {
    resetCommand();
    counter = 0;
  }

  sendPoll();
//
//  if(digitalRead(38) == HIGH && lastState == LOW){
//    sendButtonPush();
//    lastState = HIGH;
//  } else if (digitalRead(38) == LOW && lastState == HIGH){
//    sendButtonRelease();
//    lastState = LOW;
//  }
}
