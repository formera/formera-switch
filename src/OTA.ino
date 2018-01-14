
void setupOTA() {
    //######
    //OTA
    ArduinoOTA.onStart([]() {
//      DEBUG_PRINT_LN(F("OTA Start"));
    });
    ArduinoOTA.onEnd([]() {
//      DEBUG_PRINT_LN(F("OTA End"));
      reset();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //    DEBUG_PRINT_F("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
    //  DEBUG_PRINT_F("Error[%u]: ", error);
       if (error == OTA_AUTH_ERROR) DEBUG_PRINT_LN("Auth Failed");
       else if (error == OTA_BEGIN_ERROR) DEBUG_PRINT_LN("Begin Failed");
       else if (error == OTA_CONNECT_ERROR) DEBUG_PRINT_LN("Connect Failed");
       else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINT_LN("Receive Failed");
       else if (error == OTA_END_ERROR) DEBUG_PRINT_LN("End Failed");
    });
    ArduinoOTA.begin();

}
