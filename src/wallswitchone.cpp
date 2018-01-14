#include "Arduino.h"
#include "wallswitchone.h"
#include "pic.h"
#include <PubSubClient.h>

extern char myHostname[32];
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

//TODO: this is renamed because platformio links all device types...
Stream *picStreamOne = &PIC_UART;

String WallSwitchOne::getDeviceType() {
  return F("WallSwitchSingle");
}

void WallSwitchOne::setSwitchState(unsigned char id, unsigned char state) {
  DEBUG_PRINT_LN(F("WallSwitch:setSwitchState"));
  Pic.setSwitch(id+1, state);
}

void WallSwitchOne::setup(){
  DEBUG_PRINT_LN(F("WallSwitch:setup"));
  switchState=0;
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
      if(switchId == 0) {
        if(switchState != value) {
          switchState=value;
          DEBUG_PRINT_LN("New value set, mqtt publish");
          client.publish(outTopic0.c_str(), String(value).c_str(), true);
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
  Pic.setStream(picStreamOne);

  //flush input
  while (PIC_UART.available()) {
      int data = PIC_UART.read();
  }
  Pic.clean_receive_buf();
  delay(250);

}

void WallSwitchOne::start(){
    DEBUG_PRINT_LN(F("WallSwitch:start"));

    outTopic0 = mqttUser + "/" + myHostname + "/switches/0";
    controlTopic = mqttUser + "/" + myHostname + "/switches/+/set";

    DEBUG_PRINT_LN(F("Topics:"));
    DEBUG_PRINT_LN(outTopic0);
    DEBUG_PRINT_LN(controlTopic);
}



void WallSwitchOne::loop(){
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

void WallSwitchOne::prepareReset(){
}

void WallSwitchOne::onStateChange(thingState newState) {
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
      Pic.setSwitch(1, switchState);         //Send stored switch state so led is correct
      break;
    case cloudConnected:
      DEBUG_PRINT_LN(F("State is cloud connected"));
      break;
    default:
      DEBUG_PRINT_LN(F("State is unknown"));
      break;
  }

}


void WallSwitchOne::addToJSON(String& json, unsigned char id, unsigned char value) {
  json += "{";
  json += "\"id\":"+String(id);
  json += ", \"switch_type\":\"relay\"";
  json += ", \"state\":" + String(value);
  json += "}";
}

void WallSwitchOne::setupApiRoutes(ESP8266WebServer& server) {
  DEBUG_PRINT_LN(F("setupApiRoutes callback"));

  //Sensor API
  server.on("/api/sensors", HTTP_GET, [&](){      //variable capture for lambda
    server.send(200, "application/json", "[]");
  });

  //Switches API
  server.on("/api/switches", HTTP_GET, [&](){
    String json = "[";
    addToJSON(json, 0, switchState);
    json += "]";
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/0", HTTP_GET, [&](){
    String json;
    addToJSON(json, 0, switchState);
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/0", HTTP_PUT, [&](){
    handlePutSwitch(server, 0);
  });

}

void WallSwitchOne::onMqttConnect() {
  DEBUG_PRINT_LN(F("MQTT Connect callback"));


  client.subscribe(controlTopic.c_str());
//  client.subscribe(inTopic[1].c_str());
  client.publish(outTopic0.c_str(), String(switchState).c_str(), true);   //TODO: Start moving away from using String class
}

void WallSwitchOne::onMqttDisconnect() {
  DEBUG_PRINT_LN(F("MQTT Disconnect callback"));
}

void WallSwitchOne::onMqttReceiveMessage(char* topic, byte* payload, unsigned int length) {
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


void WallSwitchOne::handlePutSwitch(ESP8266WebServer& server, unsigned char id) {
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
      if (settings.containsKey("state")) {
          int state = settings["state"];
          Pic.setSwitch(id+1, state);

          DEBUG_PRINT(F("switch.state <- "));
          DEBUG_PRINT_LN(state );
      }
      server.send(200);
  }
}
