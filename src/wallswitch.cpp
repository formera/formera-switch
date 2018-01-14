#include "Arduino.h"
#include "wallswitch.h"
#include "pic.h"
#include <PubSubClient.h>
//#include <SoftwareSerial.h>

extern char myHostname[32];
//extern volatile bool silentMode;
extern bool forceAP;
extern PubSubClient client;
extern String mqttUser;
void deleteSettings();    //TODO: We need a class or h file for the generic functions
extern bool reboot;

#define WALL
#ifdef WALL
#define PIC_UART Serial
#else
#define PIC_UART softSerial
  SoftwareSerial softSerial(D7, D8); // RX, TX
#endif

Stream *picStream = &PIC_UART;

//const char WallSwitch::deviceType[] = "WallSwitchDual"; //cant override const like this in derived class

String WallSwitch::getDeviceType() {
  return F("WallSwitchDual");
}

void WallSwitch::setSwitchState(unsigned char id, unsigned char state) {
  DEBUG_PRINT_LN(F("WallSwitch:setSwitchState"));
  Pic.setSwitch(id+1, state);
}

void WallSwitch::setup(){
  DEBUG_PRINT_LN(F("WallSwitch:setup"));
  switchState[0]=0;
  switchState[1]=0;
  stateKnown = false;

  Pic.onHeartBeat([&]() {
    DEBUG_PRINT_LN(F("HeartBeatCallback"));
    lastHeartBeat = millis();
    heartBeatCount++;
  });

  Pic.onResetRequest([&]() {
    DEBUG_PRINT_LN(F("ResetRequestCallback"));
    lastHeartBeat = millis();
    heartBeatCount++;
    deleteSettings();
    reboot = true;
  });

  Pic.onSwitchUpdate([&](unsigned char id, unsigned char value) {
      stateKnown = true;
      DEBUG_PRINT_LN(F("SwitchUpdated"));
      unsigned char switchId = id-1;
      if(switchId == 0 || switchId == 1) {
        if(switchState[switchId] != value) {
          switchState[switchId]=value;
          DEBUG_PRINT_LN("New value set, mqtt publish");
          if(switchId == 0)
            client.publish(outTopic0.c_str(), String(value).c_str(), true);
          else
            client.publish(outTopic1.c_str(), String(value).c_str(), true);
        }
        else {
          DEBUG_PRINT_LN("No change");
        }
      }
      else {
        DEBUG_PRINT_LN("Ignored id " + String(switchId));
      }
    });

  delay(250);

  PIC_UART.begin(9600);
  Pic.setStream(picStream);

  //flush input
  while (PIC_UART.available()) {
      int data = PIC_UART.read();
  }
  Pic.clean_receive_buf();
  delay(250);

}

void WallSwitch::start(){
    DEBUG_PRINT_LN(F("WallSwitch:start"));

    outTopic0 = mqttUser + "/" + myHostname + "/switches/0";
    outTopic1 = mqttUser + "/" + myHostname + "/switches/1";
    controlTopic = mqttUser + "/" + myHostname + "/switches/+/set";

    DEBUG_PRINT_LN(F("Topics:"));
    DEBUG_PRINT_LN(outTopic0);
    DEBUG_PRINT_LN(outTopic1);
    DEBUG_PRINT_LN(controlTopic);
}

void WallSwitch::loop(){
  unsigned long now = millis();

  if (lastHeartBeat + 10000 < now )  {      //Send heartbeat request to pic every 10 seconds
    lastHeartBeat = now;
    DEBUG_PRINT_LN("Heartbeat timer");
    if(!stateKnown) {
      Pic.requestAll();
    }
    Pic.heartbeat();
  }


while (PIC_UART.available()) {
    int data = PIC_UART.read();
    Pic.uart_rx_handle(data);
    //Serial1.write(data);
    Pic.package_handle();
  }
}

void WallSwitch::prepareReset(){
}

void WallSwitch::onStateChange(thingState newState) {
  DEBUG_PRINT_LN(F("WallSwitch::onStateChange"));
  switch (newState) {
    case boot:
      DEBUG_PRINT_LN(F("State is boot"));
      Pic.setState(AP_STATE);                       //slow flash for boot
      break;
    case smartconfig:
      DEBUG_PRINT_LN(F("State is smartconfig"));
      Pic.setState(SMART_CONFIG_STATE);
      break;
    case apconfig:
      DEBUG_PRINT_LN(F("State is apconfig"));
      Pic.setState(SMART_CONFIG_STATE);             //fast flash for ap mode
      break;
    case offline:
      DEBUG_PRINT_LN(F("State is offline"));
      Pic.setState(AP_STATE);                       //slow flash for offline
      break;
    case online:
      DEBUG_PRINT_LN(F("State is online"));
      Pic.setState(WIFI_CONNECTED2);               // no flashing
      Pic.setSwitch(1, switchState[0]);         //Send stored switch state so led is correct
      Pic.setSwitch(2, switchState[1]);         //Send stored switch state so led is correct
      break;
    case cloudConnected:
      DEBUG_PRINT_LN(F("State is cloud connected"));
      break;
    default:
      DEBUG_PRINT_LN(F("State is unknown"));
      break;
  }

}


void WallSwitch::addToJSON(String& json, unsigned char id, unsigned char value) {
  json += "{";
  json += "\"id\":"+String(id);
  json += ", \"switch_type\":\"relay\"";
  json += ", \"state\":" + String(value);
  json += "}";
}

void WallSwitch::setupApiRoutes(ESP8266WebServer& server) {
  DEBUG_PRINT_LN(F("setupApiRoutes callback"));

  //TODO: Can this possiubly work?!

  //Sensor API
  server.on("/api/sensors", HTTP_GET, [&](){      //variable capture for lambda
    server.send(200, "application/json", "[]");
  });

  //Switches API
  server.on("/api/switches", HTTP_GET, [&](){
    String json = "[";
    addToJSON(json, 0, switchState[0]);
    json += ",";
    addToJSON(json, 1, switchState[1]);
    json += "]";
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/0", HTTP_GET, [&](){
    String json;
    addToJSON(json, 0, switchState[0]);
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/1", HTTP_GET, [&](){
    String json;
    addToJSON(json, 1, switchState[1]);
    server.send(200, "application/json", json);
    json = String();
  });


  server.on("/api/switches/0", HTTP_PUT, [&](){
    handlePutSwitch(server, 0);
  });

  server.on("/api/switches/1", HTTP_PUT, [&](){
    handlePutSwitch(server, 1);
  });

}

void WallSwitch::onMqttConnect() {
  DEBUG_PRINT_LN(F("MQTT Connect callback"));


  client.subscribe(controlTopic.c_str());
//  client.subscribe(inTopic[1].c_str());
  client.publish(outTopic0.c_str(), String(switchState[0]).c_str(), true);   //TODO: Start moving away from using String class
  client.publish(outTopic1.c_str(), String(switchState[1]).c_str(), true);   //TODO: Start moving away from using String class
}

void WallSwitch::onMqttDisconnect() {
  DEBUG_PRINT_LN(F("MQTT Disconnect callback"));
}



void WallSwitch::onMqttReceiveMessage(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT(F("MQTT Message arrived at ["));
  DEBUG_PRINT(topic);
  DEBUG_PRINT(F("] "));

  String top = String(topic);

  for (int i = 0; i < length; i++) {
    DEBUG_PRINT((char)payload[i]);
  }
  DEBUG_PRINT_LN();

  unsigned char picId = 0;
  char id = top[top.length()-5];
  if(id == '0') {
    picId = 1;
  }
  else if(id == '1') {
    picId = 2;
  }
  else {
    DEBUG_PRINT_LN(F("Unkown id"));
  }


  //TODO: Get id from topic!
  if(picId>0) {
    if ((char)payload[0] == '1') {
      Pic.setSwitch(picId, 1);
    }
    else {
      Pic.setSwitch(picId, 0);
    }
  }
}

void WallSwitch::handlePutSwitch(ESP8266WebServer& server, unsigned char id) {
  DEBUG_PRINT_LN(F("Api: Put switch"));
  String body = server.arg("plain");
  DEBUG_PRINT(F("request body: "));
  DEBUG_PRINT_LN(body);
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& settings = jsonBuffer.parseObject(body);
  if (!settings.success()) {
      DEBUG_PRINT_LN(F("Failed to parse JSON"));
      server.send(400, "text/plain", "bad request");
  }
  else {
      DEBUG_PRINT_LN(F("Updating switch"));
      //unsigned char id = 0; //TODO: Fix
      if (settings.containsKey("state")) {
          int state = settings["state"];
          Pic.setSwitch(id+1, state);

          DEBUG_PRINT(F("switch.state <- "));
          DEBUG_PRINT_LN(state );
      }
      server.send(200);
  }
}
