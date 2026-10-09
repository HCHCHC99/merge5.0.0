#include "msg_pubsub.h"
#include "msg_topics.h"
#include "log_rtt.h"

static void cfg_cb(uint32_t t, const void *d, uint16_t l, uint8_t p)
{
    LOG_INFO("配置已更新，长度：%d 字节", l);
}

void Test_ConfigManager(void)
{
    LOG_INFO("========== 配置系统测试 ==========");

    Msg_Subscribe(TOPIC_CFG_UPDATED, cfg_cb);

    // 读配置
    Msg_Publish(TOPIC_CFG_READ_REQ, 0, 0, MSG_PRIO_MID);

    // 恢复默认
    Msg_Publish(TOPIC_CFG_DEFAULT_REQ, 0, 0, MSG_PRIO_MID);

    LOG_INFO("配置系统测试完成\n");
}
