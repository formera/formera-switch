
#ifndef THING_H
#define THING_H

#include "Arduino.h"

enum thingState {   boot = 0x00,
                    smartconfig,
                    apconfig,
                    offline,
                    online,
                    cloudConnected
                };

//abstract class Thing
class Thing {
    public:
        //initialize the thing
        virtual void start();
        virtual void setup();
        virtual void loop();

        virtual void setSwitchState(unsigned char id, unsigned char state);

        virtual void prepareReset();                                  //Any device specif things to do before esp8266 reset

        virtual void setupApiRoutes(ESP8266WebServer& server);        //Setup specific routes

        virtual void onStateChange(thingState newState);


        virtual void onMqttConnect();
        virtual void onMqttDisconnect();
        virtual void onMqttReceiveMessage(char* topic, byte* payload, unsigned int length);

          virtual String getDeviceType();

};
#endif
