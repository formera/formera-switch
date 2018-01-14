#include "data.h"
#include "flash.h"

void setupWebServer() {
  setupContent();
  setupCaptivePortal();
  setupApi();
}

bool loadFromFlash(String path) {
  if(path.endsWith("/")) path += "index.htm";

  int NumFiles = sizeof(files)/sizeof(struct t_websitefiles);
  for(int i=0; i<NumFiles; i++) {
    String pathWithGz = path + ".gz";
    String fsName = files[i].filename;
    if(path.endsWith(fsName) || pathWithGz.endsWith(fsName)) {
      _FLASH_ARRAY<uint8_t>* filecontent;
      unsigned int len = 0;
      String dataType = "text/plain";
      dataType = files[i].mime;

      len = files[i].len;

      DEBUG_PRINT(F("Requested: "));
      DEBUG_PRINT_LN(path);
      DEBUG_PRINT(F("Serving: "));
      DEBUG_PRINT_LN(fsName);
      String contentType = getContentType(path);
      server.setContentLength(len);
      if(pathWithGz.endsWith(fsName)) {
        server.sendHeader("Content-Encoding", "gzip");
      }
      server.sendHeader("Cache-Control","max-age=86400");   //Cache static files for 1 day
//      server.send(200, files[i].mime, "");    //We override the mimetype as file tool seems to return wrong info
      server.send(200, contentType, "");

      filecontent = (_FLASH_ARRAY<uint8_t>*)files[i].content;
      filecontent->open();

      WiFiClient client = server.client();
      unsigned char* c;

      char buf[1024];
      int siz = filecontent->size();
      while(siz > 0) {
       size_t len = std::min((int)(sizeof(buf) - 1), siz);
       filecontent->read((uint8_t *)buf, len);
       client.write((const char*)buf, len);
       siz -= len;
      }

      return true;
    }
  }

  return false;
}


void setupContent() {

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
          DEBUG_PRINT_LN(F("Get file from network"));
        if(!loadFromFlash(server.uri()))
          server.send(404, "text/plain", "FileNotFound");

  });

}

void setupCaptivePortal() {
  // Setup web pages: root, wifi config pages, SO captive portal detectors and not found.

  //Portal etc.
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
//  server.on("/generate_204", handleRoot);   //Android captive portal. Maybe not needed. Might be handled by notFound handler.
//  server.on("/fwlink", handleRoot);         //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
}


void setupApi() {
  server.on("/api/requestFactoryReset", HTTP_POST, handleFactoryReset);
  server.on("/api/requestReset", HTTP_POST, handleReset);

  server.on("/api/wifi/networks", handleScan);

  //Legacy api
  server.on("/api/on", [](){
    device->setSwitchState(0, 1);
    server.send(200, "application/json", String(F("{ \"state\": ")) + String(1) + "}");
  });

  server.on("/api/off", [](){
    device->setSwitchState(0, 0);
    server.send(200, "application/json", "{ \"state\": " + String(0) + "}");
  });

  server.on("/api/status", [](){
    server.send(200, "application/json", "{ \"name\": \"ESP" + String(ESP.getChipId()) + "\"}");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/api/diagnostics", HTTP_GET, [](){
    String json;
    apiBuildDiagnosticsJSON(json);
    server.send(200, "application/json", json);
    json = String();
  });

  //------
  server.on("/api/device", HTTP_GET, [](){
    String json;
    json.reserve(512);
    apiBuildDeviceFullJSON(json);
    server.send(200, "application/json", json);
    json = String();
  });

  server.on("/api/settings", HTTP_GET, [](){
    String json;
    json.reserve(512);
    apiBuildSettingsJSON(json, false);
    server.send(200, "application/json", json);
    json = String();
  });
/*

*/
  server.on("/api/settings", HTTP_PUT, handlePutSettings);


  server.on("/api/update", HTTP_POST, [](){
       server.sendHeader("Connection", "close");
       bool fail = Update.hasError();
       if(!fail) {
         server.send(200);
       }
       else {
         server.send(400, "application/json", "{ \"message\":\"update failed\"}");
       }
       ESP.restart();
     },[](){
       HTTPUpload& upload = server.upload();
       if(upload.status == UPLOAD_FILE_START){
         WiFiUDP::stopAll();
         DEBUG_PRINT("Update: ");
         DEBUG_PRINT_LN(upload.filename.c_str());
         uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
         if(!Update.begin(maxSketchSpace)){ //start with max available size
           //Update.printError(Serial);
         }
       } else if(upload.status == UPLOAD_FILE_WRITE){
         if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
           //Update.printError(Serial);
         }
       } else if(upload.status == UPLOAD_FILE_END){
         if( Update.end(true) ){ //true to set the size to the current progress
           DEBUG_PRINT("Update Success: ");
           DEBUG_PRINT_LN(upload.totalSize);
           DEBUG_PRINT_LN("Rebooting...");
         } else {
           //Update.printError(Serial);
         }
       }
       yield();
     });

  device->setupApiRoutes(server);

}

static String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  //DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/"))
    path += "index.htm";
  String contentType = getContentType(path);
//  getContentType(path, contentType);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handlePutSettings() {
  DEBUG_PRINT_LN(F("Api: Put settings"));
  String body = server.arg("plain");
  DEBUG_PRINT(F("request body: "));
  DEBUG_PRINT_LN(body);
  if(updateSettings(body)) {
    saveSettings();
    server.send(200, "application/json", "{ \"result\":\"ok\" }");
  }
  else {
    server.send(400, "text/plain", "bad request");
  }
}

void handleReset() {
  reboot = true;
  server.send(200);
}

void handleFactoryReset() {
  deleteSettings();
  server.send(200);
}

//TODO: Change to arduinojson
void handleScan() {
  DEBUG_PRINT_LN(F("handle scan start"));
  int n = WiFi.scanNetworks();
  DEBUG_PRINT_LN(F("handle scan done"));

  String json = F("[");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if(i > 0) {
        json+=",";
      }
      json+= String(F("{\"ssid\":\"")) + WiFi.SSID(i) + String(F("\",\"encryption\":")) + WiFi.encryptionType(i) + String(F(",\"rssi\":")) + WiFi.RSSI(i) + "}";    //TODO: Send ecryption types as tex (ENC_TYPE_NONE etc)
    }
  } else {
    json+=F("[]");
  }
  json += F("]");
  server.send(200, "application/json", json);
  json = String();
}

/** Wifi config page handler */
void handleWifi() {
  DEBUG_PRINT_LN(F("WiFi start"));

  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String response;
  response.reserve(3000);
  response += F("<html><head>"
    "<script>function c(l){document.getElementById('n').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>"
    "</head><body>"
    "<h1>Wifi config</h1>");
  ;
  if (server.client().localIP() == apIP) {
    response += String(F("<p>You are connected through the soft AP: ")) + defaultHostname + "</p>";
  } else {
    response += String(F("<p>You are connected through the wifi network: ")) + String(ssid) + "</p>";
  }
  response += F(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>")
  ;
  response+= "<tr><td>SSID " + String(defaultHostname) + "</td></tr>";
  response+= "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>";
  response+= F(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
    "<tr><td>SSID ") ;
  response+= String(ssid);
  response+= F("</td></tr>");
  response+= "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>";
  response+=F(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>");
  ;
  DEBUG_PRINT_LN(F("scan start"));
  int n = WiFi.scanNetworks();
  DEBUG_PRINT_LN(F("scan done"));
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      response+= String(F("\r\n<tr><td>SSID <a href='#' onclick='c(this)'>")) + String(WiFi.SSID(i)) +"</a> "+ String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":" *") + " (" + String(WiFi.RSSI(i)) + ")</td></tr>";
    }
  } else {
    response+= F("<tr><td>No WLAN found</td></tr>");
  }

  response += F(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network', value='");
    response+=String(ssid);
    response+=F("' id='n' name='n'/>"
    "<br /><input type='password' placeholder='password', value='' id='p' name='p'/>"
    "<br /><input type='text' placeholder='hostname', value='");
    response+=String(myHostname);
    response+=F("' name='h'/>"
    "<br /><input type='text' placeholder='ip', value='");
    response+=String(inputIp);
    response+=F("' name='i'/>"
    "<br /><input type='text' placeholder='gateway', value='");
    response+=String(inputGateway);
    response+=F("' name='g'/>"
    "<br /><input type='text' placeholder='subnet', value='");
    response+=String(inputSubnet);
    response+=F("' name='s'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>");
  ;
  server.send(200, "text/html", response); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  //server.client().stop(); // Stop is needed because we sent no content length
  DEBUG_PRINT_LN(F("WiFi done"));

}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  DEBUG_PRINT_LN(F("WiFi save"));
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.arg("h").toCharArray(myHostname, sizeof(myHostname) - 1);
  server.arg("i").toCharArray(inputIp, sizeof(inputIp) - 1);
  server.arg("g").toCharArray(inputGateway, sizeof(inputGateway) - 1);
  server.arg("s").toCharArray(inputSubnet, sizeof(inputSubnet) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length

  //Parse the ips
  if(ip.fromString(inputIp)) {
    useDHCP = false;
  }
  else {
    useDHCP = true;
  }

  gateway.fromString(inputGateway);
  subnet.fromString(inputSubnet);

  saveSettings();

}
