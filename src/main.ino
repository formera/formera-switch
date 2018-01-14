
/*

#define DEBUG_ESP_CORE
#define DEBUG_ESP_SSL
#define DEBUG_ESP_WIFI
#define DEBUG_ESP_HTTP_CLIENT
#define DEBUG_ESP_HTTP_UPDATE
#define DEBUG_ESP_HTTP_SERVER
#define DEBUG_ESP_UPDATER
#define DEBUG_ESP_OTA
#define DEBUG_TLS_MEM

Application flow:

* Device specific setup
 - not dependent on generic

* Setup is run on generic device
 - not dependent on specific

* Start on generic
 - All callbacks should be registered

Generic device has callbacks for:
 - signal mode
 - get specific device info
 -

*/

//To enable debugs, define #DEBUG in debug.h

#include "formera.h"
#include "debug.h"

static const String FIRMWARE_VERSION = "1.0.8";

// ===============================================================
// enable one of the below depending on what hardware to flash

// Formera SocketPlug
// #include "socketplug.h"
// SocketPlug sp = SocketPlug();

// Formera TwoTouch
#include "wallswitch.h"
WallSwitch sp = WallSwitch();

// Formera OneTouch
// #include "wallswitchone.h"
// WallSwitchOne sp = WallSwitchOne();

// ===============================================================

Thing *const device = &sp;

void setup() {

  delay(100);

#ifdef DEBUG
  DEBUG_UART.begin(115200);
  DEBUG_UART.setDebugOutput(true);    //ESP core debug
#endif

  genericSetup();
  genericStart();
}


void loop() {
  genericLoop();
}
