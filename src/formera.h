#ifndef FORMERA_H
#define FORMERA_H

//Generic functions for all Formera ESP8266 based devices

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <DNSServer.h>
#include <ESP8266mDNS.h>
//#include <EEPROM.h>

#include <ArduinoOTA.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "switch.h"
#include "sensor.h"




unsigned int udpListenPort = 8888;
WiFiUDP udp;
char packetBuffer[64]; //buffer to hold incoming and outgoing packets

const unsigned long defaultRetryTime = 60000;         //Don't set retry time too low as retry interfere the softAP operation

unsigned long retryTime = defaultRetryTime;         //Don't set retry time too low as retry interfere the softAP operation

//Used for cycling wifi
unsigned long wifiCycleTimer = 0;
const unsigned long wifiCycleTimeout = 3600000; //Power cycle wifi connection every 1 hour
//const unsigned long wifiCycleTimeout = 60000; //Power cycle wifi connection every 1 minutes
//const unsigned long wifiCycleTimeout = 600000; //Power cycle wifi connection every 10 minutes
//const unsigned long wifiCycleTimeout = 120000; //Power cycle wifi connection every 2 minutes
//const unsigned long wifiCycleTimeout = 54000000; //Power cycle wifi connection every 15 hours
//const unsigned long wifiCycleTimeout = 14400000; //Power cycle wifi connection every 4 hours
bool wifiCycle = false;
bool fsMounted = false;

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
char myHostname[32] = "";   // hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
char inputIp[16] = "";
char inputGateway[16] = "";
char inputSubnet[16] = "";

String friendlyName = "";

unsigned long debugLastWriteTime = 0;   //unsigned long to match millis()
unsigned int wifiConnects = 0;

bool mqttSecure = false;
bool mqttConnected = false;
bool mqttEnabled = false;
String mqttHost = "";
int mqttPort = 33333;
String mqttUser = "";     //TODO: Move these to char[]
String mqttToken = "";

bool apMode = false;

// DNS server, used by captive portal
const byte DNS_PORT = 53;
//DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

// Soft AP network parameters
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

//Static ip
IPAddress ip(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress subnet(0, 0, 0, 0);

bool useDHCP = true;   //TODO: Currenty not used, instead checking if ip[0]!=0
bool apServerStarted = false;
// Should I connect to WLAN asap?
bool connect;
bool reboot = false;        //flag to initiate clean reboot

volatile bool silentMode = false;    //Disable leds

bool forceAP=false;

unsigned long rebootInitiated = 0;

// Last time I tried to connect to WLAN
unsigned long lastConnectTry = 0;

// Current WLAN status
int status = WL_IDLE_STATUS;

String defaultHostname = "Formera" + String(ESP.getChipId());


long loopTimer = 0;
String baseDevicetime = "";

unsigned long sensorLastWriteTime = 0;   //unsigned long to match millis()

int sensorType[SENSOR_COUNT];
float sensorValues[SENSOR_COUNT][2];
bool sensorValueValid[SENSOR_COUNT];

// PIR Sensor HC-SR501
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status
int motionLed = 12; //D6

#endif
