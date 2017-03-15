

#define MY_ID 0x18

#define LEN_COMMAND 20
#define COMMAND_FROM_INDEX 0
#define COMMAND_LEN_INDEX 1
#define COMMAND_FOR_INDEX 2
#define COMMAND_START_INDEX 3
#define COMMAND_CDC_DISC_INDEX 9
#define COMMAND_CDC_TRACK_INDEX 10

#define DEBUG 1
#define VERBOSE 1
#define ECHO 1
#define OTHER_MESSAGES 1

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




int incomingByte = 0;   // for incoming Serial1 data
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

//SoftwareSerial1 Serial1(10, 11);

void setup() {
  // inicializo ambos puertos
  Serial1.begin(9600, SERIAL_8E1);
  Serial.begin(9600);
  millisSinceLastPoll = -30001;
  radioHasResponded = 0;
  pinMode(38, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

int calcCheckSum (int* pack, int packSize) {
 byte sum = 0;
 for (int i=0; i < (packSize); i++) sum = sum ^ pack[i];
 if(DEBUG){
  Serial.print("checkeo checksum");
  Serial.println(sum,HEX);
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
    Serial1.write(toSend[i]);
  }
  Serial1.write(toSend[counter + 3]);

  Serial.print("Message from CDChanger to Radio: ");
  if(messageType == CDC_COMMAND_CD_TRACK_STATUS_NP){
    Serial.println("CDC_COMMAND_CD_TRACK_STATUS_NP");
  } else if(messageType == CDC_COMMAND_CD_TRACK_STATUS_PLAY){
    Serial.println("CDC_COMMAND_CD_TRACK_STATUS_PLAY");
  } else if(messageType == CDC_COMMAND_CD_TRACK_STATUS_PAUSE){
    Serial.println("CDC_COMMAND_CD_TRACK_STATUS_PAUSE");
  } else if(messageType == CDC_COMMAND_TRACK_STARTED_PLAYING){
    Serial.println("CDC_COMMAND_TRACK_STARTED_PLAYING");
  } else if(messageType == CDC_COMMAND_CD_STATUS_SCAN_FORWARD){
    Serial.println("CDC_COMMAND_CD_STATUS_SCAN_FORWARD");
  } else if(messageType == CDC_COMMAND_CD_STATUS_SCAN_BACKWARD){
    Serial.println("CDC_COMMAND_CD_STATUS_SCAN_BACKWARD");
  } else if(messageType == CDC_COMMAND_MODE_5){
    Serial.println("CDC_COMMAND_MODE_5");
  } else if(messageType == CDC_COMMAND_MODE_6){
    Serial.println("CDC_COMMAND_MODE_6");
  } else if(messageType == CDC_COMMAND_TRACK_END_PLAYING){
    Serial.println("CDC_COMMAND_TRACK_END_PLAYING");
  } else if(messageType == CDC_COMMAND_CD_SEEKING){
    Serial.println("CDC_COMMAND_CD_SEEKING");
  } else if(messageType == CDC_COMMAND_CD_CHECK){
    Serial.println("CDC_COMMAND_CD_CHECK");
  } else if(messageType == CDC_COMMAND_NO_MAGAZINE){
    Serial.println("CDC_COMMAND_NO_MAGAZINE");
  } else {
    Serial.println("UNKNOWN COMMAND");
  }

  if(ECHO){
    for(int i = 0; i < counter + 3; i++){
      Serial.print(toSend[i], HEX);
      Serial.print("-");
    }
    Serial.println(toSend[counter + 3], HEX);
  }
}

void sendBroadcastMessage(int messageType){
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
    Serial1.write(toSend[i]);
  }
  Serial1.write(toSend[counter + 3]);

  Serial.print("Broadcast Message from CDC: ");
  if(messageType == CDC_BROADCAST_COMMAND_POLL_RESPONSE){
    Serial.println("CDC_BROADCAST_COMMAND_POLL_RESPONSE");
  } else if(messageType == CDC_BROADCAST_COMMAND_ANNOUNCE){
    Serial.println("CDC_BROADCAST_COMMAND_ANNOUNCE");
  } else {
    Serial.println("UNKNOWN COMMAND");
  }

  if(ECHO){
    for(int i = 0; i < counter + 3; i++){
      Serial.print(toSend[i], HEX);
      Serial.print("-");
    }
    Serial.println(toSend[counter + 3], HEX);
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

void printInSerial1Command(int startIndex, int stopIndex){
  for(int i = startIndex; i < stopIndex - 1; i++){
      Serial.print(command[i], HEX);
      Serial.print("-");
  }
  Serial.print(command[stopIndex - 1], HEX);
  Serial.println("");
}

void analyzeSteeringWheelCommand(){
//  Serial1.println("NOT YET IMPLEMENTED");
}

void analyzeCDCCommand(){
//  Serial1.println("NOT YET IMPLEMENTED");
}

void analyzeRadioCommand(){
  byte to = command[COMMAND_FOR_INDEX];
  if(to == BROADCAST){
    Serial.print("Broadcast Message from Radio: ");
    printInSerial1Command(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
  } else if (to == CDC) {
    Serial.print("Message from Radio to CDChanger: ");
    int message = detectMessageTypeRadioToCDC();
    if(VERBOSE){
      printInSerial1Command(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
    }
    switch (message){
      case RADIO_COMMAND_POLL:
        Serial.println("RADIO_COMMAND_POLL");
        sendBroadcastMessage(CDC_BROADCAST_COMMAND_POLL_RESPONSE);
        radioHasResponded = 1;
        break;
      case RADIO_COMMAND_REQUEST_CURRENT_STATUS:
        Serial.println("RADIO_COMMAND_REQUEST_CURRENT_STATUS");
        break;
      case RADIO_COMMAND_STOP_PLAYING:
        Serial.println("RADIO_COMMAND_STOP_PLAYING");
        cdcState = CDC_STATE_STOP;
        sendRadioMessage(CDC_COMMAND_TRACK_END_PLAYING);
        break;
      case RADIO_COMMAND_START_PLAYING:
        Serial.println("RADIO_COMMAND_START_PLAYING");
        cdcState = CDC_STATE_PLAY;
        sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_NP);
        sendRadioMessage(CDC_COMMAND_TRACK_END_PLAYING);
        break;
      case RADIO_COMMAND_FAST_SCAN:
        Serial.println("RADIO_COMMAND_FAST_SCAN");
        // aquí tengo en valueFromRadioToCDC = 0 si es forward y valueFromRadioToCDC = 1 si es backwards
        sendRadioMessage(CDC_COMMAND_CD_STATUS_SCAN_FORWARD);
        break;
      case RADIO_COMMAND_CHANGE_CD:
        Serial.println("RADIO_COMMAND_CHANGE_CD");
        // en este caso tengo en valueFromRadioToCDC el cd al cual cambiar
        cdcCurrentDisc = valueFromRadioToCDC;
        break;
      case RADIO_COMMAND_SCAN:
        Serial.println("RADIO_COMMAND_SCAN");
        // aquí tengo en valueFromRadioToCDC = 0 si se deshabilita scan y valueFromRadioToCDC = 1 si se habilita
        break;
      case RADIO_COMMAND_RANDOM:
        Serial.println("RADIO_COMMAND_RANDOM");
        // aquí tengo en valueFromRadioToCDC = 0 si random está deshabilitado y valueFromRadioToCDC = 1 si se habilita
        break;
      case RADIO_COMMAND_CHANGE_TRACK:
        Serial.println("RADIO_COMMAND_CHANGE_TRACK");
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
        Serial.println("UNKNOWN");
        break;
    }
    if(cdcState == CDC_STATE_PLAY){
      sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_PLAY);
    } else {
      sendRadioMessage(CDC_COMMAND_CD_TRACK_STATUS_NP);
    }
  } else {
    if(OTHER_MESSAGES){
      Serial.print("Message from Radio to ");
      Serial.print(command[COMMAND_FOR_INDEX], HEX);
      Serial.print(": ");
    }
    printInSerial1Command(COMMAND_START_INDEX, command[COMMAND_LEN_INDEX] + 1);
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
        Serial.print("paso check de largo, checksum a verificar: ");
        Serial.print(checksum, HEX);
        Serial.print(" vs ");
        Serial.println(command[topeCommand-1], HEX);
      }
      if(checksum == command[topeCommand - 1]){
        // el comando parece valido!
        if(VERBOSE){
          Serial.print("Se recibio comando desde: ");
          Serial.println(command[COMMAND_FROM_INDEX], HEX);
          Serial.print("Se recibio comando para:  ");
          Serial.println(command[COMMAND_FOR_INDEX], HEX);
          Serial.print("De largo: ");
          Serial.println(command[COMMAND_LEN_INDEX], HEX);
          Serial.print("Conteniendo: ");
          for(int i = 3; i < topeCommand - 1; i++){
            if(i == topeCommand - 1){
              Serial.println(command[i], HEX);
            } else {
              Serial.print(command[i], HEX);
              Serial.print(" ");
            }
          }
          Serial.print("Con checksum: ");
          Serial.println(command[topeCommand - 1], HEX);
        }
        analyzeCommand();
      } else {
        // Descartamos comando parece incorrecto
        Serial.println("Comando incorrecto!");
        Serial.print("Se recibio comando desde: ");
        Serial.println(command[COMMAND_FROM_INDEX], HEX);
        Serial.print("Se recibio comando para:  ");
        Serial.println(command[COMMAND_FOR_INDEX], HEX);
        Serial.print("De largo: ");
        Serial.println(command[COMMAND_LEN_INDEX], HEX);
        Serial.print("Conteniendo: ");
        for(int i = 3; i < topeCommand - 1; i++){
          if(i == topeCommand - 1){
            Serial.println(command[i], HEX);
          } else {
            Serial.print(command[i], HEX);
            Serial.print(" ");
          }
        }
        Serial.print("Con checksum: ");
        Serial.println(command[topeCommand - 1], HEX);
        while(Serial1.available() > 0) {
          char t = Serial1.read();
        }
        while(Serial1.available() > 0) {
          char t = Serial1.read();
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
        Serial.println("Polling! (Announce)");
      }
      sendBroadcastMessage(CDC_BROADCAST_COMMAND_ANNOUNCE);
    }
    millisSinceLastPoll = currentMillis;
  }
}

void addByteToCommand(int incomingByte){
  command[topeCommand] = incomingByte;
  if(DEBUG){
    Serial.print("agrego byte ");
    Serial.println(incomingByte, HEX);
  }
  topeCommand++;
  checkIfCommandValid();
}

void sendButtonPush(){
  Serial1.write(0x50);
  Serial1.write(0x04);
  Serial1.write(0x68);
  Serial1.write(0x3b);
  Serial1.write(0x01);
  Serial1.write(0x06);
  Serial.println("button push");
}

void sendButtonRelease(){
  Serial1.write(0x50);
  Serial1.write(0x04);
  Serial1.write(0x68);
  Serial1.write(0x3b);
  Serial1.write(0x21);
  Serial1.write(0x26);
  Serial.println("button release");
}

void loop() {
  digitalWrite(13, LOW);
  if (Serial1.available() > 0) {
    incomingByte = Serial1.read();
    addByteToCommand(incomingByte);
    counter = 0;
    digitalWrite(13, HIGH);
    delay(1);
  } else {
    counter = counter + 1;
  }
  
    
  if (counter > 300000) {
    resetCommand();
    counter = 0;
  }

  sendPoll();

  if(digitalRead(38) == HIGH && lastState == LOW){
    sendButtonPush();
    lastState = HIGH;
  } else if (digitalRead(38) == LOW && lastState == HIGH){
    sendButtonRelease();
    lastState = LOW;
  }
}
