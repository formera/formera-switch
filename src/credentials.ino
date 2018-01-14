
//TODO: update settings should be able to replace the parsing part of loadSettings
bool updateSettings(const String& json) {
  StaticJsonBuffer<1536> jsonBuffer;
  JsonObject& settings = jsonBuffer.parseObject(json);
  if (!settings.success()) {
        DEBUG_PRINT_LN(F("Failed to parse JSON"));
        return false;
      }
      else {
        DEBUG_PRINT_LN(F("Updating settings"));

        if (settings.containsKey("silent_mode")) {
          silentMode = settings["silent_mode"];
          //DEBUG_PRINT_LN("silentMode <- " + String(silentMode));
        }
        if (settings.containsKey("use_dhcp")) {
          useDHCP = settings["use_dhcp"];
          //DEBUG_PRINT_LN("useDHCP <- " + String(useDHCP));
        }
        if (settings.containsKey("ssid")) {
          snprintf(ssid, 32, settings["ssid"]);
          //DEBUG_PRINT_LN("ssid <- " + String(ssid));
        }
        if (settings.containsKey("password")) {
          snprintf(password, 32, settings["password"]);
          //DEBUG_PRINT_LN("password <- " + String(password));
        }

        if (settings.containsKey("name")) {
          friendlyName = settings["name"].as<char*>();
          //DEBUG_PRINT_LN("friendlyName <- " + String(friendlyName));
        }

        if (settings.containsKey("hostname")) {
          snprintf(myHostname, 32, settings["hostname"]);
          //DEBUG_PRINT_LN("myHostname <- " + String(myHostname));
        }

        if (settings.containsKey("static_ip")) {
          ip.fromString(settings["static_ip"].as<char*>());
          //DEBUG_PRINT_LN("static_ip <- " + ip.toString());
        }
        if (settings.containsKey("gateway")) {
          gateway.fromString(settings["gateway"].as<char*>());
          //DEBUG_PRINT_LN("gateway <- " + gateway.toString());
        }
        if (settings.containsKey("subnet")) {
          subnet.fromString(settings["subnet"].as<char*>());
          //DEBUG_PRINT_LN("subnet <- " + subnet.toString());
        }
        if (settings.containsKey("mqtt_enabled")) {
          mqttEnabled = settings["mqtt_enabled"];
          //DEBUG_PRINT_LN("mqttEnabled <- " + String(mqttEnabled));
        }
        if (settings.containsKey("mqtt_secure")) {
          mqttSecure = settings["mqtt_secure"];
          //DEBUG_PRINT_LN("mqttEnabled <- " + String(mqttEnabled));
        }
        if (settings.containsKey("mqtt_host")) {
          mqttHost = settings["mqtt_host"].as<char*>();
          //DEBUG_PRINT_LN("mqttHost <- " + mqttHost);
        }
        if (settings.containsKey("mqtt_port")) {
          mqttPort = settings["mqtt_port"];
          //DEBUG_PRINT_LN("mqttHost <- " + mqttHost);
        }
        if (settings.containsKey("mqtt_user")) {
          mqttUser = settings["mqtt_user"].as<char*>();
          //DEBUG_PRINT_LN("mqttToken <- " + mqttToken);
        }
        if (settings.containsKey("mqtt_token")) {
          mqttToken = settings["mqtt_token"].as<char*>();
          //DEBUG_PRINT_LN("mqttToken <- " + mqttToken);
        }
        if (settings.containsKey("base_devicetime")) {
          baseDevicetime = settings["base_devicetime"].as<char*>();
          //DEBUG_PRINT_LN("base_devicetime <- " + base_devicetime);
        }
        if (settings.containsKey("sensor0_type")) {
           sensorType[0] = SetSensorTypeFromString(settings["sensor0_type"].as<char*>());
        }
    /*    if (settings.containsKey("sensor1_type")) {
          sensorType[1] = SetSensorTypeFromString(settings["sensor1_type"].asString());
        }
        if (settings.containsKey("sensor2_type")) {
          sensorType[2] = SetSensorTypeFromString(settings["sensor2_type"].asString());
        }
        if (settings.containsKey("sensor3_type")) {
          sensorType[3] = SetSensorTypeFromString(settings["sensor3_type"].asString());
        }*/
        return true;
    }
}


int SetSensorTypeFromString(String s) {
  if(s == "DHT22") {
    return DHT22;
  }
  if(s == "DS18B20") {
    return DS18B20;
  }
  return NoSensor;
}

bool loadSettings() {
StaticJsonBuffer<1536> jsonBuffer;

  DEBUG_PRINT_LN(F("Load settings"));
  if(SPIFFS.exists("/settings.json")) {
    DEBUG_PRINT_LN(F("settings.json exists"));
    File file = SPIFFS.open("/settings.json", "r");
    String json = file.readString();
    file.println(json);
    DEBUG_PRINT_LN(json);
    JsonObject& settings = jsonBuffer.parseObject(json);
    if (settings.success()) {
      snprintf(ssid, 32, settings["ssid"]);
      snprintf(password, 32, settings["password"]);
      silentMode = settings["silent_mode"];
      useDHCP = settings["use_dhcp"];
      friendlyName = settings["name"].as<char*>();
      snprintf(myHostname, 32, settings["hostname"]);
      if(ip.fromString(settings["static_ip"].as<char*>())) {
        DEBUG_PRINT_LN(F("Static IP valid"));
      }
      else {
        DEBUG_PRINT_LN(F("Invalid or missing static IP"));
      }
      if (gateway.fromString(settings["gateway"].as<char*>())) {
        DEBUG_PRINT_LN(F("Gateway IP valid"));
      }
      else {
        DEBUG_PRINT_LN(F("Invalid or missing gateway IP"));
      }
      if (subnet.fromString(settings["subnet"].as<char*>())) {
        DEBUG_PRINT_LN(F("Subnet mask valid"));
      }
      else {
        DEBUG_PRINT_LN(F("Invalid or missing subnet mask"));
      }
      mqttEnabled = settings["mqtt_enabled"];
      mqttSecure = settings["mqtt_secure"];
      mqttHost = settings["mqtt_host"].as<char*>();
      if (settings.containsKey("mqtt_port")) {
        mqttPort = settings["mqtt_port"];
      }
      if (settings.containsKey("mqtt_user")) {
        mqttUser = settings["mqtt_user"].as<char*>();
      }
      if (settings.containsKey("mqtt_token")) {
        mqttToken = settings["mqtt_token"].as<char*>();
      }
      if (settings.containsKey("base_devicetime")) {
        baseDevicetime = settings["base_devicetime"].as<char*>();
      }
      if (settings.containsKey("sensor0_type")) {
         sensorType[0] = SetSensorTypeFromString(settings["sensor0_type"].as<char*>());
      }
  /*    if (settings.containsKey("sensor1_type")) {
        sensorType[1] = SetSensorTypeFromString(settings["sensor1_type"].asString());
      }
      if (settings.containsKey("sensor2_type")) {
        sensorType[2] = SetSensorTypeFromString(settings["sensor2_type"].asString());
      }
      if (settings.containsKey("sensor3_type")) {
        sensorType[3] = SetSensorTypeFromString(settings["sensor3_type"].asString());
      }*/

    if(ssid[0]==0) {  //creadentials cleared   //Added MP 2017-09-19
      DEBUG_PRINT_LN("empty ssid, credentials clear");
      return false;
    }

      file.close();
      return true;
    }
    else {
      DEBUG_PRINT_LN(F("Settings parse failed"));
      return false;
    }

  }
  else {
    DEBUG_PRINT_LN(F("Settings not found"));
    return false;
  }
}

void saveSettings() {
  String json;
  apiBuildSettingsJSON(json, true);

  File file = SPIFFS.open("/settings.json", "w");
  if(file) {
    DEBUG_PRINT_LN(F("Writing settings"));
    DEBUG_PRINT_LN(json);
    file.print(json);
    //settings.printTo(file);
    file.close();
  }
  else {
    DEBUG_PRINT_LN(F("Unable to open file for writing"));
  }
}


void deleteSettings() {
  if(SPIFFS.remove("/settings.json"))  {
    DEBUG_PRINT_LN(F("Settings file deleted"));
  }
  else {
    DEBUG_PRINT_LN(F("Failed to remove settings"));
  }
}
