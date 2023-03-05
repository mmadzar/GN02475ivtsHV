#include "MqttMessageHandler.h"

MqttMessageHandler::MqttMessageHandler()
{
}

void MqttMessageHandler::HandleMessage(const char *command, const char *message, int length)
{
  if (strcmp(command, "ivts") == 0)
    status.ivtsCommand = message;
  else
  {
    for (size_t i = 0; i < SwitchCount; i++)
    {
      SwitchConfig *sc = &settings.switches[i];
      // find switch in settings and set status value by index
      if (strcmp(sc->name, command) == 0)
      {
        status.switches[i] = String(message).toInt();
        break;
      }
    }
  }
}

void MqttMessageHandler::callback(char *topic, byte *message, unsigned int length)
{
}

void MqttMessageHandler::handle()
{
}