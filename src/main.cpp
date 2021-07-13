#include <Arduino.h>

#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

#define MAX_LENGHT_MSG 250

typedef struct gpsData
{
  float latitude;
  float longitude;
  float speed_kph;
  float heading;
  float altitude;
} GPSData;

typedef struct message
{
  int index = 0;
  char number[15];
  char text[MAX_LENGHT_MSG];
  uint16_t *lenght;
} Message;

GPSData getGPSData();
void showGPSData(GPSData location);
void deleteAllSMS();
Message getSMS();

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Rastreador Veicular"));
  Serial.println(F("Iniciando... (Pode levar alguns segundos)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("SIM808 não localizado"));
  }
  Serial.println(F("SIM808 OK"));

  Serial.println(F("Testando GPS..."));
  fona.enableGPS(true);
  GPSData location = getGPSData();
  showGPSData(location);

  // Print SIM card IMEI number.
  char imei[16] = {0};
  int imeiLen = fona.getIMEI(imei);

  if (imeiLen > 0)
  {
    Serial.print("SIM card IMEI: ");
    Serial.println(imei);
  }

  deleteAllSMS();

  //set up the FONA to send a +CMTI notification when an SMS is received
  fonaSerial->print("AT+CNMI=2,1\r\n");

  Serial.println("Setup Finalizado");
}

void loop()
{
  char lat[12];
  char lon[12];
  GPSData location;
  Message msg = getSMS();

  if (msg.index > 0)
  {
    location = getGPSData();
    showGPSData(location);

    dtostrf(location.latitude, 8, 6, lat);
    dtostrf(location.longitude, 8, 6, lon);
    sprintf(msg.text, "Latitude : %s\nLongitude : %s\nhttp://maps.google.com/maps?q=%s,%s\n", lat, lon, lat, lon);

    Serial.print(msg.text);

    fona.sendSMS(msg.number, msg.text);
  }
  delay(2000);
  Serial.println(F("\nEsperando uma Mensagem!!\n"));
};

GPSData getGPSData()
{
  GPSData gpsData;
  bool gps_success = fona.getGPS(&gpsData.latitude, &gpsData.longitude, &gpsData.speed_kph, &gpsData.heading, &gpsData.altitude);

  while (!gps_success)
  {
    delay(5000);
    Serial.println("\nAguardando sinal de pelo menos 4 satélites...\n");
    gps_success = fona.getGPS(&gpsData.latitude, &gpsData.longitude, &gpsData.speed_kph, &gpsData.heading, &gpsData.altitude);
  }

  return gpsData;
}

void showGPSData(GPSData location)
{
  Serial.print("GPS lat:");
  Serial.println(location.latitude, 6);
  Serial.print("GPS long:");
  Serial.println(location.longitude, 6);
  Serial.print("GPS speed KPH:");
  Serial.println(location.speed_kph);
  Serial.print("GPS heading:");
  Serial.println(location.heading);
  Serial.print("GPS altitude:");
  Serial.println(location.altitude);
}

void deleteAllSMS()
{
  int indexSMS = fona.getNumSMS();

  for (int count = 1; count <= indexSMS; count++)
    fona.deleteSMS(count);
}

Message getSMS()
{
  Message recivedSMS;
  recivedSMS.index = fona.getNumSMS();
  if (recivedSMS.index > 0)
  {
    fona.getSMSSender(recivedSMS.index, recivedSMS.number, MAX_LENGHT_MSG);
    fona.readSMS(recivedSMS.index, recivedSMS.text, MAX_LENGHT_MSG, recivedSMS.lenght);
    fona.deleteSMS(recivedSMS.index);
  }
  return recivedSMS;
}