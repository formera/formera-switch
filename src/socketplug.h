#ifndef SOCKETPLUG_H
#define SOCKETPLUG_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "thing.h"
#include "Arduino.h"
#include "switch.h"
#include "debug.h"

#define PLUG

//TODO: Make enum and move to corrent file
const int NoSensor = 0;
const int DS18B20 = 1;
const int DHT22 = 2;

#ifdef PLUG  // Use this for the Formera Plug.
  //Formera SocketPlug
  const int REDLED = 13;
  const int BLUELED = 3;
  const int BUTTON = 1;
  const int RELAY = 14;
#else
  //nodeMCU devkit board
  const int REDLED = 5; //D1;
  const int BLUELED = 16; //D0;
  const int BUTTON = 4; //D2;
  const int RELAY = 14; //D5;

#endif

class SocketPlug : public Thing {
    public:
      //Thing interface
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
      virtual String getDeviceType();
      //Socket plug
      void handlePutSwitch(ESP8266WebServer& server);
    private:
      bool silentMode;

};


#endif
