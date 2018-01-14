/*
  switch.h
*/

#ifndef switch_h
#define switch_h

#include "Arduino.h"

enum enumSwitchType { relay, dimmer };

class Switch
{
  public:
    Switch(int id, enumSwitchType type, int pin);
    int getState();
    void setState(int state);
    void addToJSON(String& json);
  private:
    int _id;
    int _pin;
    enumSwitchType _type;
};

#endif
