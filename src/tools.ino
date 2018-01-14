
const char* GetWifiStatusText(int status) {
  switch(status) {
    case 255 : return "WL_NO_SHIELD";
    case 0 : return "WL_IDLE_STATUS";
    case 1 : return "WL_NO_SSID_AVAIL";
    case 2 : return "WL_SCAN_COMPLETED";
    case 3 : return "WL_CONNECTED";
    case 4 : return "WL_CONNECT_FAILED";
    case 5 : return "WL_CONNECTION_LOST";
    case 6 : return "WL_DISCONNECTED";
    default: return "UNKNOWN WIFI STATE";
  }
}

const char* bool_cast(const bool b) {
    return b ? "true" : "false";
}

// Is this an IP?
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

// IP to String?
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

//TODO: This is coming in ipaddress class in esp8266 arduino 2.1.0
bool IPAddressFromString(const char *address, IPAddress& out) {
    // TODO: add support for "a", "a.b", "a.b.c" formats

    uint16_t acc = 0; // Accumulator
    uint8_t dots = 0;

    while (*address)
    {
        char c = *address++;
        if (c >= '0' && c <= '9')
        {
            acc = acc * 10 + (c - '0');
            if (acc > 255) {
                // Value out of [0..255] range
                return false;
            }
        }
        else if (c == '.')
        {
            if (dots == 3) {
                // Too much dots (there must be 3 dots)
                return false;
            }
//            out._address.bytes[dots++] = acc;
            out[dots++] = acc;
            acc = 0;
        }
        else
        {
            // Invalid char
            return false;
        }
    }

    if (dots != 3) {
        // Too few dots (there must be 3 dots)
        return false;
    }
//    out._address.bytes[3] = acc;
    out[3] = acc;
    return true;
}
