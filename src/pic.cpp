//Test program esp8266<->Pic with OTA update support
//Sends hearbeat request to Pic every 5 seconds

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
//#include <SoftwareSerial.h>
#include "pic.h"
#include "debug.h"


PicClass::PicClass()
: _port(0)
, _heartbeat_callback(NULL)
, _switch_callback(NULL)
, _reset_callback(NULL)
, _progress_callback(NULL)
{
}

PicClass::~PicClass(){
}

void PicClass::onHeartBeat(THandlerFunction fn) {
    _heartbeat_callback = fn;
}

void PicClass::onResetRequest(THandlerFunction fn) {
    _reset_callback = fn;
}

void PicClass::onSwitchUpdate(THandlerFunction_Switch fn) {
    _switch_callback = fn;
}

unsigned char PicClass::get_check_sum(unsigned char *pack, unsigned short pack_len)
{
  unsigned short i;
  unsigned char check_sum = 0;

  for(i = 0; i < pack_len; i ++)
  {
    check_sum += *pack ++;
  }

  return check_sum;
}

/*
control request,not request response
data section can embed multi datapoint

datapoint
dpid  1  datapoint index
dptype  1  datapoint type
dplen	match follow table
dpvalue (MSB data struct)


type  	value  length 		comment
raw  	0x00  	x  	datapoint depand on user definition
bool    0x01  	1  	value £∫0x00/0x01
value   0x02  	4  	32bit int£¨high byte fisrt(MSB)
string  0x03  	x  	string
enum  	0x04  	1  	enumeration value 0-255
bitmap  0x05  1/2/4  	high byte fisrt(MSB)

*/

/*
wifi module  ---> TOUCH IC
head	2	0x55AA MSB
version	1	0x00
command	1	0x00(heartbeat)
length	2	0x0000 MSB
data	xxxx	none
chksum	1	head + version + command + length + data

touch  ---> wifi
head	2	0x55AA MSB
version	1	0x00
command	1	0x00(heartbeat)
length	2	0x00001 MSB
data	xxxx	0x00/0x01(0x00 for powerup, 0x01 for normal work state)
chksum	1	head + version + command + length + data
*/
void PicClass::heartbeat() {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 0x00;
  tx_buf[3] = 0x00;
  tx_buf[4] = 0x00;
  tx_buf[5] = 0x00;
  tx_buf[6] = get_check_sum(tx_buf, 6);
  picStream->write(tx_buf, 7);
}

void PicClass::requestAll() {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 0x00;
  tx_buf[3] = 0x08;
  tx_buf[4] = 0x00;
  tx_buf[5] = 0x00;
  tx_buf[6] = get_check_sum(tx_buf, 6);
  picStream->write(tx_buf, 7);
}

//55aa 00 06 00 05 0101000101 0e
void PicClass::setSwitch(unsigned char id, unsigned char value) {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 0x00;
  tx_buf[3] = 0x06;
  tx_buf[4] = 0x00;
  tx_buf[5] = 0x05;
  tx_buf[6] = id;
  tx_buf[7] = 0x01;
  tx_buf[8] = 0x00;
  tx_buf[9] = 0x01;
  tx_buf[10] = value;
  tx_buf[11] = get_check_sum(tx_buf, 11);
  picStream->write(tx_buf, 12);
  delay(10);  //we add small delay here as multiple quick relay changes can mess up the serial packets!?
}

//55aa000300010205
void PicClass::setState(WIFI_WORK_STATE_E state) {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 0x00;
  tx_buf[3] = 0x03;
  tx_buf[4] = 0x00;
  tx_buf[5] = 0x01;
  tx_buf[6] = state;
  tx_buf[7] = get_check_sum(tx_buf, 7);
  picStream->write(tx_buf, 8);
}

int state =0;

unsigned long latestHeartbeat = 0;

void PicClass::handle_heartbeat() {
  DEBUG_PRINT_LN(F("Got heart beat response"));
  latestHeartbeat = millis();
  if (_heartbeat_callback) {
    _heartbeat_callback();
  }
}

void PicClass::handle_reset_request() {
  DEBUG_PRINT_LN(F("Got reset request"));
  latestHeartbeat = millis();
  if(_reset_callback) {
    _reset_callback();
  }
}

void PicClass::handle_new_state_request() {

  unsigned char state = rx_buf[DATA_START];
  DEBUG_PRINT(F("Got new state request"));
  DEBUG_PRINT_LN(String(state));

  latestHeartbeat = millis();
}


void PicClass::handle_state_upload() {
    DEBUG_PRINT_LN(F("Got state"));
    latestHeartbeat = millis();

    //unsigned int total
    unsigned int dp_len;

    unsigned int total_len = rx_buf[LENGTH_HIGH] * 0x100;
    total_len += rx_buf[LENGTH_LOW];

    for(int i = 0;i < total_len;)
    {
      DEBUG_PRINT_LN(F("Datapoint"));

      unsigned char dp_id = rx_buf[DATA_START];
      DP_TYPE_E dp_type = (DP_TYPE_E)rx_buf[DATA_START + 1];
      dp_len = rx_buf[DATA_START + i + 2] * 0x100;
      dp_len += rx_buf[DATA_START + i + 3];
      unsigned char dp_value = rx_buf[DATA_START+4];

      if (dp_type == DP_TYPE_BOOL) {
        DEBUG_PRINT(F("Updated id "));
        DEBUG_PRINT_LN(String(dp_id) + " to " + String(dp_value));
        if(_switch_callback) {
          _switch_callback(dp_id, dp_value);
        }
      }
      else {
        DEBUG_PRINT(F("Unsupported data type "));
        DEBUG_PRINT_LN(String(dp_type));
      }
      //ret = data_point_handle(rx_buf + DATA_START + i);

      i += (dp_len + 4);
    }
}


//###############
void PicClass::clean_receive_buf(void)
{
  package_finish = 0;
  rx_count = 0;
  rx_value_len = 0;
}

void PicClass::package_handle(void)
{
  unsigned char check_sum;
  unsigned short len;

  if(package_finish)
  {
    DEBUG_PRINT_LN();

    DEBUG_PRINT_LN(F("Got complete packet"));
    len = rx_buf[LENGTH_HIGH] * 0x100;
    len += rx_buf[LENGTH_LOW];

    check_sum = get_check_sum(rx_buf, len + PROTOCOL_HEAD - 1);
    if(check_sum == rx_buf[len + PROTOCOL_HEAD - 1])
    {
      DEBUG_PRINT_LN(F("Checksum ok"));
      data_handle();
    }
    else
    {
      DEBUG_PRINT_LN(F("Checksum fail"));
      //Checksum fail
    }

    clean_receive_buf();
  }
}

void PicClass::data_handle() {
  FRAME_TYPE_E cmd_type = (FRAME_TYPE_E)rx_buf[FRAME_TYPE];
  switch(cmd_type)
  {
    case HEAT_BEAT_CMD:
      handle_heartbeat();
      break;
    case STATE_UPLOAD_CMD:
      handle_state_upload();
      break;
    case WIFI_RESET_CMD:    //Reset request (touch requres wifi module reset)
      handle_reset_request();
      break;
    case WIFI_MODE_CMD:     //Wifi new state reset request (touch requres wifi module reset to new state)
      handle_new_state_request();
      break;
    default:
      DEBUG_PRINT(F("Unsupported packet: "));
      DEBUG_PRINT_LN(String(cmd_type));
      break;
  }
}

void PicClass::uart_rx_handle(unsigned char value)
{
  if(package_finish == false)
  {
    switch(rx_count)
    {
    case HEAD_FIRST:
      if(value == 0x55)
      {
        rx_buf[rx_count] = value;
        rx_count = HEAD_SECOND;
      }
      break;

    case HEAD_SECOND:
      if(value == 0xaa)
      {
        rx_buf[rx_count] = value;
        rx_count = PROTOCOL_VERSION;
      }
      else
      {
        rx_count = HEAD_FIRST;
      }
      break;

    case PROTOCOL_VERSION:
      if(value == VERSION)
      {
        rx_buf[rx_count] = value;
        rx_count = FRAME_TYPE;
      }
      else
      {
        rx_count = HEAD_FIRST;
      }
      break;

    case FRAME_TYPE:
      rx_buf[rx_count] = value;
      rx_count = LENGTH_HIGH;
      break;

    case LENGTH_HIGH:
      rx_buf[rx_count] = value;
      rx_count = LENGTH_LOW;
      break;

    case LENGTH_LOW:
      rx_buf[rx_count] = value;
      rx_count ++;
      rx_value_len = rx_buf[LENGTH_HIGH] * 0x100;
      rx_value_len += rx_buf[LENGTH_LOW];
      if((rx_value_len + PROTOCOL_HEAD) >= WIFI_UART_RECV_BUF_LMT)
      {
        rx_count = HEAD_FIRST;
      }
     break;

    default:
      if(rx_count < WIFI_UART_RECV_BUF_LMT + PROTOCOL_HEAD)
      {
        rx_buf[rx_count] = value;
        rx_count ++;
      }

      if(rx_count == rx_value_len + PROTOCOL_HEAD)
      {
        package_finish = true;
        rx_value_len = 0;
        rx_count = 0;
      }
      break;
    }
  }
}
PicClass& PicClass::setStream(Stream* stream){
    this->picStream = stream;
    return *this;
}

PicClass Pic;
