/**
 * @file led_sequence.h
 * @brief LED序列定义 - 编译时固化在ROM
 */

#ifndef __LED_SEQUENCE_H__
#define __LED_SEQUENCE_H__

#include "led.h"
#include "led_seq_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// 序列注册表（外部访问）
// ==============================

/**
 * @brief 根据序列ID获取序列指针
 * @param id 序列ID
 * @return 序列指针，无效返回NULL
 */
const LED_Sequence_t* LED_GetSequenceByID(LED_SeqID_t id);

/**
 * @brief 获取序列总数
 * @return 序列数量
 */
uint8_t LED_GetSequenceCount(void);

/**
 * @brief 根据索引获取序列指针
 * @param index 序列索引
 * @return 序列指针，无效返回NULL
 */
const LED_Sequence_t* LED_GetSequenceByIndex(uint8_t index);

/**
 * @brief 获取序列ID对应的名称
 * @param id 序列ID
 * @return 序列名称字符串
 */
const char* LED_GetSequenceNameByID(LED_SeqID_t id);

#ifdef __cplusplus
}
#endif

#endif /* __LED_SEQUENCE_H__ */

