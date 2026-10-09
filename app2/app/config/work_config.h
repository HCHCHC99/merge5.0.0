// #ifndef __WORK_CONFIG_H
// #define __WORK_CONFIG_H

// #include <stdbool.h>
// #include <stdint.h>

// #define W_AXIS_NUM_MAX   (2)
// #define W_MODEL_NUM_MAX  (256)

// // 查表返回码枚举
// typedef enum
// {
//     FIND_SUCCESS = 0,
//     FIND_FAIL    = 1
// }FindRet_t;

// // 单推杆2bit逻辑枚举，避免硬编码数字
// typedef enum
// {
//     LOGIC_NONE    = 0x00,  // 00 01：无推杆
//     LOGIC_A       = 0x02,  // 10 结合=伸出，分离=缩回
//     LOGIC_B       = 0x03   // 11 分离=伸出，结合=缩回
// }Logic_t;

// // 单根推杆2bit逻辑单元
// typedef struct
// {
//     uint8_t val : 2; // 2bit推杆逻辑：00/01无推杆，10伸出，11缩回
// }Axis_Logic_Bit_t;

// // 整体工作逻辑位域结构体
// typedef struct
// {
//     uint8_t car_id     : 8;        // 车型序号
//     uint8_t logic_flag : 4;        // 逻辑标记 4bit
//     Axis_Logic_Bit_t axis[W_AXIS_NUM_MAX]; // axis[0]前桥，axis[1]后桥，各占2bit
// //    uint16_t reserve ;
// }Work_Logic_t;

// typedef union
// {
//     uint16_t work_config;
//     Work_Logic_t    bits;
// }Work_Config_t;

// // 修正初始化宏：支持子结构体赋值，无多余参数
// #define AXIS_CFG(cid, flag, ax0_val, ax1_val) \
// { \
//     .work_config = 0, \
//     .bits = { \
//         .car_id = (cid), \
//         .logic_flag = (flag), \
//         .axis[0] = {.val = (ax0_val)}, \
//         .axis[1] = {.val = (ax1_val)} \
//     } \
// }

// // 外部常量表声明
// extern const Work_Config_t WORK_CONFIG_TABLE[];
// extern const uint16_t WORK_CONFIG_TABLE_CNT;

// // 函数声明
// void work_config_init(void);
// FindRet_t WorkConfig_Find(uint8_t car_id, uint8_t logic_flag, uint16_t *table_idx);

// //// 编译期静态断言，防止配置溢出
// //#define STATIC_ASSERT(expr) typedef char assert_failed[(expr)?1:-1]
// //STATIC_ASSERT(sizeof(Work_Logic_t) == 2); // 必须占用2字节
// //STATIC_ASSERT(W_AXIS_NUM_MAX <= 2);      // 匹配位域4bit总空间

// #endif

#ifndef __WORK_CONFIG_H
#define __WORK_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define W_AXIS_NUM_MAX   (2)
#define W_MODEL_NUM_MAX  (256)

// 新增：推杆行程硬件范围（全局统一，flash模块引用）
#define AXIS_POS_MIN     0U
#define AXIS_POS_MAX     10000U    // 根据实际采样量程修改
#define AXIS_COMBINE_OFFSET_MM 22U // 分离/结合固定差值20mm

// 查表返回码枚举
typedef enum
{
    FIND_SUCCESS = 0,
    FIND_FAIL    = 1
}FindRet_t;

// 单推杆2bit逻辑枚举，避免硬编码数字
typedef enum
{
    LOGIC_NONE    = 0x00,  // 00 01：无推杆
    LOGIC_A       = 0x02,  // 10 结合=伸出，分离=缩回
    LOGIC_B       = 0x03   // 11 分离=伸出，结合=缩回
}Logic_t;

// 单根推杆2bit逻辑单元
typedef struct
{
    uint8_t val : 2; // 2bit推杆逻辑：00/01无推杆，10伸出，11缩回
}Axis_Logic_Bit_t;

// 整体工作逻辑位域结构体
typedef struct
{
    uint8_t car_id     : 8;        // 车型序号
    uint8_t logic_flag : 4;        // 逻辑标记 4bit
    Axis_Logic_Bit_t axis[W_AXIS_NUM_MAX]; // axis[0]前桥，axis[1]后桥，各占2bit
}Work_Logic_t;

typedef union
{
    uint16_t work_config;
    Work_Logic_t    bits;
}Work_Config_t;

// 修正初始化宏：支持子结构体赋值，无多余参数
#define AXIS_CFG(cid, flag, ax0_val, ax1_val) \
{ \
    .work_config = 0, \
    .bits = { \
        .car_id = (cid), \
        .logic_flag = (flag), \
        .axis[0] = {.val = (ax0_val)}, \
        .axis[1] = {.val = (ax1_val)} \
    } \
}

// 外部常量表声明
extern const Work_Config_t WORK_CONFIG_TABLE[];
extern const uint16_t WORK_CONFIG_TABLE_CNT;

// 函数声明
void work_config_init(void);
FindRet_t WorkConfig_Find(uint8_t car_id, uint8_t logic_flag, uint16_t *table_idx);
/**
 * @brief 系统工作模式更改
 * @note 系统运行中，成功标定，那么更改系统工作模式，并存储
 */
void work_config_modify(uint8_t car_id, uint8_t logic_flag);
#endif
