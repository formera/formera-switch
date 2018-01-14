#ifndef DEBUG_H
#define DEBUG_H

// #define DEBUG

#ifdef DEBUG
  #define DEBUG_UART Serial      //Debug port
  #define DEBUG_PRINT(x)      DEBUG_UART.print (x)
  #define DEBUG_PRINT_LN(x)   DEBUG_UART.println (x)
  #define DEBUG_PRINT_F(x, y)   DEBUG_UART.printf (x, y)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT_LN(x)
  #define DEBUG_PRINT_F(x)
#endif


#endif
