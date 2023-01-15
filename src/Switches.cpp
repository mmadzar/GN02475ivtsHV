#include "Switches.h"

MqttPubSub *mqttClientSwitches;
Settings settings;

Switches::Switches()
{
}

void Switches::setup(MqttPubSub &mqtt_client)
{
  Serial.println("setup switches");
  mqttClientSwitches = &mqtt_client;
  for (size_t i = 0; i < SwitchCount; i++)
  {
    SwitchConfig *sc = &settings.switches[i];
    configs[i] = new SwitchConfig(sc->device, sc->name, sc->pin, sc->channel, sc->switchtype);
    devices[i] = new Switch(i, *configs[i]);
    devices[i]->onChange([](const char *name, devicet devicetype, int value)
                         { mqttClientSwitches->sendMessage(String(value), String(wifiSettings.hostname) + "/out/switches/" + name); });
    devices[i]->setup();
  }
}

void Switches::handle()
{
  for (size_t i = 0; i < SwitchCount; i++)
  {
    devices[i]->handle();
  }
}