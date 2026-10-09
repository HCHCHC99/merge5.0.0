/**
 * @file led_sequence.c
 * @brief LED序列定义实现 - 全部在ROM中
 */

#include "led_sequence.h"
#include "led_seq_id.h"
#include <string.h>

// ==============================
// 序列项数组定义（全部在ROM中）
// ==============================

// -------- 单色常亮 --------
static const LED_SequenceItem_t items_solid_red[] = {
    {0, LED_COLOR_RED},
};
static const LED_SequenceItem_t items_solid_green[] = {
    {0, LED_COLOR_GREEN},
};
static const LED_SequenceItem_t items_solid_yellow[] = {
    {0, LED_COLOR_YELLOW},
};
static const LED_SequenceItem_t items_solid_black[] = {
    {0, LED_COLOR_BLACK},
};

// -------- 双色常亮 --------
static const LED_SequenceItem_t items_solid_red_yellow[] = {
    {0, LED_COLOR_RED_YELLOW},
};
static const LED_SequenceItem_t items_solid_red_green[] = {
    {0, LED_COLOR_RED_GREEN},
};
static const LED_SequenceItem_t items_solid_green_yellow[] = {
    {0, LED_COLOR_GREEN_YELLOW},
};

//// -------- 呼吸灯 --------
//static const LED_SequenceItem_t items_breath_red[] = {
//   {50,  LED_COLOR_BLACK},
//   {100, LED_COLOR_RED},
//   {100, LED_COLOR_RED},
//   {100, LED_COLOR_RED},
//   {50,  LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_breath_green[] = {
//   {50,  LED_COLOR_BLACK},
//   {100, LED_COLOR_GREEN},
//   {100, LED_COLOR_GREEN},
//   {100, LED_COLOR_GREEN},
//   {50,  LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_breath_yellow[] = {
//   {50,  LED_COLOR_BLACK},
//   {100, LED_COLOR_YELLOW},
//   {100, LED_COLOR_YELLOW},
//   {100, LED_COLOR_YELLOW},
//   {50,  LED_COLOR_BLACK},
//};

//// -------- 标准闪烁 1Hz --------
//static const LED_SequenceItem_t items_blink_red_1hz[] = {
//    {500, LED_COLOR_RED},
//    {500, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_blink_green_1hz[] = {
//    {500, LED_COLOR_GREEN},
//    {500, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_blink_yellow_1hz[] = {
//    {500, LED_COLOR_YELLOW},
//    {500, LED_COLOR_BLACK},
//};

// -------- 标准闪烁 2Hz --------
static const LED_SequenceItem_t items_blink_red_2hz[] = {
    {250, LED_COLOR_RED},
    {250, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_green_2hz[] = {
    {250, LED_COLOR_GREEN},
    {250, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_yellow_2hz[] = {
    {250, LED_COLOR_YELLOW},
    {250, LED_COLOR_BLACK},
};

//// -------- 标准闪烁 5Hz --------
//static const LED_SequenceItem_t items_blink_red_5hz[] = {
//    {100, LED_COLOR_RED},
//    {100, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_blink_green_5hz[] = {
//    {100, LED_COLOR_GREEN},
//    {100, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_blink_yellow_5hz[] = {
//    {100, LED_COLOR_YELLOW},
//    {100, LED_COLOR_BLACK},
//};

// -------- 标准闪烁 10Hz --------
static const LED_SequenceItem_t items_blink_red_10hz[] = {
    {50, LED_COLOR_RED},
    {50, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_green_10hz[] = {
    {50, LED_COLOR_GREEN},
    {50, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_yellow_10hz[] = {
    {50, LED_COLOR_YELLOW},
    {50, LED_COLOR_BLACK},
};

static const LED_SequenceItem_t items_blink_red_yellow_10hz[] = {
    {50, LED_COLOR_RED_YELLOW},
    {50, LED_COLOR_BLACK},
};

// -------- 自定义闪烁 2xHz --------
static const LED_SequenceItem_t items_blink_red_2xhz[] = {
    {200, LED_COLOR_RED},
    {200, LED_COLOR_BLACK},
	{200, LED_COLOR_RED},
	{900, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_green_2xhz[] = {
    {200, LED_COLOR_GREEN},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_GREEN},
    {900, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_yellow_2xhz[] = {
    {200, LED_COLOR_YELLOW},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_YELLOW},
    {900, LED_COLOR_BLACK},
};

// -------- 自定义闪烁 3xHz --------
static const LED_SequenceItem_t items_blink_red_3xhz[] = {
    {200, LED_COLOR_RED},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_RED},
    {200, LED_COLOR_BLACK},
	{200, LED_COLOR_RED},
	{500, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_green_3xhz[] = {
    {200, LED_COLOR_GREEN},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_GREEN},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_GREEN},
    {500, LED_COLOR_BLACK},
};
static const LED_SequenceItem_t items_blink_yellow_3xhz[] = {
    {200, LED_COLOR_YELLOW},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_YELLOW},
    {200, LED_COLOR_BLACK},
    {200, LED_COLOR_YELLOW},
    {500, LED_COLOR_BLACK},
};

//// -------- 双闪 --------
//static const LED_SequenceItem_t items_double_blink_red[] = {
//    {100, LED_COLOR_RED},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_RED},
//    {300, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_double_blink_green[] = {
//    {100, LED_COLOR_GREEN},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_GREEN},
//    {300, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_double_blink_yellow[] = {
//    {100, LED_COLOR_YELLOW},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_YELLOW},
//    {300, LED_COLOR_BLACK},
//};

// -------- 交替闪烁 --------
static const LED_SequenceItem_t items_alt_red_green[] = {
    {250, LED_COLOR_RED},
    {250, LED_COLOR_GREEN},
};
static const LED_SequenceItem_t items_alt_red_yellow[] = {
    {250, LED_COLOR_RED},
    {250, LED_COLOR_YELLOW},
};
static const LED_SequenceItem_t items_alt_green_yellow[] = {
    {250, LED_COLOR_GREEN},
    {250, LED_COLOR_YELLOW},
};

//// -------- 特殊模式 --------
//static const LED_SequenceItem_t items_heartbeat[] = {
//    {50,  LED_COLOR_RED},
//    {50,  LED_COLOR_BLACK},
//    {50,  LED_COLOR_RED},
//    {500, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_police[] = {
//    {200, LED_COLOR_RED},
//    {200, LED_COLOR_BLACK},
//    {200, LED_COLOR_YELLOW},
//    {200, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_sos[] = {
//    // S (短闪3次)
//    {150, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {150, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {150, LED_COLOR_RED},
//    {300, LED_COLOR_BLACK},
//    // O (长闪3次)
//    {450, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {450, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {450, LED_COLOR_RED},
//    {300, LED_COLOR_BLACK},
//    // S (短闪3次)
//    {150, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {150, LED_COLOR_RED},
//    {150, LED_COLOR_BLACK},
//    {150, LED_COLOR_RED},
//    {700, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_rgb_flow[] = {
//    {300, LED_COLOR_RED},
//    {300, LED_COLOR_GREEN},
//    {300, LED_COLOR_YELLOW},
//    {300, LED_COLOR_BLACK},
//};

//// -------- 间隔模式 --------
//static const LED_SequenceItem_t items_interval_2_red[] = {
//    {100, LED_COLOR_RED},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_RED},
//    {900, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_interval_2_green[] = {
//    {100, LED_COLOR_GREEN},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_GREEN},
//    {900, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_interval_3_red[] = {
//    {100, LED_COLOR_RED},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_RED},
//    {500, LED_COLOR_BLACK},
//};
//static const LED_SequenceItem_t items_interval_3_green[] = {
//    {100, LED_COLOR_GREEN},
//    {100, LED_COLOR_BLACK},
//    {100, LED_COLOR_GREEN},
//    {500, LED_COLOR_BLACK},
//};

// ==============================
// 序列定义（全部在ROM中）
// ==============================
static const LED_Sequence_t g_sequences[] = {
    // -------- 单色常亮 --------
    [SEQ_ID_SOLID_RED] = {
        .items = items_solid_red,
        .item_count = sizeof(items_solid_red) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 0,
        .name = "Solid_Red",
    },
    [SEQ_ID_SOLID_GREEN] = {
        .items = items_solid_green,
        .item_count = sizeof(items_solid_green) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 0,
        .name = "Solid_Green",
    },
    [SEQ_ID_SOLID_YELLOW] = {
        .items = items_solid_yellow,
        .item_count = sizeof(items_solid_yellow) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 0,
        .name = "Solid_Yellow",
    },
    [SEQ_ID_SOLID_BLACK] = {
        .items = items_solid_black,
        .item_count = sizeof(items_solid_black) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 0,
        .name = "Solid_Black",
    },

    // -------- 双色常亮 --------
    [SEQ_ID_SOLID_RED_YELLOW] = {
        .items = items_solid_red_yellow,
        .item_count = sizeof(items_solid_red_yellow) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Solid_Red_Yellow",
    },
    [SEQ_ID_SOLID_RED_GREEN] = {
        .items = items_solid_red_green,
        .item_count = sizeof(items_solid_red_green) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Solid_Red_Green",
    },
    [SEQ_ID_SOLID_GREEN_YELLOW] = {
        .items = items_solid_green_yellow,
        .item_count = sizeof(items_solid_green_yellow) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Solid_Green_Yellow",
    },

//   // -------- 呼吸灯 --------
//   [SEQ_ID_BREATH_RED] = {
//       .items = items_breath_red,
//       .item_count = sizeof(items_breath_red) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "Breath_Red",
//   },
//   [SEQ_ID_BREATH_GREEN] = {
//       .items = items_breath_green,
//       .item_count = sizeof(items_breath_green) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "Breath_Green",
//   },
//   [SEQ_ID_BREATH_YELLOW] = {
//       .items = items_breath_yellow,
//       .item_count = sizeof(items_breath_yellow) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "Breath_Yellow",
//   },

//    // -------- 标准闪烁 1Hz --------
//    [SEQ_ID_BLINK_RED_1HZ] = {
//        .items = items_blink_red_1hz,
//        .item_count = sizeof(items_blink_red_1hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 1000,
//        .name = "Blink_Red_1Hz",
//    },
//    [SEQ_ID_BLINK_GREEN_1HZ] = {
//        .items = items_blink_green_1hz,
//        .item_count = sizeof(items_blink_green_1hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 1000,
//        .name = "Blink_Green_1Hz",
//    },
//    [SEQ_ID_BLINK_YELLOW_1HZ] = {
//        .items = items_blink_yellow_1hz,
//        .item_count = sizeof(items_blink_yellow_1hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 1000,
//        .name = "Blink_Yellow_1Hz",
//    },

    // -------- 标准闪烁 2Hz --------
    [SEQ_ID_BLINK_RED_2HZ] = {
        .items = items_blink_red_2hz,
        .item_count = sizeof(items_blink_red_2hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Blink_Red_2Hz",
    },
    [SEQ_ID_BLINK_GREEN_2HZ] = {
        .items = items_blink_green_2hz,
        .item_count = sizeof(items_blink_green_2hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Blink_Green_2Hz",
    },
    [SEQ_ID_BLINK_YELLOW_2HZ] = {
        .items = items_blink_yellow_2hz,
        .item_count = sizeof(items_blink_yellow_2hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Blink_Yellow_2Hz",
    },

//    // -------- 标准闪烁 5Hz --------
//    [SEQ_ID_BLINK_RED_5HZ] = {
//        .items = items_blink_red_5hz,
//        .item_count = sizeof(items_blink_red_5hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 200,
//        .name = "Blink_Red_5Hz",
//    },
//    [SEQ_ID_BLINK_GREEN_5HZ] = {
//        .items = items_blink_green_5hz,
//        .item_count = sizeof(items_blink_green_5hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 200,
//        .name = "Blink_Green_5Hz",
//    },
//    [SEQ_ID_BLINK_YELLOW_5HZ] = {
//        .items = items_blink_yellow_5hz,
//        .item_count = sizeof(items_blink_yellow_5hz) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 200,
//        .name = "Blink_Yellow_5Hz",
//    },

   // -------- 标准闪烁 10Hz --------
    [SEQ_ID_BLINK_RED_10HZ] = {
        .items = items_blink_red_10hz,
        .item_count = sizeof(items_blink_red_10hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Red_10Hz",
    },
    [SEQ_ID_BLINK_GREEN_10HZ] = {
        .items = items_blink_green_10hz,
        .item_count = sizeof(items_blink_green_10hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Green_10Hz",
    },
    [SEQ_ID_BLINK_YELLOW_10HZ] = {
        .items = items_blink_yellow_10hz,
        .item_count = sizeof(items_blink_yellow_10hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Yellow_10Hz",
    },

    [SEQ_ID_BLINK_RED_YELLOW_10HZ] = {
        .items = items_blink_red_yellow_10hz,
        .item_count = sizeof(items_blink_yellow_10hz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Red_Yellow_10Hz",
    },

   // -------- 自定义闪烁 2xHz --------
    [SEQ_ID_BLINK_RED_2xHZ] = {
        .items = items_blink_red_2xhz,
        .item_count = sizeof(items_blink_red_2xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Red_5Hz",
    },
    [SEQ_ID_BLINK_GREEN_2xHZ] = {
        .items = items_blink_green_2xhz,
        .item_count = sizeof(items_blink_green_2xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Green_5Hz",
    },
    [SEQ_ID_BLINK_YELLOW_2xHZ] = {
        .items = items_blink_yellow_2xhz,
        .item_count = sizeof(items_blink_yellow_2xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Yellow_5Hz",
    },

   // -------- 自定义闪烁 3xHz --------
    [SEQ_ID_BLINK_RED_3xHZ] = {
        .items = items_blink_red_3xhz,
        .item_count = sizeof(items_blink_red_3xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Red_5Hz",
    },
    [SEQ_ID_BLINK_GREEN_3xHZ] = {
        .items = items_blink_green_3xhz,
        .item_count = sizeof(items_blink_green_3xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Green_5Hz",
    },
    [SEQ_ID_BLINK_YELLOW_3xHZ] = {
        .items = items_blink_yellow_3xhz,
        .item_count = sizeof(items_blink_yellow_3xhz) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 200,
        .name = "Blink_Yellow_5Hz",
    },

//    // -------- 双闪 --------
//    [SEQ_ID_DOUBLE_BLINK_RED] = {
//        .items = items_double_blink_red,
//        .item_count = sizeof(items_double_blink_red) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Double_Blink_Red",
//    },
//    [SEQ_ID_DOUBLE_BLINK_GREEN] = {
//        .items = items_double_blink_green,
//        .item_count = sizeof(items_double_blink_green) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Double_Blink_Green",
//    },
//    [SEQ_ID_DOUBLE_BLINK_YELLOW] = {
//        .items = items_double_blink_yellow,
//        .item_count = sizeof(items_double_blink_yellow) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Double_Blink_Yellow",
//    },

    // -------- 交替闪烁 --------
    [SEQ_ID_ALT_BLINK_RED_GREEN] = {
        .items = items_alt_red_green,
        .item_count = sizeof(items_alt_red_green) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Alt_Red_Green",
    },
    [SEQ_ID_ALT_BLINK_RED_YELLOW] = {
        .items = items_alt_red_yellow,
        .item_count = sizeof(items_alt_red_yellow) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Alt_Red_Yellow",
    },
    [SEQ_ID_ALT_BLINK_GREEN_YELLOW] = {
        .items = items_alt_green_yellow,
        .item_count = sizeof(items_alt_green_yellow) / sizeof(LED_SequenceItem_t),
        .total_duration_ms = 500,
        .name = "Alt_Green_Yellow",
    },

//   // -------- 特殊模式 --------
//   [SEQ_ID_HEARTBEAT] = {
//       .items = items_heartbeat,
//       .item_count = sizeof(items_heartbeat) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "Heartbeat",
//   },
//   [SEQ_ID_POLICE] = {
//       .items = items_police,
//       .item_count = sizeof(items_police) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "Police",
//   },
//   [SEQ_ID_SOS] = {
//       .items = items_sos,
//       .item_count = sizeof(items_sos) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "SOS",
//   },
//   [SEQ_ID_RGB_FLOW] = {
//       .items = items_rgb_flow,
//       .item_count = sizeof(items_rgb_flow) / sizeof(LED_SequenceItem_t),
//       .total_duration_ms = 0,
//       .name = "RGB_Flow",
//   },
//
//    // -------- 间隔模式 --------
//    [SEQ_ID_INTERVAL_2_RED] = {
//        .items = items_interval_2_red,
//        .item_count = sizeof(items_interval_2_red) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Interval_2_Red",
//    },
//    [SEQ_ID_INTERVAL_2_GREEN] = {
//        .items = items_interval_2_green,
//        .item_count = sizeof(items_interval_2_green) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Interval_2_Green",
//    },
//    [SEQ_ID_INTERVAL_3_RED] = {
//        .items = items_interval_3_red,
//        .item_count = sizeof(items_interval_3_red) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Interval_3_Red",
//    },
//    [SEQ_ID_INTERVAL_3_GREEN] = {
//        .items = items_interval_3_green,
//        .item_count = sizeof(items_interval_3_green) / sizeof(LED_SequenceItem_t),
//        .total_duration_ms = 0,
//        .name = "Interval_3_Green",
//    },
};

// ==============================
// 序列辅助函数
// ==============================
#include "log_rtt.h"
/**
 * @brief 初始化所有序列（计算总时长）
 */
static void sequence_init_all(void)
{
    uint16_t total;

    for (uint8_t i = 0; i < SEQ_ID_COUNT; i++) {
        total = 0;
        for (uint8_t j = 0; j < g_sequences[i].item_count; j++) {
            total += g_sequences[i].items[j].duration_ms;
        }
        // 注意：这里需要强制转换const，因为初始化时计算
        ((LED_Sequence_t*)&g_sequences[i])->total_duration_ms = total;
    }
}

// ==============================
// 公共接口实现
// ==============================

const LED_Sequence_t* LED_GetSequenceByID(LED_SeqID_t id)
{
    if (id >= SEQ_ID_COUNT) return NULL;
    return &g_sequences[id];
}

uint8_t LED_GetSequenceCount(void)
{
    return SEQ_ID_COUNT;
}

const LED_Sequence_t* LED_GetSequenceByIndex(uint8_t index)
{
    if (index >= SEQ_ID_COUNT) return NULL;
    return &g_sequences[index];
}

const char* LED_GetSequenceNameByID(LED_SeqID_t id)
{
    if (id >= SEQ_ID_COUNT) return NULL;
    return g_sequences[id].name;
}

// ==============================
// 模块初始化（由LED_Init调用）
// ==============================
void LED_Seq_Init(void)
{
	// 下列函数为动态计算 单序列的总时长（需要序列配置项为 非常量类型）
	// 若下列函数未执行，则需要在 序列配置项定义时，进行配置序列时长。
//    sequence_init_all();
}
