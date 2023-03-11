#include "CanBus.h"

StaticJsonDocument<512> docJ;
char tempBufferCan[512];
CAN_FRAME frames[3];
long displayValue = 0;
int consumptionCounter = 0; // 0 - 65535(0xFFFF)
MqttPubSub *mqttClientCan;
Settings settingsCollectors;

CanBus::CanBus()
{
  init();
}

void CanBus::init()
{
  CAN0.setCANPins(settings.can0_rx, settings.can0_tx);
}

void CanBus::setup(class MqttPubSub &mqtt_client, Bytes2WiFi &wifiport, Bytes2WiFi &portDebug)
{
  mqttClientCan = &mqtt_client;
  b2w = &wifiport;
  b2wdebug = &portDebug;

  CAN0.begin(500000);
  CAN0.watchFor();

  for (size_t i = 0; i < CollectorCount; i++)
  {
    CollectorConfig *sc = &settings.collectors[i];
    configs[i] = new CollectorConfig(sc->name, sc->sendRate);
    collectors[i] = new Collector(*configs[i]);
    collectors[i]->onChange([](const char *name, int value, int min, int max, int samplesCollected, uint64_t timestamp)
                            { 
                              status.collectors[settingsCollectors.getCollectorIndex(name)]=value;
                              JsonObject root = docJ.to<JsonObject>();
                              root["value"]=value;
                              root["min"]=min;
                              root["max"]=max;
                              root["timestamp"]=timestamp;
                              root["samples"]=samplesCollected;

                              serializeJson(docJ, tempBufferCan);
                              mqttClientCan->sendMessageToTopic(String(wifiSettings.hostname) + "/out/collectors/" + name, tempBufferCan);
                            });
    collectors[i]->setup();
  }
}

void CanBus::handle()
{
  if (status.ivtsCommand.length() > 2)
  {
    Serial.printf("Sending %s...\n", status.ivtsCommand);
    if (status.ivtsCommand.startsWith("setupGN02475"))
      setupGN02475();
    if (status.ivtsCommand.startsWith("initialize"))
      initializeIVTS();
    else if (status.ivtsCommand.startsWith("restart"))
      restartIVTS();
    else if (status.ivtsCommand.startsWith("default"))
      defaultIVTS();
    else if (status.ivtsCommand.startsWith("initcurrent"))
      initCurrentIVTS();
    else if (status.ivtsCommand.startsWith("stop"))
      stopIVTS();
    else if (status.ivtsCommand.startsWith("start"))
      startIVTS();
    else if (status.ivtsCommand.startsWith("findCmd"))
    {
      pos = 0;
      findCmd();
    }
    // reset executed command
    status.ivtsCommand = "";
  }

  // execute binary can message
  if (b2w->wifiCmdPos > 5)
  {
    CAN_FRAME(newFrame);
    newFrame.id = cmdId;
    newFrame.extended = 0;
    newFrame.rtr = 0;
    newFrame.length = b2w->wifiCommand[1];
    for (size_t i = 0; i < b2w->wifiCmdPos - 2; i++)
      newFrame.data.bytes[i] = b2w->wifiCommand[i + 1];
    CAN0.sendFrame(newFrame);
    b2wdebug->addBuffer("Message sent!", 13);
    b2wdebug->send();
  }
  b2w->wifiCmdPos = 0;

  CAN_FRAME frame;
  if (CAN0.read(frame))
  {
    lastMsgRcv = status.currentMillis;
    handleFrame(frame);
   
    // store message to buffer
    b2w->addBuffer(0xf1);
    b2w->addBuffer(0x00); // 0 = canbus frame sending
    uint32_t now = micros();
    b2w->addBuffer(now & 0xFF);
    b2w->addBuffer(now >> 8);
    b2w->addBuffer(now >> 16);
    b2w->addBuffer(now >> 24);
    b2w->addBuffer(frame.id & 0xFF);
    b2w->addBuffer(frame.id >> 8);
    b2w->addBuffer(frame.id >> 16);
    b2w->addBuffer(frame.id >> 24);
    b2w->addBuffer(frame.length + (uint8_t)(((int)0) << 4)); // 2 ibus address
    for (int c = 0; c < frame.length; c++)
      b2w->addBuffer(frame.data.uint8[c]);
    b2w->addBuffer(0x0a); // new line for SavvyCan and serial monitor
  }
  if (status.currentMillis - lastSentCanLog >= 100)
  {
    lastSentCanLog = status.currentMillis;
    b2w->send();
  }
}

void getTimestamp(char *buffer)
{
  if (strcmp(status.SSID, "") == 0 || !getLocalTime(&(status.timeinfo), 10))
    sprintf(buffer, "INVALID TIME               ");
  else
  {
    long microsec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    microsec = tv.tv_usec;
    strftime(buffer, 29, "%Y-%m-%d %H:%M:%S", &(status.timeinfo));
    sprintf(buffer, "%s.%06d", buffer, microsec);
  }
}

long CanBus::handleFrame(CAN_FRAME frame)
{
  if (frame.id >= 0x521 && frame.id <= 0x528)
  {
    const long v = (long)((frame.data.bytes[2] << 24) | (frame.data.bytes[3] << 16) | (frame.data.bytes[4] << 8) | (frame.data.bytes[5]));
    uint64_t ts = status.getTimestampMicro();
    switch (frame.id)
    {
    case 0x521: // mA
      collectors[settingsCollectors.getCollectorIndex(CURRENT)]->handle((int)v, ts);
      break;
    case 0x522: // mV - U1 - 0x523, 0x524 for U2 and U3
      collectors[settingsCollectors.getCollectorIndex(VOLTAGE)]->handle((int)v, ts);
      break;
    case 0x523: // mV - U2
      collectors[settingsCollectors.getCollectorIndex(VOLTAGE2)]->handle((int)v, ts);
      break;
    case 0x525: // 0.1 C degrees
      collectors[settingsCollectors.getCollectorIndex(TEMPERATURE)]->handle((int)v, ts);
      break;
    case 0x526: // 1W
      collectors[settingsCollectors.getCollectorIndex(POWER)]->handle((int)v, ts);
      collectors[settingsCollectors.getCollectorIndex(POWER10)]->handle((int)v, ts);
      break;
    case 0x527: // 1As - Ampere hour counter As
      collectors[settingsCollectors.getCollectorIndex(CURRENTCOUNTER)]->handle((int)v, ts);
      break;
    case 0x528: // 1Wh
      collectors[settingsCollectors.getCollectorIndex(ENERGYCOUNTER)]->handle((int)v, ts);
      break;
    default:
      break;
    }
    return v;
  }
  return 0;
}

void CanBus::setupGN02475()
{
  b2wdebug->addBuffer("Setup GN02475...\r\n", 18);
  b2wdebug->handle();
  stopIVTS();
  delay(700);
  // 0 - I - cyclic default 20ms
  // 1 - U1 -  cyclic default 60ms
  // 2 - U2 - off 60ms
  // 3 - U3 - off 60ms
  // 4 - T - cyclic 100ms
  // 5 - W - cyclic 30ms
  // 6 - As - cyclic 30ms
  // 7 - Wh - cyclic 30ms
  for (size_t i = 2; i < 8; i++)
  {
    outframe.id = cmdId;   // Set our transmission address ID
    outframe.length = 8;   // Data payload 8 bytes
    outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
    outframe.rtr = 0;      // No request
    outframe.data.bytes[0] = (0x20 + i);
    switch (i)
    {
    case 2: //monitor voltage 2 for HV shunt
      outframe.data.bytes[1] = 0x22; // all off
      outframe.data.bytes[2] = 0x00; // interval
      outframe.data.bytes[3] = 0x3c; // interval
      break;
    case 3:
      outframe.data.bytes[1] = 0x00; // all off
      outframe.data.bytes[2] = 0x00; // interval
      outframe.data.bytes[3] = 0x3c; // interval
      break;
    case 4:
      outframe.data.bytes[1] = 0x22; // cyclic with errors
      outframe.data.bytes[2] = 0x00; // interval
      outframe.data.bytes[3] = 0x64; // interval
      break;
    case 5:
    case 6:
    case 7:
      outframe.data.bytes[1] = 0x22; // cyclic with errors
      outframe.data.bytes[2] = 0x00; // interval
      outframe.data.bytes[3] = 0x1E; // interval
      break;
    default:
      break;
    }

    outframe.data.bytes[4] = 0x00;
    outframe.data.bytes[5] = 0x00;
    outframe.data.bytes[6] = 0x00;
    outframe.data.bytes[7] = 0x00;
    CAN0.sendFrame(outframe);
    delay(500);

    storeIVTS();
    delay(500);
  }
  startIVTS();
  delay(500);
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

// IVTS related
void CanBus::initializeIVTS()
{
  b2wdebug->addBuffer("Init...\r\n", 9);
  b2wdebug->handle();
  // TODO firstframe = false;
  stopIVTS();
  delay(700);
  for (int i = 0; i < 9; i++)
  {
    outframe.id = cmdId;   // Set our transmission address ID
    outframe.length = 8;   // Data payload 8 bytes
    outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
    outframe.rtr = 0;      // No request
    outframe.data.bytes[0] = (0x20 + i);
    outframe.data.bytes[1] = 0x42;
    outframe.data.bytes[2] = 0x02;
    outframe.data.bytes[3] = (0x60 + (i * 18));
    outframe.data.bytes[4] = 0x00;
    outframe.data.bytes[5] = 0x00;
    outframe.data.bytes[6] = 0x00;
    outframe.data.bytes[7] = 0x00;
    CAN0.sendFrame(outframe);
    delay(500);

    storeIVTS();
    delay(500);
  }
  startIVTS();
  delay(500);
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::findCmd()
{
  if (pos != -1 && status.currentMillis - lastCmd > 10)
  {
    if (lastMsgRcv == 0 && pos > 0)
    {
      String msg("Found ");
      msg = msg + String(pos - 1) + ".\n";
      b2wdebug->addBuffer(msg.c_str(), msg.length());
      b2wdebug->send();
      pos = -1;
    }
    else if (pos <= 0x7ff)
    {
      pos++;
      if (pos % 100 == 0)
      {
        String stat("Working ");
        stat = stat + String(pos) + "...\n";
        b2wdebug->addBuffer(stat.c_str(), stat.length());
        b2wdebug->send();
      }
      stopIVTS(pos);
      lastCmd = status.currentMillis;
      lastMsgRcv = 0;
    }
  }
}

void CanBus::stopIVTS(uint32_t id)
{
  // b2wdebug->addBuffer("Stop...\r\n", 9);
  // b2wdebug->send();
  // SEND STOP///////
  outframe.id = id;          // Set our transmission address ID
  outframe.length = 8;       // Data payload 8 bytes
  outframe.extended = false; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;          // No request
  outframe.data.uint8[0] = 0x34;
  outframe.data.uint8[1] = 0x00;
  outframe.data.uint8[2] = 0x01;
  outframe.data.uint8[3] = 0x00;
  outframe.data.uint8[4] = 0x00;
  outframe.data.uint8[5] = 0x00;
  outframe.data.uint8[6] = 0x00;
  outframe.data.uint8[7] = 0x00;
  CAN0.sendFrame(outframe);
  // b2wdebug->addBuffer("Done...\r\n", 9);
  // b2wdebug->send();
}

void CanBus::stopIVTS()
{
  b2wdebug->addBuffer("Stop...\r\n", 9);
  b2wdebug->send();
  // SEND STOP///////
  outframe.id = cmdId;       // Set our transmission address ID
  outframe.length = 8;       // Data payload 8 bytes
  outframe.extended = false; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;          // No request
  outframe.data.uint8[0] = 0x34;
  outframe.data.uint8[1] = 0x00;
  outframe.data.uint8[2] = 0x01;
  outframe.data.uint8[3] = 0x00;
  outframe.data.uint8[4] = 0x00;
  outframe.data.uint8[5] = 0x00;
  outframe.data.uint8[6] = 0x00;
  outframe.data.uint8[7] = 0x00;
  CAN0.sendFrame(outframe);
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::startIVTS()
{
  b2wdebug->addBuffer("Start...\r\n", 10);
  b2wdebug->send();
  // SEND START///////
  outframe.id = cmdId;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;      // No request
  outframe.data.bytes[0] = 0x34;
  outframe.data.bytes[1] = 0x01;
  outframe.data.bytes[2] = 0x01;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
  b2wdebug->addBuffer("Done...\r\n", 9);
  b2wdebug->send();
}

void CanBus::storeIVTS()
{
  // SEND STORE///////
  outframe.id = cmdId;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;      // No request
  outframe.data.bytes[0] = 0x32;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::restartIVTS()
{
  Serial.println("Restarting IVTS...");
  // Has the effect of zeroing AH and KWH
  outframe.id = cmdId;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;      // No request
  outframe.data.bytes[0] = 0x3F;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
}

void CanBus::defaultIVTS()
{
  stopIVTS();
  delay(500);
  // Returns module to original defaults
  outframe.id = cmdId;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;      // No request
  outframe.data.bytes[0] = 0x3D;
  outframe.data.bytes[1] = 0x00;
  outframe.data.bytes[2] = 0x00;
  outframe.data.bytes[3] = 0x00;
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;
  CAN0.sendFrame(outframe);
  delay(500);
  startIVTS();
}

void CanBus::initCurrentIVTS()
{
  stopIVTS();
  delay(500);
  outframe.id = cmdId;   // Set our transmission address ID
  outframe.length = 8;   // Data payload 8 bytes
  outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
  outframe.rtr = 0;      // No request
  outframe.data.bytes[0] = (0x21);
  outframe.data.bytes[1] = 0x42;
  outframe.data.bytes[2] = 0x01;
  outframe.data.bytes[3] = (0x61);
  outframe.data.bytes[4] = 0x00;
  outframe.data.bytes[5] = 0x00;
  outframe.data.bytes[6] = 0x00;
  outframe.data.bytes[7] = 0x00;

  CAN0.sendFrame(outframe);
  storeIVTS();
  delay(500);

  startIVTS();
  delay(500);
}