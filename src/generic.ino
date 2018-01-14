

void reset()
{
  device->prepareReset();
  ESP.reset();
}


void genericSetup() {

  DEBUG_PRINT_LN(F("Generic setup"));

  //Device specific
  device->setup();                  //Do device specific setup
  device->onStateChange(boot);      //We are now booting

  //Generic setup

    WiFi.persistent(false); // Do not memorise new connections - See more at: http://www.esp8266.com/viewtopic.php?f=33&t=8628&start=4#sthash.ypTaSN0A.dpuf
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);  //Default to STA, we will switch to AP if neccessary

    DEBUG_PRINT_LN(F("Boot initiated"));

    DEBUG_PRINT_LN();
    DEBUG_PRINT_LN(F("Starting up..."));
    DEBUG_PRINT(F("Free memory at start: "));
    DEBUG_PRINT_LN(ESP.getFreeHeap());

    //Start file system
    if(SPIFFS.begin()) {
      DEBUG_PRINT_LN(F("SPIFFS mounted ok"));
      fsMounted = true;
    }
    else {
      DEBUG_PRINT_LN(F("Failed to mount SPIFFS, formatting..."));
      delay(100);
      if(SPIFFS.format()) {
        DEBUG_PRINT_LN(F("SPIFFS formatted"));
        delay(100);
        if(SPIFFS.begin()) {
          DEBUG_PRINT_LN(F("SPIFFS mounted ok"));
          fsMounted = true;
        }
        else {
          DEBUG_PRINT_LN(F("Failed to mount SPIFFS after format. Starting anyway..."));
        }
      }
    }



    bool loadOk = loadSettings();   //Load all settings
    DEBUG_PRINT_LN(F("Settings done"));

  //  bool loadOk = loadCredentials(); // Load WLAN credentials
    if (loadOk && !forceAP) {
        //Credentials exists and ok, don't start AP
          //Put this here for now as ap mode can call setupwificlient in STA_AP
        setupWifiClient();
        apMode = false;
    }
    else {
        setupWifiAP();
        device->onStateChange(apconfig);
        apMode = true;
    }
    setupWebServer();

    setupOTA();

    mqtt_setup();

    if(!apMode) {
      DEBUG_PRINT_LN(F("Starting HTTP and UDP servers"));
      server.begin(); // Web server start
      udp.begin(udpListenPort);
    }

    DEBUG_PRINT_LN(F("Setup done."));
    DEBUG_PRINT(F("Free memory: "));
    DEBUG_PRINT_LN(ESP.getFreeHeap());
    debugLastWriteTime = millis();
    wifiCycleTimer = millis();
}

void genericStart() {
  device->start();

  DEBUG_PRINT_LN(F("Generic start"));

}

void genericLoop() {

    //OTA
    ArduinoOTA.handle();

    if(apMode) {
      //DNS
      //dnsServer.processNextRequest();
      if(!apServerStarted) {
        DEBUG_PRINT_LN(F("AP mode, starting web and udp server"));
        server.begin(); // Web server start
        udp.begin(udpListenPort);
        apServerStarted= true;
      }

    }

    //HTTP
    server.handleClient();

    handleUDP();    //Handle udp discovery

    mqtt_loop();

    //Connect/reconnect etc.
    handleWifiConnection();

    if(reboot) {
        if(rebootInitiated == 0) {
          DEBUG_PRINT_LN(F("Reboot initiated"));
          rebootInitiated = millis();
        }
        else {
          DEBUG_PRINT(".");
          if((rebootInitiated + 2000) < millis()) {
            DEBUG_PRINT_LN(F("bye!"));
            reset();          //TODO: Not always working, may be ok in staging core 2.1.0 or just a pullup issue?!
          }
        }
        delay(50);
    }

    unsigned long now = millis();

    if (debugLastWriteTime + 30000 < now || (now < debugLastWriteTime) )  {
      DEBUG_PRINT(F("Esp heartbeat! Heap: "));
      DEBUG_PRINT_LN(ESP.getFreeHeap());
      debugLastWriteTime = now;
    }

    //Cycle wifi connected to reset stack and servers. Also cycle on millis() overflow at approx. 50 days...
    if ( (wifiCycleTimer + wifiCycleTimeout < now) || (now < wifiCycleTimer) )  {
      DEBUG_PRINT_LN(F("Cycle Wifi"));
      wifiCycleTimer = now;
  //    WiFi.disconnect();
      wifiCycle = true;
      mqtt_disconnect(); //Take down the mqtt connection gracefully
      //Force sleep of wifi to reset everything...
      WiFi.forceSleepBegin();
      delay(100);
      WiFi.forceSleepWake();
      delay(100);
    }


    device->loop();
}
