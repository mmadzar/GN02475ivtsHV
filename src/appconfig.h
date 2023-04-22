#ifndef APPCONFIG_H_
#define APPCONFIG_H_

#define HOST_NAME "GN02475ivtsHV" // same code for all bms instances ivtsHV as 01, 02, 03, 04
                                  // but this one has shunt CAN connected and acts as master

#include "../../secrets.h"
#include <stdint.h>
#include <Arduino.h>
#include <driver/gpio.h>
#include "shared/configtypes/configtypes.h"

// IVTS shunt monitors
#define CURRENT "current"
#define VOLTAGE "voltage"
#define VOLTAGE2 "voltage2"
#define TEMPERATURE "temperature"
#define POWER "power"
#define POWER10 "power10" // 10 times in a second
#define CURRENTCOUNTER "currentCounter"
#define ENERGYCOUNTER "energyCounter"

// mosfets
#define MSFT1 "msft1"
#define MSFT2 "msft2"

struct SerialConfig
{
  const char *portName;
  const unsigned long baudRate;
  const uint8_t rx;
  const uint8_t tx;
  const bool invert;
};

struct Settings
{
#define ListenChannelsCount 0
  const char *listenChannels[ListenChannelsCount] = {};

  const gpio_num_t led = (gpio_num_t)2;      // status led
  const gpio_num_t can0_rx = (gpio_num_t)32; // rev1 16; can0 transciever rx line
  const gpio_num_t can0_tx = (gpio_num_t)22; // rev1 17; can0 transciever tx line

  CollectorConfig colBms[5] = {
      {"ProtStatus", 500}, // protection status - error if >0
      {"Temp1", 2000},
      {"Temp2", 2000},
      {"Temp3", 2000},
      {"Temp4", 2000}};
  CollectorConfig colBmsCell[24] = {
      {"Cell01", 5000},
      {"Cell02", 5000},
      {"Cell03", 5000},
      {"Cell04", 5000},
      {"Cell05", 5000},
      {"Cell06", 5000},
      {"Cell07", 5000},
      {"Cell08", 5000},
      {"Cell09", 5000},
      {"Cell10", 5000},
      {"Cell11", 5000},
      {"Cell12", 5000},
      {"Cell13", 5000},
      {"Cell14", 5000},
      {"Cell15", 5000},
      {"Cell16", 5000},
      {"Cell17", 5000},
      {"Cell18", 5000},
      {"Cell19", 5000},
      {"Cell20", 5000},
      {"Cell21", 5000},
      {"Cell22", 5000},
      {"Cell23", 5000},
      {"Cell24", 5000}};

#define CollectorCount 8
  CollectorConfig collectors[CollectorCount] = {
      {CURRENT, 0},        // mA
      {VOLTAGE, 0},        // mV
      {VOLTAGE2, 0},       // mV
      {TEMPERATURE, 0},    // 0.1 C degrees - Shunt temperature C degrees
      {POWER, 0},          // 1W
      {POWER10, 100},        // 1W
      {CURRENTCOUNTER, 0}, // 1As current counter
      {ENERGYCOUNTER, 0}}; // 1Wh energy counter

  int getCollectorIndex(const char *name)
  {
    for (size_t i = 0; i < CollectorCount; i++)
    {
      if (strcmp(collectors[i].name, name) == 0)
        return i;
    }
    return -1;
  }

#define SwitchCount 2
  SwitchConfig switches[SwitchCount] = {
      {devicet::msft_01_pwm, MSFT1, 21, switcht::on_off}, // mosfet gate 1
      {devicet::msft_02_pwm, MSFT2, 17, switcht::on_off}  // mosfet gate 2
  };

  int getSwitchIndex(devicet device)
  {
    for (size_t i = 0; i < SwitchCount; i++)
    {
      if (switches[i].device == device)
        return i;
    }
  }
};

struct Intervals
{
  int statusPublish = 1000;   // interval at which status is published to MQTT
  int Can2Mqtt = 1000;        // send CAN messages to MQTT every n secconds. Accumulate messages until. Set this to 0 for forwarding all CAN messages to MQTT as they are received.
  int CANsend = 10;           // interval at which to send CAN Messages to car bus network (milliseconds)
  int click_onceDelay = 1000; // milliseconds
  int serialInterval = 30000; // milliseconds - goes fom 250 when battery in use to 10 000 when not in use, configured over MQTT interval command
};

extern Settings settings;
extern Intervals intervals;

#endif /* APPCONFIG_H_ */
