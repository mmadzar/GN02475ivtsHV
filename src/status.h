#ifndef STATUS_H_
#define STATUS_H_

#include "appconfig.h"
#include "shared/status_base.h"
#include "shared/base/Collector.h"

struct Status : public StatusBase
{
  Collector *colBms[5];
  Collector *colBmsCell[24];

  String ivtsCommand = "";
  String bmsCommand = "";
  bool running = false;
  int collectors[CollectorCount];
  int switches[SwitchCount]{-1, -1};

  // BMS
  byte serialBytes[2048];
  int serialBytesSize;
  bool monitorStarted = true;

#define BMScollectorCount 29 // 24 cells + 4 temps + protection_status
  int BMScollectors[BMScollectorCount];

  void initBMScollectors()
  {
  }

  int bmsCollectors[BMScollectorCount];

  JsonObject GenerateJson()
  {

    JsonObject root = this->PrepareRoot();

    root["running"] = running;

    JsonObject jcollectors = root.createNestedObject("collectors");
    for (size_t i = 0; i < CollectorCount; i++)
      jcollectors[settings.collectors[i].name] = collectors[i];

    JsonObject jswitches = root.createNestedObject("switches");
    for (size_t i = 0; i < SwitchCount; i++)
      jswitches[settings.switches[i].name] = switches[i];

    return root;
  }
};

extern Status status;

#endif /* STATUS_H_ */
