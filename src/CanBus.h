#ifndef CANBUS_H_
#define CANBUS_H_

#include <Arduino.h>
#include "shared/base/Collector.h"
#include "shared/configtypes/configtypes.h"
#include <esp32_can.h>
#include <CAN_config.h>
#include "appconfig.h"
#include <ArduinoJson.h>
#include "status.h"
#include "shared/MqttPubSub.h"
#include "shared/Bytes2WiFi.h"

class CanBus
{
private:
  Collector *collectors[CollectorCount];
  CollectorConfig *configs[CollectorCount];
  Bytes2WiFi *b2w;
  Bytes2WiFi *b2wdebug;

  int cmdId = 0x411; // default 0x411

  Settings settings;
  void init();
  CAN_device_t CAN_cfg;    // CAN Config
  long previousMillis = 0; // last time a CAN Message was send
  long handleFrame(CAN_FRAME frame);

  // find vars
  long pos = 0;
  long lastCmd = 0;
  long lastMsgRcv = 0;

public:
  CanBus();
  void handle();
  void setup(class MqttPubSub &mqtt_client, Bytes2WiFi &wifiport, Bytes2WiFi &portDebug);
  void findCmd();
  void setupGN02475();
  void initializeIVTS();
  void stopIVTS();
  void stopIVTS(uint32_t id);
  void storeIVTS();
  void startIVTS();
  void initCurrentIVTS();
  void defaultIVTS();
  void restartIVTS();
  CAN_FRAME outframe;
};

#endif /* CANBUS_H_ */
