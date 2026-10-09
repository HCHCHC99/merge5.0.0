#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

// ########################### 系统总开关 ###########################
#define SYS_ENABLE_SCHEDULER                   1     // 调度器总开关
#define SYS_ENABLE_EVENT_GROUP                 1     // 事件组开关
#define SYS_ENABLE_MSG_PUBSUB                  1     // 消息发布订阅开关
#define SYS_ENABLE_CONFIG                      1     // 系统配置管理
#define SYS_ENABLE_STORAGE                     1     // 数据存储
#define SYS_ENABLE_POWER                       1     // 电源模块
#define SYS_ENABLE_PWM						   1     // pwm模块
#define SYS_ENABLE_CAN						   1     // can模块
#define SYS_ENABLE_UDS                       1     // UDS 诊断协议栈
#define SYS_ENABLE_STATE                       1     // 状态机模块
#define SYS_ENABLE_KEY                         1     // key 模块
#define SYS_ENABLE_LED                         1     // led 模块
#define SYS_ENABLE_POS                         1     // 位置 模块
#define SYS_ENABLE_WORK_CGF                    1     // 机型配置 模块         //260613_RL_add

// ########################### 服务开关 ###########################
#define SYS_ENABLE_TICK_1MS                    1     // 1ms 系统时钟（必须开启）
#define SYS_ENABLE_LOG_RTT                     1     // RTT日志开关
#define SYS_ENABLE_SOFT_TIMER                  1     // 软件定时器模块开关

// ########################### 应用功能开关 ###########################
#define APP_ENABLE_ACTUATOR_CTRL               0     // 电推杆控制
#define APP_ENABLE_FAULT_MANAGE                0     // 故障管理
#define APP_ENABLE_POSITION_LOOP               0     // 位置环
#define APP_ENABLE_CURRENT_LOOP                0     // 电流环 ( 使能时，采用独立电流环，不采用 pwm触发电流采样)

// ########################### PID 三环开关 ###########################
#define APP_ENABLE_CTRL_POSITION               1     // 位置环
#define APP_ENABLE_CTRL_SPEED                  1     // 速度环
#define APP_ENABLE_CTRL_CURRENT                1     // 电流环

// ########################### 位置传感器裁剪开关 ###########################
#define SYS_ENABLE_POS_SENSOR                  1     // 位置传感器总开关
#define SYS_ENABLE_SENSOR_POT                  1     // 电位计（当前使用）
#define SYS_ENABLE_SENSOR_HALL                 0     // 电机霍尔
#define SYS_ENABLE_SENSOR_INC_ENC              0     // 增量编码器
#define SYS_ENABLE_SENSOR_ABS_ENC              0     // 绝对编码器

// ########################### 驱动裁剪开关 ###########################
#define HAL_ENABLE_GPIO                        1
#define HAL_ENABLE_PWM                         1
#define HAL_ENABLE_ADC                         1
#define HAL_ENABLE_TIMER                       1
#define HAL_ENABLE_UART                        0

// ########################### 优先级定义（数字越小优先级越高） ###########################
#define SYS_PRIO_HIGHEST                       0     // 故障、急停
#define SYS_PRIO_HIGH                          1     // 位置环、电流环、推杆控制
#define SYS_PRIO_MID                           2     // 消息、事件
#define SYS_PRIO_LOW                           3     // 状态机
#define SYS_PRIO_LOWEST                        4     // 日志、后台

// ########################### 调度周期配置 ###########################
#define SYS_TICK_PERIOD_MS                     1

// ########################### 消息/事件配置 ###########################
#define MSG_PRIO_HIGH                          0
#define MSG_PRIO_MID                           1
#define MSG_PRIO_LOW                           2

//#define MAX_MSG_SUBSCRIBE                      32    // 最大订阅数

// #define MAX_EVENT_GROUPS                       8     // 最大事件组数

typedef enum {
    EVENT_GROUPS_SYS    = 0U,     /* 系统事件组 */
    EVENT_GROUPS_KEY,             /* 按键事件组 */
    EVENT_GROUPS_LED1,            /* LED1事件组 */
    EVENT_GROUPS_LED2,            /* LED2事件组 */
    MAX_EVENT_GROUPS  = 0x20              /* 最大事件组数 */
} Event_Groups_Name_t;

// ########################### 推杆执行器相关裁剪开关 ###########################
#define SYS_ENABLE_ACTUATOR_SYSTEM             1     // 系统状态机总开关
#define SYS_ENABLE_ACTUATOR                    1     // 推杆状态机总开关
#define SYS_ENABLE_MOTOR                       1     // 电机状态机总开关
#define SYS_ENABLE_POSITION_LOOP               1     // 位置环开关（防过冲）
#define SYS_ENABLE_CURRENT_LOOP                1     // 电流环开关（限流）
#define SYS_ENABLE_BLOCK_DETECTION             1     // 堵转检测开关
#define SYS_ENABLE_ACTUATOR_SOFT_LIMIT         1     // 推杆软限位开关

// ########################### 故障码定义 ###########################
#define SYS_FAULT_NONE                         0x00  // 无故障
#define SYS_FAULT_MOTOR_BLOCKED                0x01  // 电机堵转
#define SYS_FAULT_POS_OVER_ERROR               0x02  // 位置超差
#define SYS_FAULT_CURRENT_OVERLOAD             0x03  // 电流过载
#define SYS_FAULT_POWER_UNDERVOLTAGE           0x04  // 电源欠压
#define SYS_FAULT_SOFT_LIMIT                   0x05  // 软限位触发
#define SYS_FAULT_EMERGENCY_STOP               0x06  // 急停触发

#endif
