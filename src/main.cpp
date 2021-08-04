#include <Arduino.h>
#include <avr/pgmspace.h>

#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

const uint8_t MAX_LENGHT_MSG = 250;
const uint8_t FONE_DIGITS = 15;

typedef struct gpsData
{
  float latitude;
  float longitude;
} GPSData;

typedef struct message
{
  int8_t index = 0;
  char number[FONE_DIGITS] = {0};
  char text[MAX_LENGHT_MSG] = {0};
  uint16_t *lenght;
} Message;

GPSData *getGPSData(GPSData *loc);
void deleteAllSMS();
bool getSMS(Message *receivedSMS);
void sendActualLocation(char *sendTo);
void activeTracking(bool tracking, char *sendTo, uint8_t trackingDelayMin);
int freeRam();

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
GPSData *location = (GPSData *)malloc(sizeof(GPSData));
Message *msg = (Message *)malloc(sizeof(Message));

const char mainMenu[] PROGMEM = "Ola.\nResponda com a opcao:\n(1)Ativar/Desativar GPS.\n(2)Localizar.\n(>=3)Configurar tempo.\n(0)Cancelar. \n";
const char msgError[] PROGMEM = "Erro na tentativa.\n";
//const char foneNumber[] PROGMEM = "+5541996070984";
const char foneNumber[] PROGMEM = "+5541991940319";
const char linkGmaps[] PROGMEM = "Latitude: %s\nLongitude: %s\nhttp://maps.google.com/maps?q=%s,%s\n";

bool onMenu = false;
bool responseSMS = false;
bool tracking = false;
uint8_t trackingDelayMin = 1;
char txtBuffer[MAX_LENGHT_MSG];

void setup()
{
  while (!Serial)
    ;
  Serial.begin(115200);
  Serial.print(F("Rastreador"));
  Serial.println(F("Iniciando..(Pode levar alguns segundos)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("SIM808 não localizado"));
  }

  Serial.println(F("SIM808 OK"));
  Serial.println(F("Testando GPS."));

  fona.enableGPS(true);

  if (!(msg || location))
  {
    Serial.println(F("Variaveis não alocadas"));
    while (1)
      ;
  }

  location = getGPSData(location);

  // Print SIM card IMEI number.
  char imei[16] = {0};
  uint8_t imeiLen = fona.getIMEI(imei);

  if (imeiLen > 0)
  {
    Serial.print(F("SIM card IMEI: "));
    Serial.println(imei);
  }

  deleteAllSMS();

  //set up the FONA to send a +CMTI notification when an SMS is received
  fonaSerial->print("AT+CNMI=2,1\r\n");

  Serial.println(F("Setup Finalizado."));
}

void loop()
{
  onMenu = getSMS(msg);
  while (onMenu)
  {
    if (strcmp_P(msg->number, foneNumber) != 0)
    {
      memcpy_P(txtBuffer, msgError, sizeof(msgError));

      onMenu = false;
      Serial.print(msg->number);

      memcpy_P(msg->number, foneNumber, sizeof(foneNumber));

      fona.sendSMS(msg->number, txtBuffer);
      deleteAllSMS();
      break;
    }
    else
    {
      memcpy_P(txtBuffer, mainMenu, sizeof(mainMenu));

      fona.sendSMS(msg->number, txtBuffer);

      while (!responseSMS)
      {
        responseSMS = getSMS(msg);

        if (msg->text[0] == '1')
          tracking = !tracking;
        else if (msg->text[0] == '2')
          sendActualLocation(msg->number);
        else if (msg->text[0] >= '3')
          trackingDelayMin = msg->text[0] - '0';
        else if (msg->text[0] == '0')
          onMenu = false;
        else
          responseSMS = false;
      }
    }
  }
  activeTracking(tracking, msg->number, trackingDelayMin);
  deleteAllSMS();
  Serial.print(F("Esperando SMS.\n"));
  Serial.println(freeRam());
  delay(1000);
};

GPSData *getGPSData(GPSData *loc)
{
  bool gps_success = fona.getGPS(&loc->latitude, &loc->longitude);
  Serial.println(freeRam());
  while (!gps_success)
  {
    Serial.println(freeRam());
    Serial.print(F("Aguardando sinal dos satélites...\n"));
    gps_success = fona.getGPS(&loc->latitude, &loc->longitude);
    delay(10000);
  }
  return loc;
}

void deleteAllSMS()
{
  int indexSMS = fona.getNumSMS();
  for (uint8_t count = 1; count <= indexSMS; count++)
    fona.deleteSMS(count);
}

bool getSMS(Message *recivedSMS)
{

  bool readMsg = false;
  recivedSMS->index = fona.getNumSMS();
  if (recivedSMS->index > 0)
  {
    fona.getSMSSender(recivedSMS->index, recivedSMS->number, MAX_LENGHT_MSG);
    readMsg = fona.readSMS(recivedSMS->index, recivedSMS->text, MAX_LENGHT_MSG, recivedSMS->lenght);
    fona.deleteSMS(recivedSMS->index);
    Serial.println(readMsg);
  }

  return readMsg;
}

void sendActualLocation(char *sendTo)
{
  char lat[12];
  char lon[12];

  location = getGPSData(location);

  dtostrf(location->latitude, 8, 6, lat);
  dtostrf(location->longitude, 8, 6, lon);

  memcpy_P(txtBuffer, linkGmaps, sizeof(linkGmaps));
  sprintf(txtBuffer, txtBuffer, lat, lon, lat, lon);

  //Serial.println(txtBuffer);

  fona.sendSMS(sendTo, txtBuffer);
}

void activeTracking(bool tracking, char *sendTo, uint8_t trackingDelayMin)
{
  if (tracking)
  {
    sendActualLocation(sendTo);
    delay(trackingDelayMin * 60000);
  }
}

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}