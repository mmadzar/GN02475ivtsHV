#include <Arduino.h>
#include "appconfig.h"
#include "status.h"
#include "shared/WiFiOTA.h"
#include "MqttMessageHandler.h"
#include <ArduinoOTA.h>
#include <string.h>
#include "shared/MqttPubSub.h"
#include "shared/Bytes2WiFi.h"
#include "Switches.h"
#include "CanBus.h"

Status status;
Intervals intervals;
WiFiSettings wifiSettings;
WiFiOTA wota;
MqttPubSub mqtt;
Switches pwmCtrl;
Bytes2WiFi bytesWiFi;
Bytes2WiFi debugWiFi;

CanBus can;
uint32_t lastBytesSent = 0; // millis when last packet is sent

long loops = 0;
long lastLoopReport = 0;

bool firstRun = true;

long lastVacuumReadTime = 0;
int lastVacuumRead = 0;

void setup()
{
  SETTINGS.loadSettings();
  pwmCtrl.setup(mqtt);
  Serial.begin(115200);
  Serial.println("Serial started!");
  pinMode(settings.led, OUTPUT);

  wota.setupWiFi();
  wota.setupOTA();
  mqtt.setup();
  bytesWiFi.setup(23);
  debugWiFi.setup(24);
  can.setup(mqtt, bytesWiFi, debugWiFi);
}

void loop()
{
  status.currentMillis = millis();
  if (status.currentMillis - lastLoopReport > 1000) // number of loops in 1 second - for performance measurement
  {
    lastLoopReport = status.currentMillis;
    Serial.printf("Loops in a second: %u\n", loops);
    status.loops = loops;
    loops = 0;
  }
  loops++;

  can.handle();
  pwmCtrl.handle();

  wota.handleWiFi();
  wota.handleOTA();
  if (loops % 10 == 0) // check mqtt every 10th cycle
    mqtt.handle();

  bytesWiFi.handle();
  debugWiFi.handle();

  mqtt.publishStatus(!firstRun);
  firstRun = false;
}
