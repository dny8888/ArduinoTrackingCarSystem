#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define OK_LED 8

// Endereco I2C do sensor MPU-6050
const int MPU = 0x68;

const uint8_t MAX_LENGHT_MSG = 70;
const uint8_t PHONE_DIGITS = 15;

typedef struct gpsData
{
  float latitude = 0;
  float longitude = 0;
} GPSData;

typedef struct message
{
  int8_t index = 0;
  char number[PHONE_DIGITS] = {0};
  char text[MAX_LENGHT_MSG] = {0};
  uint16_t *lenght = NULL;
} Message;

void deleteAllSMS();
bool getSMS(Message *receivedSMS);
void sendActualLocation(char *sendTo);
bool getAccMove();

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

GPSData location;
Message msg;

char phoneOwner[] = "+5541991940319";
char linkGmaps[] = "Latitude: %s\nLongitude: %s\nhttp://maps.google.com/maps?q=%s,%s\n";
char mainMenu[] = "[R]Ativar/Desativar GPS\n[L]Localizar.\n[T(1~9)]Configurar tempo.Ex:T8\n";

char txtBuffer[MAX_LENGHT_MSG];
char phoneBuffer[PHONE_DIGITS];

const uint16_t oneMinInMillis = 60000;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
uint8_t trackingDelayMin = 1;

bool delayTrigger = 1;
bool getTrigger = 0;
bool smsTrigger = 0;
bool accTrigger = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Rastreador"));
  Serial.println(F("Iniciando..(Pode levar alguns segundos)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("SIM808 não localizado"));
    while (1)
      ;
  }

  Serial.println(F("SIM808 OK"));
  Serial.println(F("Testando GPS."));

  fona.enableGPS(true);

  // Solicitando dados gps e passando para variavel location
  while (!fona.getGPS(&(location.latitude), &(location.longitude)))
  {
    Serial.print(F("Aguardando sinal dos satélites...\n"));
    delay(10000);
  };

  deleteAllSMS();

  //set up the FONA to send a +CMTI notification when an SMS is received
  fonaSerial->print("AT+CNMI=2,1\r\n");

  Serial.println(F("Setup Finalizado."));

  // Inicializa o MPU-6050
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  // Configura Acelerometro para fundo de escala desejado
  //
  //      Wire.write(0b00000000); // fundo de escala em +/-2g
  //      Wire.write(0b00001000); // fundo de escala em +/-4g
  //      Wire.write(0b00010000); // fundo de escala em +/-8g
  //      Wire.write(0b00011000); // fundo de escala em +/-16g
  //
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);
  Wire.write(0b00011000); // Trocar esse comando para fundo de escala desejado conforme acima
  Wire.endTransmission();

  pinMode(OK_LED, OUTPUT);
  digitalWrite(OK_LED, HIGH);
}

void loop()
{
  currentMillis = millis();

  //Verifica se passou intervalo.
  if (currentMillis - previousMillis >= (trackingDelayMin * oneMinInMillis))
  {
    delayTrigger = 1;
    previousMillis = currentMillis;
  }

  //Verifica se recebeu mensagem.
  if (getSMS(&msg))
  {
    if (msg.text[0] == 'R')
      getTrigger = getTrigger == 0 ? 1 : 0;
    else if (msg.text[0] == 'L')
      smsTrigger = 1;
    else if (msg.text[0] == 'T')
      //pega o 2º caracter e converte em um inteiro
      trackingDelayMin = msg.text[1] - '0';
    else
      fona.sendSMS(phoneOwner, mainMenu);
    deleteAllSMS();
  }

  //Verifica se houve movimentação
  if (getAccMove())
  {
    accTrigger = 1;
  }

  Serial.print(F("delayTrigger:"));
  Serial.println(delayTrigger ? "TRUE" : "FALSE");
  Serial.print(F("getTrigger:"));
  Serial.println(getTrigger ? "TRUE" : "FALSE");
  Serial.print(F("smsTrigger:"));
  Serial.println(smsTrigger ? "TRUE" : "FALSE");
  Serial.print(F("accTrigger:"));
  Serial.println(accTrigger ? "TRUE" : "FALSE");
  Serial.print(F("trackingDelayMin:"));
  Serial.println(trackingDelayMin);

  //Envia localização se rastreamento está ativo E passou o intervalo
  if (delayTrigger && getTrigger)
  {
    sendActualLocation(phoneOwner);
    delayTrigger = 0;
  };

  //Envia localização se rastreamento está ativo E houve movimentação
  if (getTrigger && accTrigger)
  {
    sendActualLocation(phoneOwner);
    accTrigger = 0;
    trackingDelayMin = 1;
  };

  // Envia localização se solicitado por mensagem.
  if (smsTrigger)
  {
    sendActualLocation(phoneOwner);
    smsTrigger = 0;
  };

  delay(1000);
}

void deleteAllSMS()
{
  int indexSMS = fona.getNumSMS();
  for (uint8_t count = 1; count <= indexSMS; count++)
    fona.deleteSMS(count);
}

bool getSMS(Message *sms)
{
  sms->index = fona.getNumSMS();

  if (sms->index > 0)
  {
    fona.getSMSSender(sms->index, sms->number, MAX_LENGHT_MSG);
    return fona.readSMS(sms->index, sms->text, MAX_LENGHT_MSG, sms->lenght);
  }
  return false;
}

void sendActualLocation(char *sendTo)
{
  char lat[12];
  char lon[12];

  // Solicitando dados gps e passando para variavel location
  while (!fona.getGPS(&(location.latitude), &(location.longitude)))
  {
    Serial.println(F("Aguardando satelites..."));
    delay(1000);
  };
  //tranformando as variaveis float em char*
  dtostrf(location.latitude, 8, 6, lat);
  dtostrf(location.longitude, 8, 6, lon);

  //concatenando dados no buffer de texto
  sprintf(txtBuffer, linkGmaps, lat, lon, lat, lon);

  //Solicitando Envio
  fona.sendSMS(sendTo, txtBuffer);
}

bool getAccMove()
{
  float AccX, AccY, AccZ, accVector;

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true); // Solicita os dados ao sensor

  // Armazena o valor dos sensores nas variaveis correspondentes
  AccX = Wire.read() << 8 | Wire.read(); //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AccY = Wire.read() << 8 | Wire.read(); //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AccZ = Wire.read() << 8 | Wire.read(); //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)

  AccX /= 2048;
  AccY /= 2048;
  AccZ /= 2048;
  accVector = sqrt(AccX * AccX + AccY * AccY + AccZ * AccZ);

  return accVector > 2 ? 1 : 0;
  // Imprime na Serial os valores obtidos
  /* Alterar divisão conforme fundo de escala escolhido:
      Acelerômetro
      +/-2g = 16384
      +/-4g = 8192
      +/-8g = 4096
      +/-16g = 2048
*/
}