#include "MqttMessageHandler.h"

MqttMessageHandler::MqttMessageHandler()
{
}

void MqttMessageHandler::HandleMessage(const char *command, const char *message, int length)
{
  if (strcmp(command, "ivts") == 0)
    status.ivtsCommand = message;
  else if (strcmp(command, "start") == 0)
    status.running = true;
  else if (strcmp(command, "stop") == 0)
    status.running = false;
  else if (strcmp(command, "interval") == 0)
    intervals.serialInterval = String(message).toInt();
  Serial.println(intervals.serialInterval);
}

void MqttMessageHandler::callback(char *topic, byte *message, unsigned int length)
{
}

void MqttMessageHandler::handle()
{
}