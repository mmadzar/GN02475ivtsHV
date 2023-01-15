#ifndef MQTTMESSAGEHANDLER_H_
#define MQTTMESSAGEHANDLER_H_

#include "Arduino.h"
#include "appconfig.h"
#include "status.h"

class MqttMessageHandler
{

public:
    MqttMessageHandler();
    void handle();
    void static HandleMessage(const char *topic, const char *message, int length);
    static void callback(char *topic, byte *message, unsigned int length);
};

#endif /* MQTTMESSAGEHANDLER_H_ */
