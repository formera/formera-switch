
#include <PubSubClient.h>

WiFiClientSecure mqttClientSecure;
WiFiClient mqttClientUnsecure;
PubSubClient client;

long lastReconnectAttempt = 0;
const int mqttConnectDelay = 10000;

void callback(char* topic, byte* payload, unsigned int length) {

  device->onMqttReceiveMessage(topic, payload, length);


}

String statusTopic;



boolean mqtt_reconnect() {
  DEBUG_PRINT_LN(F("MQTT reconnect"));
  bool connected = false;
  if(mqttUser.length() == 0) {
    connected = client.connect(defaultHostname.c_str(), statusTopic.c_str(), 0, true, "offline");
  }
  else{
    connected = client.connect(defaultHostname.c_str(), mqttUser.c_str(), mqttToken.c_str(), statusTopic.c_str(), 0, true, "offline");
  }

  if (connected) {
    DEBUG_PRINT_LN(F("MQTT connected"));

    // Once connected, publish online and switch state messages
    device->onStateChange(cloudConnected);    //Device is responsible to publish switch/sensor status
    client.publish(statusTopic.c_str(), "online", true);   //TODO: Start moving away from using String class
    device->onMqttConnect();
    // ... and resubscribe
  }
  else {
    DEBUG_PRINT_LN(F("Failed to connect to MQTT host"));
  }
  return client.connected();
}

//TODO: rebuildTopics should be called if mqtt user is changed. Maybe will require a reboot instead
void mqtt_rebuildTopics() {
  //Build topics using structure mqttUser/chipid/...
  statusTopic = mqttUser + "/" + myHostname + "/status";      //TODO: Decide on topic structure

  DEBUG_PRINT(F("Status topic: "));
  DEBUG_PRINT(statusTopic);
}

void mqtt_setup() {
  DEBUG_PRINT(F("MQTT setup: "));
  DEBUG_PRINT(mqttHost.c_str());
  DEBUG_PRINT(F(":"));
  DEBUG_PRINT_LN(mqttPort);

  if(mqttSecure) {
    DEBUG_PRINT_LN(F("Using secure mqtt"));
    client.setClient(mqttClientSecure);
  }
  else {
    DEBUG_PRINT_LN(F("Using unsecure mqtt"));
    client.setClient(mqttClientUnsecure);
  }

  client.setServer(mqttHost.c_str(), mqttPort);
  client.setCallback(callback);
  lastReconnectAttempt = 0;
  mqtt_rebuildTopics();
}

void mqtt_disconnect() {
  if(client.connected()) {
    client.publish(statusTopic.c_str(), "offline", true);   //send offline status before disconnect
  }

  client.disconnect();
  mqttConnected = false;
}

void mqtt_loop() {
  if(!mqttEnabled)
    return;
  if(WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if(mqttConnected) {
          DEBUG_PRINT_LN(F("MQTT connection lost"));
          mqttConnected = false;
          device->onMqttDisconnect();
      }

      long now = millis();
      if (now - lastReconnectAttempt > mqttConnectDelay) {
        lastReconnectAttempt = now;
        // Attempt to reconnect
        if (mqtt_reconnect()) {
          lastReconnectAttempt = 0;
          mqttConnected = true;
        }
      }
    }
    else {
      //Wifi and mqtt connected


      // Client connected
    }
  }
  else {
    if(mqttConnected) {
      DEBUG_PRINT_LN(F("MQTT, wifi connection lost, setting disconnected state"));
      mqtt_disconnect();
    }
  }
  client.loop();  //update mqtt client
}
