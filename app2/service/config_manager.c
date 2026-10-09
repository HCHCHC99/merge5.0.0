#include "config_manager.h"
#include "msg_pubsub.h"
#include "msg_topics.h"
#include <stdint.h>
#include <string.h>

#if SYS_ENABLE_CONFIG

// ==============================
// 配置结构体 → 完全内部私有
// ==============================
typedef struct {
    int32_t  pos_kp;
    int32_t  pos_ki;
    int32_t  pos_kd;
    uint8_t  speed_percent;
    uint16_t current_limit;
    uint8_t  auto_stop_en;
} SysConfig_t;

// ==============================
// 内部全局变量 → 外部绝对不可访问
// ==============================
static SysConfig_t s_cfg;
static const SysConfig_t s_default_cfg = {
    .pos_kp = 100,
    .pos_ki = 10,
    .pos_kd = 5,
    .speed_percent = 50,
    .current_limit = 1500,
    .auto_stop_en = 1,
};

// ==============================
// 内部函数
// ==============================
static void Cfg_LoadDefault(void);
static void Cfg_UpdateNotify(void);
static void Cfg_MsgCallback(uint32_t topic, const void* data, uint16_t len, uint8_t prio);

// ==============================
// 初始化：订阅配置消息
// ==============================
void Cfg_Init(void)
{
    Cfg_LoadDefault();

    // 配置系统订阅自己的消息
    Msg_Subscribe(TOPIC_CFG_WRITE_REQ,  Cfg_MsgCallback);
    Msg_Subscribe(TOPIC_CFG_READ_REQ,   Cfg_MsgCallback);
    Msg_Subscribe(TOPIC_CFG_DEFAULT_REQ,Cfg_MsgCallback);
}

// ==============================
// 加载默认配置
// ==============================
static void Cfg_LoadDefault(void)
{
    memcpy(&s_cfg, &s_default_cfg, sizeof(SysConfig_t));
}

// ==============================
// 发布：配置已更新
// ==============================
static void Cfg_UpdateNotify(void)
{
    Msg_Publish(TOPIC_CFG_UPDATED, &s_cfg, sizeof(SysConfig_t), MSG_PRIO_MID);
}

// ==============================
// 配置系统消息回调（核心解耦点）
// ==============================
static void Cfg_MsgCallback(uint32_t topic, const void* data, uint16_t len, uint8_t prio)
{
    switch(topic)
    {
        // 写配置
        case TOPIC_CFG_WRITE_REQ:
            if(data && len == sizeof(SysConfig_t))
            {
                memcpy(&s_cfg, data, sizeof(SysConfig_t));
                Cfg_UpdateNotify();  // 发布更新
            }
            break;

        // 读配置 → 自动上报
        case TOPIC_CFG_READ_REQ:
            Cfg_UpdateNotify();
            break;

        // 恢复默认
        case TOPIC_CFG_DEFAULT_REQ:
            Cfg_LoadDefault();
            Cfg_UpdateNotify();
            break;
    }
}

// 空任务（可用于后期保存、校验）
void Cfg_Task(void)
{
}

#endif
