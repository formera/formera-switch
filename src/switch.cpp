#include "Arduino.h"
#include "switch.h"

Switch::Switch(int id, enumSwitchType type, int pin)
{
//  pinMode(pin, OUTPUT);
  _id = id;
  _pin = pin;
  _type = type;
}

void Switch::setState(int state)
{
  digitalWrite(_pin, state);
}

int Switch::getState()
{
  return digitalRead(_pin);
}


//TODO: At some point inherited classes for relay and dimmer

void Switch::addToJSON(String& json) {
  json += "{";
  json += "\"id\":"+String(_id);
  switch (_type) {
    case relay:
      json += ", \"switch_type\":\"relay\"";
      break;
    case dimmer:
      json += ", \"switch_type\":\"dimmer\"";
      break;
  }
  json += ", \"state\":" + String(digitalRead(_pin));
  json += "}";
}
