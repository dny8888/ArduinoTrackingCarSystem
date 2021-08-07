#include <Arduino.h>

#include <SoftwareSerial.h>

#include "Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define OK_LED 8

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

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

GPSData location;
Message msg;

char phoneOwner[] = "+5541991940319";
char linkGmaps[] = "Latitude: %s\nLongitude: %s\nhttp://maps.google.com/maps?q=%s,%s\n";
char mainMenu[] = "[R]Ativar/Desativar GPS\n[L]Localizar.\n[T(1~9)]Configurar tempo\n";

char txtBuffer[MAX_LENGHT_MSG];
char phoneBuffer[PHONE_DIGITS];

const uint16_t oneMinInMillis = 60000;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
uint8_t trackingDelayMin = 1;

bool delayTrigger = false;
bool getTrigger = false;
bool smsTrigger = false;
bool onMenu = false;
bool tracking = false;

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

  pinMode(OK_LED, OUTPUT);
  digitalWrite(OK_LED, HIGH);
}

void loop()
{
  currentMillis = millis();
  if (currentMillis - previousMillis >= (trackingDelayMin * oneMinInMillis))
  {
    delayTrigger = true;
    previousMillis = currentMillis;
  }

  onMenu = getSMS(&msg);

  while (onMenu)
  {
    if (msg.text[0] == 'R')
      getTrigger = !getTrigger;
    else if (msg.text[0] == 'L')
      smsTrigger = true;
    else if (msg.text[0] == 'T')
      trackingDelayMin = msg.text[2] - '0';
    else
      fona.sendSMS(phoneOwner, mainMenu);
    onMenu = false;
    deleteAllSMS();
  }

  Serial.println(currentMillis);
  Serial.println(delayTrigger);
  Serial.println(getTrigger);

  if ((delayTrigger && getTrigger) || smsTrigger)
  {
    sendActualLocation(phoneOwner);
    delayTrigger = false;
    smsTrigger = false;
  }

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
