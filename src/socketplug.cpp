#include "socketplug.h"
#include "Arduino.h"
#include <Ticker.h>
#include <PubSubClient.h>
#include "sensor.h"
#include "DHT.h"
#include <OneWire.h>

extern char myHostname[32];
extern volatile bool silentMode;
extern bool forceAP;
extern PubSubClient client;
extern String mqttUser;

extern unsigned long sensorLastWriteTime;

extern int sensorType[4];
extern float sensorValues[4][2];
extern bool sensorValueValid[4];
extern bool reboot;

void deleteSettings();    //TODO: We need a class or h file for the generic functions

int lastRelayState;
Ticker blueLedBlink;
bool blueIsFlashing = false;
const int apModeFlashTime = 250;
const int notConnectedFlashTime = 1000;
Switch switch0(0, relay, RELAY);

//Topics for switch control over mqtt
String outTopic;
String inTopic;

bool getDS18B20Temperature(float& celsius, int pin)
{
  OneWire DS18B20(pin);
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];

  if ( !DS18B20.search(addr)) {
    DS18B20.reset_search();
    delay(250);
    DEBUG_PRINT(F("Error: "));
    DEBUG_PRINT_LN("DS18B20 - Cannot read Temperature from sensor. - Pin " + pin);
    celsius = -999;
    return false;
  }

  DS18B20.reset();
  DS18B20.select(addr);
  DS18B20.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = DS18B20.reset();
  DS18B20.select(addr);
  DS18B20.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = DS18B20.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  DS18B20.reset();

  DEBUG_PRINT(F("OneWire Temperature: "));
  DEBUG_PRINT_LN(celsius);
  DEBUG_PRINT(F("OneWire Pin: "));
  DEBUG_PRINT_LN(pin);

  return true;
}

bool getDHT22HumidityTemperature(int pin, float& temp, float& hum)
{

  DHT dht;
  dht.setup(pin);
  delay(dht.getMinimumSamplingPeriod());

  String dht_status = dht.getStatusString();
  hum = dht.getHumidity();
  temp = dht.getTemperature();

  DEBUG_PRINT(F("DHT Status: "));
  DEBUG_PRINT_LN(dht_status);
  if(dht_status == "TIMEOUT") {
    return false;
  }
  DEBUG_PRINT(F("Humidity: "));
  DEBUG_PRINT_LN(hum);
  DEBUG_PRINT(F("Temperature: "));
  DEBUG_PRINT_LN(temp);

  return true;
}

String SocketPlug::getDeviceType() {
  return F("SocketPlug");
}

void SocketPlug::setSwitchState(unsigned char id, unsigned char state) {
  digitalWrite(REDLED, state);
  digitalWrite(RELAY, state);
}

void flipBlueLed()
{
  unsigned char state = digitalRead(BLUELED);  // get the current state of GPIO1 pin
  digitalWrite(BLUELED, (!state) & ~silentMode);     // set pin to the opposite state
}

void flashBlueLed(uint time) {
    if(time==0) {
        if(blueIsFlashing) {
          blueLedBlink.detach();
          blueIsFlashing=false;
        }
    }
    else {
      blueLedBlink.attach_ms(time, flipBlueLed);
      blueIsFlashing=true;
    }
}

//Interrupt service routine
void buttonInterrupt();
//volatile bool silentModeISR = false;    //Used in ISR

void SocketPlug::setup(){
  pinMode(REDLED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BLUELED, OUTPUT);
  pinMode(BUTTON, INPUT);
  digitalWrite(BLUELED, 0);
  digitalWrite(REDLED, 0);
  digitalWrite(RELAY, 0);

  long keypressTime = millis();
  digitalWrite(REDLED,1);
  delay(1000);

  if(digitalRead(BUTTON)==0) {
    DEBUG_PRINT_LN(F("Button press on startup, going to AP mode"));
    delay(100); //Debounce
    forceAP=true;
    while(digitalRead(BUTTON)==0) {
      yield();
    }
    if(keypressTime + 5000 < millis()) {
      DEBUG_PRINT_LN(F("Long button press, deleting settings"));
      deleteSettings();
    }
  }
  digitalWrite(REDLED, 0);

}

void SocketPlug::prepareReset(){
  digitalWrite(BLUELED, 0);
  digitalWrite(REDLED, 0);
  digitalWrite(RELAY, 0);
}

void SocketPlug::start(){
  DEBUG_PRINT_LN("SocketPlug::start");
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInterrupt, FALLING);
}

void SocketPlug::loop(){
  //Check if relay has changed. TODO: This should probably be moved somewhere else
  int currentRelayState = digitalRead(RELAY);

  //Detect relay change. Publish to mqtt
    if( lastRelayState != currentRelayState ) {
      DEBUG_PRINT_LN(F("Relay state changed"));
      if(client.connected()) {
        DEBUG_PRINT_LN(F("MQTT connected, publishing"));
      client.publish(outTopic.c_str(), String(currentRelayState).c_str(), true);
    }
    else {
      DEBUG_PRINT_LN(F("MQTT disabled/not connected, no publish"));
    }
      lastRelayState = currentRelayState;
  }

  //Do sensor poll
  unsigned long now = millis();

    if (sensorLastWriteTime + SENSOR_POLL_INTERVAL_MS < now || (now < sensorLastWriteTime) )  {
      DEBUG_PRINT_LN("Polling sensors");

      for(int i=0;i<SENSOR_COUNT;i++) {
        DEBUG_PRINT("Sensor: ");
        DEBUG_PRINT_LN(i);

        bool result = false;
        int pin;
        switch(i) {
          case 0: pin = 4;//5;//13;
            break;
          case 1: pin = 5;
            break;
          case 2: pin = 2;
            break;
          case 3: pin = 12;
            break;
        }
        switch (sensorType[i]) {
          case NoSensor :
            DEBUG_PRINT_LN("No sensor configured");
            result = false;
            break;
          case DS18B20 :
            DEBUG_PRINT("Sensor: DS18B20, pin ");
            DEBUG_PRINT_LN(pin);

            result = getDS18B20Temperature(sensorValues[i][0], pin);
            break;
          case DHT22 :
            DEBUG_PRINT("Sensor: DHT22, pin ");
            DEBUG_PRINT_LN(pin);
            result = getDHT22HumidityTemperature(pin, sensorValues[i][0], sensorValues[i][1]);
            break;
        }
        sensorValueValid[i] = result;
      }

      if(client.connected()) {

        DEBUG_PRINT_LN("mqtt client connected, posting sensor values");

        for(int i=0; i<SENSOR_COUNT; i++) {
          if(sensorValueValid[i]) {

            if(sensorType[i] == DS18B20) {

              // String topic0 = String("sensor") + String(i) + String("/temp");
              String topic0 = mqttUser + "/" + myHostname + String("/sensor/") + String(i) + String("/temp");  //MP

              String value0 = String(sensorValues[i][0]);
              client.publish(topic0.c_str(), value0.c_str(), true);
            }
            else if(sensorType[i] == DHT22) {
                // String topic1 = String("sensor") + String(i) + String("/temp");
                String topic1 = mqttUser + "/" + myHostname + String("/sensor/") + String(i) + String("/temp");  //MP
                String value1 = String(sensorValues[i][0]);
                client.publish(topic1.c_str(), value1.c_str(), true);

                String topic2 = mqttUser + "/" + myHostname + String("/sensor/") + String(i) + String("/hum");
                String value2 = String(sensorValues[i][1]);
                client.publish(topic2.c_str(), value2.c_str(), true);
            }
          }
          else {
            DEBUG_PRINT_LN("Sensor" + String(i) + "invalid");
          }
        }

        if(client.connected()) {
          if( lastRelayState != currentRelayState ) {
            DEBUG_PRINT_LN(F("Relay state changed, mqtt publish"));
            client.publish(outTopic.c_str(), String(currentRelayState).c_str(), true);
            lastRelayState = currentRelayState;
          }
        }


      DEBUG_PRINT_LN("--mqtt Post complete");

      }
      else {
        DEBUG_PRINT_LN("Not connected, result will not be posted");
      }

      sensorLastWriteTime = now;
  }



}

//ISR
void buttonInterrupt() {

  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

   // If interrupts come faster than 1000ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 500)  {
    if(digitalRead(RELAY) == 0)
    {
      digitalWrite(RELAY,1);
      digitalWrite(REDLED, 1 & ~silentMode);
    }
    else
    {
      digitalWrite(RELAY,0);
      digitalWrite(REDLED, 0 & ~silentMode);
    }
  }
  last_interrupt_time = interrupt_time;
}



void SocketPlug::onStateChange(thingState newState) {
  DEBUG_PRINT_LN(F("SocketPlug::onStateChange"));
  switch (newState) {
    case boot:
      DEBUG_PRINT_LN(F("State is boot"));
      flashBlueLed(notConnectedFlashTime);  //Start flashing
      break;
    case smartconfig:
    DEBUG_PRINT_LN(F("State is smartconfig"));
      break;
    case apconfig:
      DEBUG_PRINT_LN(F("State is apconfig"));
      flashBlueLed(apModeFlashTime);  //Start flashing AP
      break;
    case offline:
      DEBUG_PRINT_LN(F("State is offline"));
      flashBlueLed(notConnectedFlashTime);  //Start flashing
      //digitalWrite(REDLED,0);
      break;
    case online:
      DEBUG_PRINT_LN(F("State is online"));
        flashBlueLed(0);                              //Stop flashing
        digitalWrite(BLUELED, 0x01 & ~silentMode );   //Sold blue led
      break;
    case cloudConnected:
      DEBUG_PRINT_LN(F("State is cloud connected"));
      break;
    default:
      DEBUG_PRINT_LN(F("State is unknown"));
      break;

  }

}


void SocketPlug::setupApiRoutes(ESP8266WebServer& server) {
  DEBUG_PRINT_LN(F("setupApiRoutes callback"));

  //TODO: Can this possiubly work?!

  //Sensor API
  server.on("/api/sensors", HTTP_GET, [&](){      //variable capture for lambda
    StaticJsonBuffer<768> jsonBuffer;

    //TODO: Move this and share with device.ino json builder
    JsonArray& data = jsonBuffer.createArray();
    for(int i=0; i<SENSOR_COUNT; i++) {
        if(sensorType[i] == DS18B20) {
          JsonObject& item = data.createNestedObject();
          item["type"] = String("DS18B20");
          item["valid"] = sensorValueValid[i];
          if(sensorValueValid[i]) {
            item["temperature"] = String(sensorValues[i][0]);
          }
        }
        else if(sensorType[i] == DHT22) {
            JsonObject& item = data.createNestedObject();
            item["type"] = String("DHT22");
            item["valid"] = sensorValueValid[i];
            if(sensorValueValid[i]) {
              item["temperature"] = String(sensorValues[i][0]);
              item["humidity"] = String(sensorValues[i][1]);
            }
        }
        else {
          JsonObject& item = data.createNestedObject();
          item["type"] = String("none");
        }

    }
    String json;
    json.reserve(512);
    data.printTo(json);
    server.send(200, "application/json", json);
    json = String();
  });

  //Switches API
  server.on("/api/switches", HTTP_GET, [&](){
    String json = "[";
    switch0.addToJSON(json);
    json += "]";
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/0", HTTP_GET, [&](){
    String json;
    switch0.addToJSON(json);
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/switches/0", HTTP_PUT, [&](){
    handlePutSwitch(server);
  });

}

void SocketPlug::onMqttConnect() {
  DEBUG_PRINT_LN(F("MQTT Connect callback"));
  outTopic = mqttUser + "/" + myHostname + "/switches/0";
  inTopic = mqttUser + "/" + myHostname + "/switches/0/set";
  client.subscribe(inTopic.c_str());
  client.publish(outTopic.c_str(), String(lastRelayState).c_str(), true);   //TODO: Start moving away from using String class
  lastRelayState = digitalRead(RELAY);
}

void SocketPlug::onMqttDisconnect() {
  DEBUG_PRINT_LN(F("MQTT Disconnect callback"));
}

void SocketPlug::onMqttReceiveMessage(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT_LN(F("MQTT Message arrived ["));
  DEBUG_PRINT_LN(topic);
  DEBUG_PRINT(F("] "));
  for (int i = 0; i < length; i++) {
    DEBUG_PRINT((char)payload[i]);
  }
  DEBUG_PRINT_LN();

  if ((char)payload[0] == '1') {
    switch0.setState(1);
    digitalWrite(REDLED, 1 & ~silentMode);    //TODO: Maybe add signalling led to switch class
  }
  else {
    switch0.setState(0);
    digitalWrite(REDLED, 0);    //TODO: Maybe add signalling led to switch class
  }
}

void SocketPlug::handlePutSwitch(ESP8266WebServer& server) {
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
          switch0.setState(state);
          digitalWrite(REDLED, state & ~silentMode);    //TODO: Maybe add signalling led to switch class

          DEBUG_PRINT(F("switch0.state <- "));
          DEBUG_PRINT_LN(String(switch0.getState()) );
      }
      server.send(200); //, "text/plain", "");
  }
}
