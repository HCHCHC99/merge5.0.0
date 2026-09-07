#include "msg_pubsub.h"
#include <string.h>
#include "log_rtt.h"

#if SYS_ENABLE_MSG_PUBSUB

#define MAX_MSG_TOPIC        32
#define MAX_MSG_SUBSCRIBE    16

// 订阅节点
typedef struct {
    uint32_t        topic;
    MsgCallback_t   callback;
    uint8_t         valid;  // 有效标志（0=无效，1=有效）
} MsgSubscribe_t;

// 消息系统控制块
static struct {
    MsgSubscribe_t  list[MAX_MSG_SUBSCRIBE];
    uint16_t        cnt;
} msg_cb;

// ------------------------------
// 初始化
// ------------------------------
void Msg_Init(void)
{
    msg_cb.cnt = 0;
    memset(&msg_cb, 0, sizeof(msg_cb));
    LOG_INFO("Msg PubSub init OK, max subscribe: %d", MAX_MSG_SUBSCRIBE);
}

// ------------------------------
// 检查是否已订阅
// ------------------------------
static uint8_t Msg_IsSubscribed(uint32_t topic, MsgCallback_t callback)
{
    for (uint16_t i = 0; i < MAX_MSG_SUBSCRIBE; i++) {
        if (msg_cb.list[i].valid &&
            msg_cb.list[i].topic == topic &&
            msg_cb.list[i].callback == callback) {
            return 1;
        }
    }
    return 0;
}

// ------------------------------
// 订阅（带可靠性检查+重复订阅防护）
// ------------------------------
uint8_t Msg_Subscribe(uint32_t topic, MsgCallback_t callback)
{
    // 可靠性检查 1：回调为空 → 不允许订阅
    if (callback == NULL) {
        LOG_ERROR("Msg Subscribe: callback is NULL (topic:0x%04X)", topic);
        return 1;
    }

    // 可靠性检查 2：重复订阅 → 拒绝
    if (Msg_IsSubscribed(topic, callback)) {
        LOG_WARN("Msg Subscribe: duplicate subscribe (topic:0x%04X)", topic);
        return 1;
    }

    // 可靠性检查 3：订阅表已满 → 拒绝
    if (msg_cb.cnt >= MAX_MSG_SUBSCRIBE) {
        LOG_ERROR("Msg Subscribe: subscribe list full (topic:0x%04X)", topic);
        return 1;
    }

    // 找空闲节点加入订阅列表
    for (uint16_t i = 0; i < MAX_MSG_SUBSCRIBE; i++) {
        if (!msg_cb.list[i].valid) {
            msg_cb.list[i].topic = topic;
            msg_cb.list[i].callback = callback;
            msg_cb.list[i].valid = 1;
            msg_cb.cnt++;
            LOG_INFO("Msg Subscribe: topic 0x%04X OK (total:%d)", topic, msg_cb.cnt);
            return 0;
        }
    }

    return 1;
}

// ------------------------------
// 取消订阅
// ------------------------------
uint8_t Msg_Unsubscribe(uint32_t topic, MsgCallback_t callback)
{
    for (uint16_t i = 0; i < MAX_MSG_SUBSCRIBE; i++) {
        if (msg_cb.list[i].valid &&
            msg_cb.list[i].topic == topic &&
            msg_cb.list[i].callback == callback) {
            msg_cb.list[i].valid = 0;
            msg_cb.cnt--;
            LOG_INFO("Msg Unsubscribe: topic 0x%04X OK (total:%d)", topic, msg_cb.cnt);
            return 0;
        }
    }
    LOG_WARN("Msg Unsubscribe: not found (topic:0x%04X)", topic);
    return 1;
}

// ------------------------------
// 发布（带完整可靠性检查）
// ------------------------------
void Msg_Publish(uint32_t topic, const void* data, uint16_t len, uint8_t prio)
{
    uint8_t match_found = 0;

    // 可靠性检查：优先级越界
    if (prio > MSG_PRIO_LOW) {
        LOG_WARN("Msg Publish: invalid prio (topic:0x%04X, prio:%d)", topic, prio);
        prio = MSG_PRIO_LOW;
    }

    // 遍历所有订阅者
    for (uint16_t i = 0; i < MAX_MSG_SUBSCRIBE; i++) {
        if (msg_cb.list[i].valid && msg_cb.list[i].topic == topic) {
            match_found = 1;

            // 可靠性检查：回调为空 → 跳过
            if (msg_cb.list[i].callback == NULL) {
                LOG_ERROR("Msg Publish: callback NULL (topic:0x%04X, idx:%d)", topic, i);
                msg_cb.list[i].valid = 0;  // 标记无效节点
                msg_cb.cnt--;
                continue;
            }

            // 安全调用回调
            msg_cb.list[i].callback(topic, data, len, prio);
        }
    }

    // 无任何人订阅 → 静默忽略（工业级安全行为）
    if (match_found == 0) {
        LOG_DEBUG("Msg Publish: no subscriber (topic:0x%04X)", topic);
    }
}

// ------------------------------
// 获取当前订阅数
// ------------------------------
uint16_t Msg_GetSubscribeCount(void)
{
    return msg_cb.cnt;
}

#endif
