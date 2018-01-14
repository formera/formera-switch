#ifndef PIC_H
#define PIC_H
#include <functional>

typedef enum {
  SMART_CONFIG_STATE = 0x00,    //fast flashing
  AP_STATE,                     //slow flashing
  WIFI_NOT_CONNECTED,           //No flashing??
  WIFI_CONNECTED,               //off
  WIFI_CONNECTED2,               //off
  WIFI_STATE_UNKNOW = 0xff,
} WIFI_WORK_STATE_E;

typedef enum {
    HEAD_FIRST = 0x00,
    HEAD_SECOND,
    PROTOCOL_VERSION,
    FRAME_TYPE,
    LENGTH_HIGH,
    LENGTH_LOW,
    DATA_START,
} FRAME_ORDER_E;

typedef enum {
    HEAT_BEAT_CMD = 0,                          //ÐÄÌø°ü
    PRODUCT_INFO_CMD,                           //²úÆ·ÐÅÏ¢
    WORK_MODE_CMD,                              //²éÑ¯MCU Éè¶¨µÄÄ£¿é¹¤×÷Ä£Ê½
    WIFI_STATE_CMD,                             //wifi¹¤×÷×´Ì¬
    WIFI_RESET_CMD,                             //ÖØÖÃwifi
    WIFI_MODE_CMD,                              //Ñ¡Ôñsmartconfig/APÄ£Ê½
    DATA_QUERT_CMD,                             //ÃüÁîÏÂ·¢
    STATE_UPLOAD_CMD,                           //×´Ì¬ÉÏ±¨
    STATE_QUERY_CMD,                            //×´Ì¬²éÑ¯
    UPDATE_QUERY_CMD,                           //Éý¼¶²éÑ¯
    UPDATE_START_CMD,                           //Éý¼¶¿ªÊ¼
    UPDATE_TRANS_CMD,                           //Éý¼¶´«Êä
    GET_ONLINE_TIME_CMD,                        //»ñÈ¡ÏµÍ³Ê±¼ä
    FACTORY_MODE_CMD,                           //½øÈë²ú²âÄ£Ê½
} FRAME_TYPE_E;

typedef enum {
    DP_TYPE_RAW = 0x00,
    DP_TYPE_BOOL ,
    DP_TYPE_VALUE,
    DP_TYPE_STRING,
    DP_TYPE_ENUM,
    DP_TYPE_BITMAP,
} DP_TYPE_E;

#define VERSION  0x00
#define PROTOCOL_HEAD     0x07
#define WIFI_UART_RECV_BUF_LMT  128



class PicClass
{
  public:
  	typedef std::function<void(void)> THandlerFunction;
    typedef std::function<void(unsigned char, unsigned char)> THandlerFunction_Switch;
    typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;

    PicClass();
    ~PicClass();
    static unsigned char get_check_sum(unsigned char *pack, unsigned short pack_len);
    void heartbeat();
    void requestAll();
    void setSwitch(unsigned char id, unsigned char value);
    void setState(WIFI_WORK_STATE_E state);
    void handle_heartbeat();
    void handle_reset_request();
    void handle_new_state_request();
    void handle_state_upload();
    void clean_receive_buf(void);
    void package_handle(void);
    void data_handle();
    void uart_rx_handle(unsigned char value);

    void onHeartBeat(THandlerFunction fn);
    void onResetRequest(THandlerFunction fn);
    void onSwitchUpdate(THandlerFunction_Switch fn);

    PicClass& setStream(Stream* stream);

  private:
    int _port;

    THandlerFunction _heartbeat_callback;
    THandlerFunction _reset_callback;
    THandlerFunction_Switch _switch_callback;
//    THandlerFunction_Error _error_callback;
    THandlerFunction_Progress _progress_callback;
    bool package_finish = false;

    unsigned int rx_count = HEAD_FIRST;
    unsigned int rx_value_len;

    unsigned long last;
    unsigned long flash;

    unsigned char tx_buf[128];
    unsigned char rx_buf[WIFI_UART_RECV_BUF_LMT];


    Stream *picStream;
};

extern PicClass Pic;


#endif
