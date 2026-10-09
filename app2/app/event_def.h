/**
 * @file    event_def.h
 * @brief   全系统事件位定义
 * @note    事件按模块分组，不同事件组位可重复使用
 */
#ifndef __EVENT_DEF_H
#define __EVENT_DEF_H

#include <stdint.h>
#include "sys_config.h"

#if SYS_ENABLE_EVENT_GROUP

#include "led_events.h"

/*--------------------- 系统事件组 事件定义---------------------*/
#define EVT_SYS_INIT_DONE              (1U << 0)    // 系统初始化完成
#define EVT_SYS_FAULT                  (1U << 1)    // 系统故障                           0x0000 0002
#define EVT_SYS_EMERGENCY              (1U << 2)    // 系统急停
#define EVT_SYS_RECOVERY               (1U << 3)    // 系统故障恢复

#define EVT_SYS_VOLT_NORMAL            (1U << 4)    // 电压正常
#define EVT_SYS_VOLT_OVER              (1U << 5)    // 电压过压
#define EVT_SYS_VOLT_UNDER             (1U << 6)    // 电压欠压

// 系统 事件 共用事件定义
#define EVT_SYS_CMD_WORK_ENABLE        (1U << 7)    // 系统工作： 使能                      0x0000 0080
#define EVT_SYS_CMD_AUTO_COMBINE       (1U << 8)    // 系统工作： 自动结合
#define EVT_SYS_CMD_AUTO_SEPARATE      (1U << 9)    // 系统工作： 自动分离
#define EVT_SYS_CMD_CALIB_MODE_1       (1U << 10)    // 系统工作： 标定模式1    标定1号推杆
#define EVT_SYS_CMD_CALIB_MODE_2       (1U << 11)    // 系统工作： 标定模式2    标定2号推杆
#define EVT_SYS_CMD_CALIB_MODE_EXIT    (1U << 12)    // 系统工作： 退出标定模式
#define EVT_SYS_ST_WORK_ERROR          (1U << 13)    // 系统工作： 工作错误

#define EVT_SYS_COMM_ERROR             (1U << 14)   // 通信异常
#define EVT_SYS_STORE_ERROR            (1U << 15)   // 存储异常

/*--------------------- 推杆事件组 事件定义---------------------*/
#define EVT_ACT_WORK_ENABLE            (1U << 0)    // 推杆 工作使能     0x0000 0001
#define EVT_ACT_WORK_DISABLE           (1U << 1)    // 推杆 工作禁止     0x0000 0002

#define EVT_ACT_COMBINE                (1U << 2)    // 推杆 结合         0x0000 0004
#define EVT_ACT_COMBINE_TRY_1          (1U << 3)    // 推杆 结合 尝试1   0x0000 0008
#define EVT_ACT_COMBINE_TRY_2          (1U << 4)    // 推杆 结合 尝试2   0x0000 0010
#define EVT_ACT_COMBINE_BACK           (1U << 5)    // 推杆 结合 回退    0x0000 0040
#define EVT_ACT_COMBINE_SUCCESS        (1U << 6)    // 推杆 结合 成功    0x0000 0080
#define EVT_ACT_COMBINE_FAIL           (1U << 7)    // 推杆 结合 失败    0x0000 0100

#define EVT_ACT_SEPARATE               (1U << 8)    // 推杆 分离          0x0000 0200
#define EVT_ACT_SEPARATE_TRY_1         (1U << 9)    // 推杆 分离 尝试1   0x0000 0400
#define EVT_ACT_SEPARATE_TRY_2         (1U << 10)    // 推杆 分离 尝试2   0x0000 0800
#define EVT_ACT_SEPARATE_BACK          (1U << 11)    // 推杆 分离 回退   0x0000 2000
#define EVT_ACT_SEPARATE_SUCCESS       (1U << 12)    // 推杆 分离 成功   0x0000 4000
#define EVT_ACT_SEPARATE_FAIL          (1U << 13)    // 推杆 分离 失败   0x0000 8000

#define EVT_ACT_HOLD                   (1U << 14)     // 推杆 保持 65536
#define EVT_ACT_ERROR                  (1U << 15)     // 推杆 错误 131072
#define EVT_ACT_RESET                  (1U << 16)     // 推杆 复位 262144

// 超速 使能事件
#define EVT_ACT_OVERSPEED_ENABLE			(1U << 17)
// 超速 失能事件
#define EVT_ACT_OVERSPEED_DISABLE			(1U << 18)
// 超速报警 失能事件
#define EVT_ACT_OVERSPEED_ALARM_DISABLE		(1U << 19)

#define EVT_ACT_HGEAR_COMB_ED      		(1U << 20)      //
#define EVT_ACT_HGEAR_NO_COMB     		(1U << 21)
#define EVT_ACT_HGEAR_COMB_ING     		(1U << 22)
#define EVT_ACT_HGEAR_CANNEL     		(1U << 23)
#define EVT_ACT_HGEAR_COMB_EXIT     	(1U << 24)

/*--------------------- 电机事件组 事件定义---------------------*/
#define EVT_MOT_WORK_ENABLE            (1U << 0)    // 电机 工作使能  0x0000 0001
#define EVT_MOT_WORK_DISABLE           (1U << 1)    // 电机 工作禁止  0x0000 0002

#define EVT_MOT_FORWARD                (1U << 2)    // 电机正转  0x0000 0004
#define EVT_MOT_REVERSE                (1U << 3)    // 电机反转  0x0000 0008
#define EVT_MOT_STOP                   (1U << 4)    // 电机停止  0x0000 0010
#define EVT_MOT_BRAKE                  (1U << 5)    // 电机制动  0x0000 0020

#define EVT_MOT_ERROR                  (1U << 6)    // 推杆错误  0x0000 0040
#define EVT_MOT_RESET                  (1U << 7)    // 推杆 复位  0x0000 0080

// 事件组核心：仅用于跨状态机同步、触发，不传递大量数据
// 系统状态机 → 推杆状态机 事件
#define EVT_SYS_TO_ACTUATOR_ENABLE        (1 << 0)  // 允许推杆运动
#define EVT_SYS_TO_ACTUATOR_DISABLE       (1 << 1)  // 禁止推杆运动
#define EVT_SYS_TO_ACTUATOR_TARGET_POS    (1 << 2)  // 下发目标位置
#define EVT_SYS_TO_ACTUATOR_EMERGENCY     (1 << 3)  // 急停指令
// 推杆状态机 → 电机状态机 事件
#define EVT_ACTUATOR_TO_MOTOR_FORWARD     (1 << 4)  // 电机正转（伸出）
#define EVT_ACTUATOR_TO_MOTOR_REVERSE     (1 << 5)  // 电机反转（收回）
#define EVT_ACTUATOR_TO_MOTOR_STOP        (1 << 6)  // 电机停止
#define EVT_ACTUATOR_TO_MOTOR_BRAKE       (1 << 7)  // 电机制动
#define EVT_ACTUATOR_TO_MOTOR_CUR_LIMIT   (1 << 8)  // 下发电流限制阈值
// 电机状态机 → 推杆/系统状态机 事件
#define EVT_MOTOR_TO_ACTUATOR_CUR_OK      (1 << 9)  // 电流正常
#define EVT_MOTOR_TO_ACTUATOR_CUR_LIMIT   (1 << 10) // 电流已限流
#define EVT_MOTOR_TO_ACTUATOR_BLOCKED     (1 << 11) // 电机堵转
#define EVT_MOTOR_TO_SYS_FAULT            (1 << 12) // 电机故障
// 推杆状态机 → 系统状态机 事件
#define EVT_ACTUATOR_TO_SYS_POS_REACHED   (1 << 13) // 位置到位
#define EVT_ACTUATOR_TO_SYS_POS_ERROR     (1 << 14) // 位置超差
#define EVT_ACTUATOR_TO_SYS_LIMIT         (1 << 15) // 软限位触发

/*--------------------- 按键事件组 ---------------------*/
#define EVT_KEY_ACT1_MANUAL_COMBINE    (1U << 0)    // 推杆1手动结合指令
#define EVT_KEY_ACT1_MANUAL_SEPARATE   (1U << 1)    // 推杆1手动分离指令
#define EVT_KEY_ACT2_MANUAL_COMBINE    (1U << 2)    // 推杆2手动结合指令
#define EVT_KEY_ACT2_MANUAL_SEPARATE   (1U << 3)    // 推杆2手动分离指令
#define EVT_KEY_ENTER_CALIB            (1U << 4)    // 进入标定模式

#define EVT_KEY0_PRESS                 (1U << 5)
#define EVT_KEY0_RELEASE               (1U << 6)
#define EVT_KEY0_LONG_PRESS            (1U << 7)

#define EVT_KEY1_PRESS                 (1U << 8)
#define EVT_KEY1_RELEASE               (1U << 9)
#define EVT_KEY1_LONG_PRESS            (1U << 10)

#define EVT_KEY0_DOUBLE_CLICK          (1U << 11)
#define EVT_KEY1_DOUBLE_CLICK          (1U << 12)
#define EVT_KEY_COMBINE_LONG_PRESS     (1U << 13)  // 组合长按

#define EVT_KEY_MODEL_ENABLE           (1U << 0)
#define EVT_KEY_M1_ENTERING            (1U << 1)
#define EVT_KEY_M1_ENTERED             (1U << 2)
#define EVT_KEY_M1_CANCEL              (1U << 3)

#define EVT_KEY_M1_MOVEOUT             (1U << 4)
#define EVT_KEY_M1_MOVEIN              (1U << 5)

#define EVT_KEY_M1_EXITING             (1U << 6)
#define EVT_KEY_M1_EXITED              (1U << 7)
#define EVT_KEY_M1_CALIB_CANCEL        (1U << 8)

#define EVT_KEY_M2_ENTERING            (1U << 9)
#define EVT_KEY_M2_ENTERED             (1U << 10)
#define EVT_KEY_M2_CANCEL              (1U << 11)

#define EVT_KEY_M2_MOVEOUT             (1U << 12)
#define EVT_KEY_M2_MOVEIN              (1U << 13)

#define EVT_KEY_M2_EXITING             (1U << 14)
#define EVT_KEY_M2_EXITED              (1U << 15)
#define EVT_KEY_M2_CALIB_CANCEL        (1U << 16)

#define EVT_KEY_RESET       			(1U << 17)

//标定事件定义
#define EVT_CALIB_INIT_DONE				(1 << 0)
#define EVT_CALIB_ENABLE				(1 << 1)
#define EVT_CALIB_DISABLE				(1 << 2)
#define EVT_CALIB_INTO_MANU_ING			(1 << 3)//手动标定
#define EVT_CALIB_INTO_MANU_CANCEL		(1 << 4)
#define EVT_CALIB_INTO_MANU				(1 << 5)
#define EVT_CALIB_MANU_OUT				(1 << 6)
#define EVT_CALIB_MANU_IN				(1 << 7)
#define EVT_CALIB_MANU_STOP				(1 << 8)
#define EVT_CALIB_EXIT_MANU_ING			(1 << 9)
#define EVT_CALIB_EXIT_MANU_CANCEL		(1 << 10)
#define EVT_CALIB_EXIT					(1 << 11)
#define EVT_CALIB_AUTO_CALIB_SEP		(1 << 12)//一键自动标定
#define EVT_CALIB_AUTO_CALIB_BACK		(1 << 13)
#define EVT_CALIB_AUTO_DONE				(1 << 14)
#define EVT_CALIB_AUTO_COMB				(1 << 15)
#define EVT_CALIB_AUTO_SEP				(1 << 16)
#define EVT_CALIB_DONE					(1 << 17)//设置当前位置为分离点
//新增
#define EVT_CALIB_INTO_MANU_ED			(1 << 18)//
#define EVT_CALIB_EST					(1 << 19)//

#if 0
/*--------------------- LED事件组 ---------------------*/
// 1 个 LED = 1 个独立事件组 (32bit)
// 每个 LED 事件组内的定义格式完全一致
// 支持无限扩展 LED3/LED4/LED5...

// ==============================
// LED独立事件，对比定义
// ==============================
#define EVT_LEDx_OFF                   (1U << 0)   // 0x00000001
#define EVT_LEDx_RED_ON                (1U << 1)   // 0x00000002
#define EVT_LEDx_GREEN_ON              (1U << 2)   // 0x00000004
#define EVT_LEDx_YELLOW_ON             (1U << 3)   // 0x00000008
#define EVT_LEDx_RED_GREEN_ON          (1U << 4)   // 0x00000010
#define EVT_LEDx_RED_YELLOW_ON         (1U << 5)   // 0x00000020
#define EVT_LEDx_GREEN_YELLOW_ON       (1U << 6)   // 0x00000040
#define EVT_LEDx_ALL_ON                (1U << 7)   // 0x00000080

#define EVT_LEDx_BLINK_LOOP            (1U << 8)   // 0x00000100
#define EVT_LEDx_BLINK_CNT_3           (1U << 9)   // 0x00000200
#define EVT_LEDx_BLINK_CNT_5           (1U << 10)   // 0x00000400
#define EVT_LEDx_BLINK_TIME_2S         (1U << 11)   // 0x00000800
#define EVT_LEDx_BLINK_TIME_5S         (1U << 12)   // 0x00001000
#define EVT_LEDx_BLINK_ALT             (1U << 13)   // 0x00002000
#define EVT_LEDx_BLINK_INTERVAL_2      (1U << 14)   // 0x00004000
#define EVT_LEDx_BLINK_INTERVAL_3      (1U << 15)   // 0x00008000

#define EVT_LEDx_FREQ1                 (1U << 16)   // 0x00010000
#define EVT_LEDx_FREQ2                 (1U << 17)   // 0x00020000
#define EVT_LEDx_FREQ3                 (1U << 18)   // 0x00040000
#define EVT_LEDx_FREQ4                 (1U << 19)   // 0x00080000 //260617_RL_add：EVT_LEDx_BLINK_ALT的频率，红-250；绿-250，中间不灭。

//260613_RL_add:增加控制led灯使能/失能事件
#define EVT_LEDx_DISABLE               (1U << 20)    //0x00100000
#define EVT_LEDx_ENABLE                (1U << 21)    //0x00200000

// ==============================
// LED1 独立事件组 (第1组)
// ==============================
#define EVT_LED1_OFF                   (1U << 0)
#define EVT_LED1_RED_ON                (1U << 1)
#define EVT_LED1_GREEN_ON              (1U << 2)
#define EVT_LED1_YELLOW_ON             (1U << 3)
#define EVT_LED1_RED_GREEN_ON          (1U << 4)
#define EVT_LED1_RED_YELLOW_ON         (1U << 5)
#define EVT_LED1_GREEN_YELLOW_ON       (1U << 6)
#define EVT_LED1_ALL_ON                (1U << 7)

#define EVT_LED1_BLINK_LOOP            (1U << 8)
#define EVT_LED1_BLINK_CNT_3           (1U << 9)
#define EVT_LED1_BLINK_CNT_5           (1U << 10)
#define EVT_LED1_BLINK_TIME_2S         (1U << 11)
#define EVT_LED1_BLINK_TIME_5S         (1U << 12)
#define EVT_LED1_BLINK_ALT             (1U << 13)
#define EVT_LED1_BLINK_INTERVAL_2      (1U << 14)
#define EVT_LED1_BLINK_INTERVAL_3      (1U << 15)

#define EVT_LED1_FREQ1                 (1U << 16)
#define EVT_LED1_FREQ2                 (1U << 17)
#define EVT_LED1_FREQ3                 (1U << 18)
#define EVT_LED1_FREQ4                 (1U << 19)

//260613_RL_add:增加控制led灯使能/失能事件
#define EVT_LED1_DISABLE               (1U << 20)
#define EVT_LED1_ENABLE                (1U << 21)
// ==============================
// LED2 独立事件组 (第2组)
// ==============================
#define EVT_LED2_OFF                   (1U << 0)
#define EVT_LED2_RED_ON                (1U << 1)
#define EVT_LED2_GREEN_ON              (1U << 2)
#define EVT_LED2_YELLOW_ON             (1U << 3)
#define EVT_LED2_RED_GREEN_ON          (1U << 4)
#define EVT_LED2_RED_YELLOW_ON         (1U << 5)
#define EVT_LED2_GREEN_YELLOW_ON       (1U << 6)
#define EVT_LED2_ALL_ON                (1U << 7)

#define EVT_LED2_BLINK_LOOP            (1U << 8)
#define EVT_LED2_BLINK_CNT_3           (1U << 9)
#define EVT_LED2_BLINK_CNT_5           (1U << 10)
#define EVT_LED2_BLINK_TIME_2S         (1U << 11)
#define EVT_LED2_BLINK_TIME_5S         (1U << 12)
#define EVT_LED2_BLINK_ALT             (1U << 13)
#define EVT_LED2_BLINK_INTERVAL_2      (1U << 14)
#define EVT_LED2_BLINK_INTERVAL_3      (1U << 15)

#define EVT_LED2_FREQ1                 (1U << 16)
#define EVT_LED2_FREQ2                 (1U << 17)
#define EVT_LED2_FREQ3                 (1U << 18)
#define EVT_LED2_FREQ4                 (1U << 19)

//260613_RL_add:增加控制led灯使能/失能事件
#define EVT_LED2_DISABLE               (1U << 20)
#define EVT_LED2_ENABLE                (1U << 21)

#endif

// ==============================
// LED3 扩展示例 (第3组)
// ==============================
#if 0
#define EVT_LED3_OFF                   (1U << 0)
#define EVT_LED3_RED_ON                (1U << 1)
...
#endif

#endif

#endif
