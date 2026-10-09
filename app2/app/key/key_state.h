// /**
//  * @file    key_state.h
//  * @brief   推杆状态机接口
//  */
// #ifndef __KEY_STATE_H
// #define __KEY_STATE_H

// #include "axis_typedef.h"

// /**
//  * @brief  推杆状态机初始化
//  * @param  sm: 状态机指针
//  * @param  evt: 事件组指针
//  */
// void Act_State_Init(StateMachine_t *sm);

// /**
//  * @brief  推杆状态机运行
//  */
// void Act_State_Task(void);


// #endif


/**
 * @file    key_state.h
 * @brief   推杆按键标定状态机对外接口
 */
#ifndef __KEY_STATE_H
#define __KEY_STATE_H

#include "axis_typedef.h"
#include "msg_pubsub.h"
#include "event_def.h"
#include "soft_timer.h"
#include "key.h"

/* 标定长按阈值 5000ms，统一使用软定时器定时 */
#define KEY_CALIB_HOLD_MS       5000U

///**
// * @brief 按键状态枚举（与源文件统一对外声明）
// */
//typedef enum {
//    KEY_STATE_INIT = 0,                   // 初始化状态
//    KEY_STATE_IDLE ,                      // 空闲状态
//   
//    KEY_STATE_CALIB_M1_ENTERTING,         // M1 标定进入中
//    KEY_STATE_CALIB_M1_IDLE,              // M1 标定 空闲状态
//    KEY_STATE_CALIB_M1_MOVEOUT,           // M1 标定 伸出状态
//    KEY_STATE_CALIB_M1_MOVEIN,            // M1 标定 缩回状态
//    KEY_STATE_CALIB_M1_EXITING,           // M1 标定退出中

//    KEY_STATE_CALIB_M2_ENTERTING,         // M2 标定进入中
//    KEY_STATE_CALIB_M2_IDLE,              // M2 标定 空闲状态
//    KEY_STATE_CALIB_M2_MOVEOUT,           // M2 标定 伸出状态
//    KEY_STATE_CALIB_M2_MOVEIN,            // M2 标定 缩回状态
//    KEY_STATE_CALIB_M2_EXITING,           // M2 标定退出中
//} KeyState_t;

/**
 * @brief  按键消息上报结构体（和key.c上报结构对齐）
 */
//typedef enum {
//    KEY_EVENT_NONE    = 0U,
//    KEY_EVENT_CLICK   = 1U,
//    KEY_EVENT_LONG    = 2U,
//    KEY_EVENT_RELEASE = 3U,
//} Key_Event_t;

//typedef struct {
//    uint32_t key_code;
//    const char *key_name;
//    Key_Event_t event;
//    uint32_t timestamp;
//} Key_Report_t;

/**
 * @brief  按键状态机初始化
 * @param  sm: 状态机句柄指针
 */
void Key_State_Init(void);

/**
 * @brief  按键状态机周期任务（10ms级周期调用）
 */
void Key_State_Task(void);

/**
 * @brief 系统状态 - 位置信息回调处理函数
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的位置状态数据
 */
void State_Key_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio);

//key状态机复位
void key_state_reset(void);

#endif
