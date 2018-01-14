#ifndef WALLSWITCH_H
#define WALLSWITCH_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "Arduino.h"
#include "debug.h"
#include "thing.h"

//TODO: Make enum and move to corrent file
const int NoSensor = 0;
const int DS18B20 = 1;
const int DHT22 = 2;

class WallSwitch : public Thing {
    public:
      virtual void start();
      virtual void setup();
      virtual void loop();
      virtual void prepareReset();
      virtual void setSwitchState(unsigned char id, unsigned char state);
      virtual void onStateChange(thingState newState);

      virtual void setupApiRoutes(ESP8266WebServer& server);

      virtual void onMqttConnect();
      virtual void onMqttDisconnect();
      virtual void onMqttReceiveMessage(char* topic, byte* payload, unsigned int length);

      void handlePutSwitch(ESP8266WebServer& server, unsigned char id);
      virtual String getDeviceType();

    private:
      void addToJSON(String& json, unsigned char id, unsigned char value);
      unsigned long lastHeartBeat = 0;
      unsigned long heartBeatCount = 0;
      unsigned char switchState[2];
      bool stateKnown;

      //Topics for switch control over mqtt
      String outTopic0;
      String outTopic1;
      String controlTopic;
};


#endif
