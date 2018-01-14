//WiFi.mode(WIFI_AP_STA);
typedef enum {
  PING_ALL = 0x00,
    PING_DEVICE,
    PING_RESPONSE,
    REQUEST_STATUS,
    STATUS_RESPONSE,
    READ_DATAPOINT,
    WRITE_DATAPOINT
} UDP_COMMAND_E;

typedef enum {
  HEADER_FIRST = 0x00,
  HEADER_SECOND,
  PROTOCOL_VERSION,
  COMMAND,
  DATA_START
} UDP_PACKET_E;

void handleUDP(){
  int packetSize = udp.parsePacket();
  if (packetSize) {
    DEBUG_PRINT_LN(F("UDP packet received"));

    int len = udp.read(packetBuffer, 64);

    //Formera udp protocol
    //Head 2     0x46
    //           0x4F
    //Ver  1     0x00
    //Cmd  1

    //command
    //0x00    All ping
    //0x01    Ping device
    //0x03    Request status
    //0x05    Read datapoints
    //0x06    Write datapoints

    //Ping response
    //4 byte  device id (msb)
    if (packetBuffer[0] == 0x46 && packetBuffer[1] == 0x4F) {
      if(packetBuffer[2] == 0x00) {  //check version
        UDP_COMMAND_E cmd = (UDP_COMMAND_E)packetBuffer[3];
        switch (cmd) {
          case PING_ALL: {
              DEBUG_PRINT_LN(F("Ping all"));
              sendPingResponse();
            }
            break;
          case PING_DEVICE: {
              DEBUG_PRINT(F("Ping device"));
              if(len-DATA_START == 4) {
                unsigned long deviceId = packetBuffer[4] << 24;
                deviceId |= packetBuffer[5] << 16;
                deviceId |= packetBuffer[6] << 8;
                deviceId |= packetBuffer[7];
                DEBUG_PRINT_LN(deviceId);
                if(deviceId == ESP.getChipId()) {
                  DEBUG_PRINT_LN(F("Thats me!"));
                  sendPingResponse();
                }
              }
            }
            break;
            case REQUEST_STATUS: {
                DEBUG_PRINT_LN(F("Read datapoint"));
            }
            case READ_DATAPOINT: {
                DEBUG_PRINT_LN(F("Read datapoint"));
            }
            case WRITE_DATAPOINT: {
                DEBUG_PRINT_LN(F("Read datapoint"));
            }
          break;
          default:
            DEBUG_PRINT_LN(F("Unknown command"));
        }
      }
      else {
        DEBUG_PRINT_LN(F("Unkown protocol version"));
      }
    }


    if (len > 0) packetBuffer[len] = 0;
    if(String(packetBuffer)=="helloesp"){
      unsigned long delayTime = (millis() ^ ESP.getChipId()) & 0xFF;
      DEBUG_PRINT(F("Delaying response (ms):"));
      DEBUG_PRINT_LN(delayTime);
      delay(delayTime);     //"random" delay
      udp.beginPacket(udp.remoteIP(),udp.remotePort());
      String json;
      json.reserve(512);
      apiBuildDeviceJSON(json);
      udp.write(json.c_str());
      udp.endPacket();
      DEBUG_PRINT_LN(F("Response sent"));
    }
  }
}

void sendPingResponse() { //unsigned char cmd) {
  unsigned char buf[32];
  buf[0] = 0x46;
  buf[1] = 0x4F;
  buf[2] = 0x00;
  buf[3] = PING_RESPONSE;
  unsigned long chipId =  ESP.getChipId();
  buf[4] = (chipId >> 24) & 0xFF;
  buf[5] = (chipId >> 16) & 0xFF;
  buf[6] = (chipId >> 8) & 0xFF;
  buf[7] = (chipId) & 0xFF;

  udp.beginPacket(udp.remoteIP(), 8888);//udp.remotePort());
  udp.write(buf, 8);
  udp.endPacket();
}

void setupWifiAP() {
    DEBUG_PRINT_LN(F("Starting AP mode"));

    WiFi.mode(WIFI_AP);
    //  WiFi.setAutoConnect(false);   //TODO: This will be added in core 2.1.0
    DEBUG_PRINT_LN(F("Configuring access point..."));
    // You can remove the password parameter if you want the AP to be open.
    //  WiFi.softAP(softAP_ssid, softAP_password);
    //WiFi.softAP(softAP_ssid);   //No Password
    WiFi.softAP(defaultHostname.c_str());   //No Password

    WiFi.softAPConfig(apIP, apIP, netMsk);
    delay(500); // Without delay I've seen the IP address blank
    DEBUG_PRINT(F("AP IP address: "));
    DEBUG_PRINT_LN(WiFi.softAPIP());
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);

}

void setupWifiClient() {
  DEBUG_PRINT_LN(F("Starting Client mode"));
  //WiFi.wifi_station_set_hostname("foobar");

  if(myHostname[0]==0) {
      DEBUG_PRINT_LN(F("Empty stored host name, will use default"));
      defaultHostname.toCharArray(myHostname, sizeof(myHostname)-1);
  }
  DEBUG_PRINT(F("Will use hostname: "));
  DEBUG_PRINT_LN(myHostname);
  WiFi.hostname(myHostname);    //TODO: Not sure this actually works

  //Set flag so main loop will connect
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
}


//TODO: There is currently no way to go from static IP to DHCP without reboot.
void connectWifi() {
  //Unit will auto reconnect

  DEBUG_PRINT_LN(F("Connecting as wifi client..."));
  int s = WiFi.status();
  DEBUG_PRINT(F("Current wifi status: "));
  DEBUG_PRINT_LN(String(s));
  if(s != 0) {
    DEBUG_PRINT_LN(F("Not ready or already connected"));
    WiFi.disconnect();    //TODO: Should reenable this and wait for disconnect and then connect again
    retryTime = 10000;
    //if we are connected and try to connect to same ssid but with dhcp we need to disconnect first.
  }
  delay(500);   //not needed?

  DEBUG_PRINT ( F("Connecting to: ") );
  DEBUG_PRINT ( ssid );
  DEBUG_PRINT ( F(", password: ") );
  DEBUG_PRINT_LN ( password );

  WiFi.begin ( ssid, password );      //Not sure what happens if we run this and already connected to another SSID?
  delay(100);
  if(!useDHCP) {
    DEBUG_PRINT ( F("Use Static IP set. Using: "));
    DEBUG_PRINT_LN ( ip.toString() );
    WiFi.config(ip, gateway, subnet);
    delay(100);
  }
  else{
    DEBUG_PRINT_LN ( F("Using DHCP"));
  }
  int connRes = WiFi.waitForConnectResult();
  DEBUG_PRINT ( F("connRes: ") );
  DEBUG_PRINT_LN ( String(GetWifiStatusText(connRes)) );
  if(connRes != WL_CONNECTED) {
    WiFi.begin();     //TODO: When running on switch hardware, we are not connected after waitForConnectResult so this is needed!? And on nodemcu running begin() if connected will go to disconnect instead of idle
  }
}

//bool retryConnect = false;

void handleWifiConnection() {
  if (connect) {
    DEBUG_PRINT_LN ( F("Connect requested") );
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + retryTime) ) {
      // If WLAN disconnected and idle try to connect
      retryTime = defaultRetryTime; //Reset to default retry time
      connect = true;
    }
    if (status != s) { // WLAN status change      0 is idle. If ssid is lost status will be wl_no_ssid_avail, to this will not be run. It should reconnect automatically
      DEBUG_PRINT( F("Wifi status changed to: ") );
      DEBUG_PRINT_LN ( String(GetWifiStatusText(s)) );
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        DEBUG_PRINT ( F("Connected to ") );
        DEBUG_PRINT_LN ( ssid );
        DEBUG_PRINT ( F("IP address: ") );
        DEBUG_PRINT_LN ( WiFi.localIP() );
        wifiConnects+=1;

        device->onStateChange(online);
        wifiCycle = false;

      }
      else {
        if( status == WL_CONNECTED) {   //Last status was connected
          DEBUG_PRINT_LN( F("Connection lost!") );

          //if wifi cycle flag is set, reconnect immediatly
          if(wifiCycle) {
            DEBUG_PRINT_LN( F("Disconnect for Wifi restart, ignoring on state change signal") );
            //connect=true;
            wifiCycle = false;
            //delay(100);
          }
          else {
            //dont flash on if we initiate wifi disconnect ie cycle
              device->onStateChange(offline);
          }
        }
        if (s == WL_NO_SSID_AVAIL) {
          DEBUG_PRINT_LN( F("WL_NO_SSID_AVAIL") );
          //WiFi.disconnect();    //Dont think this is needed
          //maybe needed: status = WL_IDLE_STATUS;    //Doesnt seem to set status on disconnect?
        }
      }
      status = s;
    }
  }
}
