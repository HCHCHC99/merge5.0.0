/*******************************************
* ???: isotp_transport.h
* ??: AI Assistant
* ??: V1.0.0
* ??: ISO 15765-2 ???????
* ??: ?????????????
*******************************************/
#ifndef ISOTP_TRANSPORT_H_
#define ISOTP_TRANSPORT_H_

/***************************** ???? *********************************/
#include "stdint.h"
#include "stdbool.h"

/***************************** ????? *********************************/
// #define ISO_DEBUG
#ifdef ISO_DEBUG
    #define ISOTP_D(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_DEBUG, COLOR_CYAN,   "ISOTP", fmt, ##__VA_ARGS__)
    #define ISOTP_I(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_INFO,  COLOR_GREEN, "ISOTP", fmt, ##__VA_ARGS__)
    #define ISOTP_W(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_WARN,  COLOR_YELLOW,"ISOTP", fmt, ##__VA_ARGS__)
    #define ISOTP_E(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_ERROR, COLOR_RED,   "ISOTP", fmt, ##__VA_ARGS__)
#else
    #define ISOTP_D(fmt, ...)  (void)0
    #define ISOTP_I(fmt, ...)  (void)0
    #define ISOTP_W(fmt, ...)  (void)0
    #define ISOTP_E(fmt, ...)  (void)0
#endif

/* OTA ????(????? CAN ID ?,???) */
#define OTA_DEBUG
#ifdef OTA_DEBUG
    #define OTA_D(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_DEBUG, COLOR_CYAN,   "OTA", fmt, ##__VA_ARGS__)
    #define OTA_I(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_INFO,  COLOR_GREEN, "OTA", fmt, ##__VA_ARGS__)
    #define OTA_W(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_WARN,  COLOR_YELLOW,"OTA", fmt, ##__VA_ARGS__)
    #define OTA_E(fmt, ...)  LOG_CH(LOG_CH_MAIN, LOG_LEVEL_ERROR, COLOR_RED,   "OTA", fmt, ##__VA_ARGS__)
#else
    #define OTA_D(fmt, ...)  (void)0
    #define OTA_I(fmt, ...)  (void)0
    #define OTA_W(fmt, ...)  (void)0
    #define OTA_E(fmt, ...)  (void)0
#endif

/***************************** CAN ID ?????? ****************************/
/* ???? CAN ID ??????
 * 0: ??
 * 1: ??
 */
#define ISOTP_ENABLE_FILTER_RECORD    1

/* ???????(???,???? ISO-TP ??) */
// #define ISOTP_AUTO_FC

/* ??? CAN ID ??(?????) */
#define ISOTP_FILTER_CAN_ID_COUNT     2
#define ISOTP_FILTER_CAN_ID_LIST      {0x18FF3AFD, 0x18FF1F11}

/* ?????????(?????)*/
#define ISOTP_FILTER_BUFFER_SIZE      64

/****************************** ??? ***********************************/

/* ISO-TP ????? */
#define ISOTP_FRAME_SINGLE           0x00    /* ?? (SF) */
#define ISOTP_FRAME_FIRST            0x10    /* ?? (FF) */
#define ISOTP_FRAME_CONSECUTIVE      0x20    /* ??? (CF) */
#define ISOTP_FRAME_FLOW_CONTROL     0x30    /* ??? (FC) */

/* ???? (Flow Status) */
#define ISOTP_FC_CTS                 0x00    /* Continue to Send, ???? */
#define ISOTP_FC_WAIT                0x01    /* Wait, ?? */
#define ISOTP_FC_OVERFLOW            0x02    /* Overflow, ?? */

/* ??? */
#define ISOTP_OK                     0       /* ?? */
#define ISOTP_BUSY                   1       /* ?(???) */
#define ISOTP_ERROR                 -1       /* ?? */
#define ISOTP_TIMEOUT               -2       /* ?? */
#define ISOTP_INCOMPLETE            -3       /* ???(??????) */

/* ???? */
#define ISOTP_BUFFER_SIZE            8192    /* ??????? (8KB) */
#define ISOTP_DEFAULT_RESPONSE_ID   0x18FF1F11
#define ISOTP_RX_TIMEOUT_MS          65535   /* ?????? (ms) */

/* ???? (????) */
#define ISOTP_DEFAULT_BLOCK_SIZE     0       /* BS = 0 (???????) */
#define ISOTP_DEFAULT_ST_MIN_MS      5       /* STmin = 5ms */

/* ????????? */
#define ISOTP_MAX_MESSAGE_LEN        (4095)  /* ISO-TP ?????? 4095 ?? */

/************************** ????????? ***************************/

/* ISO-TP ????? */
typedef enum
{
    ISOTP_RX_IDLE = 0,          /* ??,???? */
    ISOTP_RX_ACTIVE,            /* ???,??????? */
    ISOTP_RX_WAIT_FC,           /* ????:????? */
    ISOTP_RX_COMPLETE,          /* ???? */
    ISOTP_RX_TIMEOUT            /* ???? */
} isotp_rx_state_t;

/* ISO-TP ????? */
typedef enum
{
    ISOTP_TX_IDLE = 0,          /* ?? */
    ISOTP_TX_SENDING_FF,        /* ?????,?????????? */
    ISOTP_TX_SENDING_CF,        /* ?????? */
    ISOTP_TX_COMPLETE,          /* ???? */
    ISOTP_TX_TIMEOUT            /* ???? */
} isotp_tx_state_t;

/* ISO-TP ????? (???,?????) */
typedef struct
{
    /* ???? */
    isotp_rx_state_t rx_state;          /* ???? */
    uint32_t rx_src_id;                 /* ??? CAN ID (???) */
    uint32_t rx_dst_id;                 /* ???? CAN ID (???) */
    uint8_t* rx_buffer;                 /* ??????? */
    uint16_t rx_total_len;              /* ????? */
    uint16_t rx_received_len;           /* ????? */
    uint8_t rx_expected_seq;            /* ??????????? (1-15) */
    uint8_t rx_cf_count_in_block;       /* ???????????? */
    uint16_t rx_timeout_counter;        /* ??????? (ms) */
    
    /* ???? */
    isotp_tx_state_t tx_state;          /* ???? */
    uint32_t tx_dst_id;                 /* ???? CAN ID */
    uint8_t* tx_buffer;                 /* ??????? */
    uint16_t tx_total_len;              /* ??????? */
    uint16_t tx_sent_len;               /* ????? */
    uint8_t tx_seq;                     /* ???????? */
    uint8_t tx_cf_count_in_block;       /* ???????????? */
    uint16_t tx_timeout_counter;        /* ??????? (ms) */
    uint8_t tx_bs;                      /* ???????? */
    uint8_t tx_st_min;                  /* ??????????? */
    uint16_t tx_st_min_counter;         /* STmin ????? */
    
    /* ???? */
    uint8_t local_bs;                   /* ????? (???????) */
    uint8_t local_st_min;               /* ???????? (???????) */
    uint16_t timeout_ms;                /* ???? */
    
    /* ???? */
    uint8_t channel;                    /* CAN ??? */
} isotp_connection_t;

/***************************** ???? **********************************/

/* ??? ISO-TP ? */
void isotp_init(uint8_t channel);

/* 1ms ??????? (??? 1ms ?????) */
void isotp_ms_update(void);

/* ?? CAN ? (? CAN ???????) */
int8_t isotp_receive_frame(uint8_t channel, uint32_t can_id, uint8_t* frame_data, 
                            uint8_t frame_len, uint8_t* out_data, uint16_t* out_len);

/* ?????? (????? FF/CF) */
int8_t isotp_send_message(uint8_t channel, uint32_t dst_id, uint8_t* data, uint16_t len);

/* ?????? (?????????,???????) */
void isotp_tx_process(void);

/* ?????? (??????) */
void isotp_reset_rx(void);

/* ?????? */
void isotp_reset_tx(void);

/* ???????? */
isotp_rx_state_t isotp_get_rx_state(void);

/* ???????? */
isotp_tx_state_t isotp_get_tx_state(void);

/* ????????? (? isotp_receive_frame ???) */
void isotp_handle_flow_control(uint8_t flow_status, uint8_t block_size, uint8_t st_min);

/* ==================== CAN ID ???????? ==================== */
#if (ISOTP_ENABLE_FILTER_RECORD == 1)
/* ??????? */
uint32_t isotp_get_filter_record_count(void);

/* ??????? CAN ID */
uint32_t isotp_get_last_filtered_can_id(void);

/* ????????? */
void isotp_get_last_filtered_data(uint8_t* out_data, uint8_t* out_len);

/* ?????????(0=??,1=??...)*/
bool isotp_get_filter_record(uint16_t index, uint32_t* can_id, uint8_t* data, uint8_t* len);
#endif

#endif /* ISOTP_TRANSPORT_H_ */
