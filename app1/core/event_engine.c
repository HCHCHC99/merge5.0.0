///**
// * @file    event_engine.c
// * @brief   事件组引擎实现（多实例、静态池、线程安全简化版）
// */

//#include "event_engine.h"
//#include <string.h>

//#if SYS_ENABLE_EVENT_GROUP

//// 静态事件组池
//static EventGroup_t s_event_pool[MAX_EVENT_GROUPS] = {0};

///**
// * @brief 创建事件组
// */
//EventGroup_t* Event_CreateGroup(void)
//{
//    for (int i = 0; i < MAX_EVENT_GROUPS; i++)
//    {
//        if (!s_event_pool[i].valid)
//        {
//            memset(&s_event_pool[i], 0, sizeof(EventGroup_t));
//            s_event_pool[i].valid = 1;
//            return &s_event_pool[i];
//        }
//    }
//    return NULL;
//}

///**
// * @brief 发送事件
// */
//void Event_Send(EventGroup_t *group, EventBit_t bits)
//{
//    if (group == NULL || !group->valid) return;
//    group->bits |= bits;
//}

///**
// * @brief 清除指定事件
// */
//void Event_Clear(EventGroup_t *group, EventBit_t bits)
//{
//    if (group == NULL || !group->valid) return;
//    group->bits &= ~bits;
//}

///**
// * @brief 清除所有事件
// */
//void Event_ClearAll(EventGroup_t *group)
//{
//    if (group == NULL || !group->valid) return;
//    group->bits = 0;
//}

///**
// * @brief 获取事件位
// */
//EventBit_t Event_Get(EventGroup_t *group)
//{
//    if (group == NULL || !group->valid) return 0;
//    return group->bits;
//}

///**
// * @brief 检查事件是否触发
// */
//uint8_t Event_Check(EventGroup_t *group, EventBit_t bits)
//{
//    if (group == NULL || !group->valid) return 0;
//    return (group->bits & bits) ? 1 : 0;
//}

//#endif
