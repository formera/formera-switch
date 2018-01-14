
//
void apiBuildDiagnosticsJSON(String& json) {
  json = F("{");
  json += F("\"heap\":");
  json += String(ESP.getFreeHeap());
  // json += F(", \"analog\":");
  // json += String(analogRead(A0));
  json += F(", \"gpio\":");
  json += String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
  json += F("}");
}

  void apiBuildDeviceFullJSON(String& json) {
    StaticJsonBuffer<768> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["chip_id"] = ESP.getChipId();
    root["company"] = String(F("Formera"));
    root["type"] = device->getDeviceType();
    root["fw_version"]= FIRMWARE_VERSION;
    root["uptime"]=millis()/1000;
    root["wifi_connects"]=wifiConnects;
    root["free_memory"]=ESP.getFreeHeap();
    root["name"]= (friendlyName ? friendlyName : "");
    root["mac"]= WiFi.macAddress();
    root["ssid"]= ssid;
    String strength = String( WiFi.RSSI()) +String(F("dBm"));
    root["signal_strength"]= strength;
    root["hostname"]= myHostname;
    root["ip_address"]= WiFi.localIP().toString();
    root["gateway"]= WiFi.gatewayIP().toString();
    root["subnet"]= WiFi.subnetMask().toString();
    root["primary_dns"]= WiFi.dnsIP(0).toString();
    root["secondary_dns"]= WiFi.dnsIP(1).toString();
    if(!mqttConnected && mqttEnabled) {
      root["mqtt_status"] = String(F("Disconnected"));
    } else if(mqttConnected) {
      root["mqtt_status"] = String(F("Connected"));
    }
    else {
      root["mqtt_status"] = String(F("Disabled"));
    }

    root["sensors"]=SENSOR_COUNT;   //TODO: Set variables/defines
    JsonArray& data = root.createNestedArray("sensors");
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
    root["apMode"] = apMode;
    root.printTo(json);
  }

String getSensorTypeString(int type) {
    if(type == DHT22) {
      return "DHT22";
    }
    else if(type == DS18B20) {
      return "DS18B20";
    }
    else
      return "none";
  }


void apiBuildSettingsJSON(String& json, bool includeSecrets) {

  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["silent_mode"] = silentMode;
  root["use_dhcp"] = useDHCP;
  root["ssid"] = ssid;
  if(includeSecrets) {
    root["password"] = password;
  }
  else {
    root["password"] = "*";
  }
  root["name"] = (friendlyName ? friendlyName : "");
  root["hostname"] = myHostname;
  root["static_ip"] = ip.toString();
  root["gateway"] = gateway.toString();
  root["subnet"] = subnet.toString();
  root["mqtt_enabled"] = mqttEnabled;
  root["mqtt_secure"] = mqttSecure;
  root["mqtt_host"] = (mqttHost ? mqttHost : "");
  root["mqtt_port"] = mqttPort;
  root["mqtt_user"] = (mqttUser ? mqttUser : "");
  root["mqtt_token"] = (includeSecrets ? (mqttToken ? mqttToken : "") : "*");
  root["base_devicetime"] = baseDevicetime;


    root["sensor0_type"] = getSensorTypeString(sensorType[0]);
    // root["sensor1_type"] = getSensorTypeString(sensorType[1]);
    // root["sensor2_type"] = getSensorTypeString(sensorType[2]);
    // root["sensor3_type"] = getSensorTypeString(sensorType[3]);
  root.printTo(json);
}



void apiBuildDeviceJSON(String& json) {

  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["chip_id"] = ESP.getChipId();
  root["company"] = String(F("Formera"));
  root["model"] = String(F("Socket"));
  root["name"]= (friendlyName ? friendlyName : "");
  root["hostname"]= myHostname;
  root["ip_address"]= WiFi.localIP().toString();
  root["sensors"]=SENSOR_COUNT;   //TODO: Set variables/defines
  root["switches"]=1;   //TODO: Set variables/defines
  root["apMode"] = apMode;
  root.printTo(json);
}
