/*******************************************
* ???: uds_ota.c
* ??: AI Assistant
* ??: V1.0.0
* ??: UDS OTA ???????
* ??: ? main.c ?? ISOTP_RxCallback / ISOTP_RegisterRxFilters /
*       g_delayed_reset_ms / s_uds_rx_buffer,????
*******************************************/
#include "uds_ota.h"
#include "adapter_can.h"
#include "isotp_transport.h"
#include "uds_diagnostic.h"
#include "flash_download.h"
#include "rtt_log.h"
#include "sys_tick.h"
#include "bootloader_app.h"
#include "bootloader_app.h"

/***************************** ???? ***********************************/
extern void uds_dl_init_fw(void);

/***************************** ???? ***********************************/

/* ???????:UDS handler ?? DELAYED_RESET_MS,Poll ?? ms ?? */
volatile uint32_t g_delayed_reset_ms = 0;
volatile uint8_t  g_force_ota_cmd = 0;
volatile uint8_t  g_swdt_feed_disable = 0;

/***************************** ???? ***********************************/

/* ISOTP ???? UDS ???????(?? 4096 ??) */
static uint8_t s_uds_rx_buffer[4100];

/***************************** ???? ***********************************/

/*
 * ISOTP CAN RX ?? - ? CanIf ?????????????
 * ? CAN ??? ISOTP ??,?????? UDS ????
 */
static void ISOTP_RxCallback(const CanMsg_t *pMsg)
{
    uint16_t out_len = 0;
    int8_t result = isotp_receive_frame(0, pMsg->u32ID,
                                        (uint8_t*)pMsg->au8Data, pMsg->u8DLC,
                                        s_uds_rx_buffer, &out_len);
    if (result == ISOTP_OK) {
        uds_receive_handler(0, pMsg->u32ID, s_uds_rx_buffer, out_len);
    }
}

/*
 * ?? ISOTP ??? 1 ? CAN ID ? CanIf ????
 * ISOTP ????: 0x18FF3AFD
 */
static void ISOTP_RegisterRxFilters(void)
{
    static const uint32_t s_isotp_can_ids[1] = {
        0x18FF3AFDUL,  /* ?????? ID (TBOX ? ???) */
    };

    CanIf_RxFilterEntry_t stcEntry;
    stcEntry.u32CanId   = 0UL;
    stcEntry.u32CanMask = 0UL;  /* ???? */
    stcEntry.u8Format   = CAN_ID_EXT;
    stcEntry.pfnCallback = &ISOTP_RxCallback;

    for (uint8_t i = 0U; i < 1U; i++) {
        stcEntry.u32CanId = s_isotp_can_ids[i];
        if (!CanIf_RegisterRxFilter(&stcEntry)) {
            MAIN_D("ISOTP: failed to register RX filter for CAN ID 0x%08X\r\n",
                   s_isotp_can_ids[i]);
        }
    }
    MAIN_D("ISOTP: 1 CAN ID RX filters registered\r\n");
}

/***************************** ?????? *******************************/

/* ??? UDS/CAN/ISOTP ? */
void UdsOta_Init(void)
{
    MAIN_D("=== UDS Stack Init Start ===\r\n");

    isotp_init(0);
    ISOTP_RegisterRxFilters();
    FlashDownload_Init(NULL);
    uds_dl_init_fw();
    uds_init();

    MAIN_D("=== UDS Stack Init Done ===\r\n");
}

/* ????? */
void UdsOta_Poll(void)
{
    static uint64_t s_last_ms_tick = 0;
    uint64_t current_tick = SysTick_GetTick();
    /* ??2/3: ??OTA?? ? ?????? bootloader(0x01/0x02/0x03 ?? boot ? 50ms ????) */
    if (g_force_ota_cmd != 0U) {
        uint8_t u8ForceCmd = g_force_ota_cmd;
        g_force_ota_cmd = 0U;
        MAIN_D("Force OTA cmd 0x18FF5858 = 0x%02X, resetting to bootloader...\r\n", (unsigned int)u8ForceCmd);
        NVIC_SystemReset();
        while (1) { }
    }

    /* 1ms ?? */
    if (current_tick != s_last_ms_tick) {
        s_last_ms_tick = current_tick;

        /* ??????? */
        if (g_delayed_reset_ms > 0) {
            g_delayed_reset_ms--;
            if (g_delayed_reset_ms == 0) {
                MAIN_D("Delayed reset done, resetting...\r\n");
                NVIC_SystemReset();
                while(1);
            }
        }

        isotp_ms_update();
        uds_ms_update();
        isotp_tx_process();
    }

    FlashDownload_Task();
    CanIf_Poll();
}

/* APP ??????????? UDS ?? */
void UdsOta_App_CheckPendingAck(void)
{
    App_CheckPendingUdsAck();
}

/* Bootloader ?? UDS ???? */
void UdsOta_Bootloader_Enter(void) { /* not used in APP-only */ }
