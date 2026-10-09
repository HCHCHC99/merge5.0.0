#include "storage.h"
#include "msg_pubsub.h"
#include "msg_topics.h"
#include "log_rtt.h"

void Test_Storage(void)
{
    LOG_INFO("======= 双存储系统测试 =======");

    uint8_t test_cfg[] = {10, 20, 30, 40};
    Msg_Publish(TOPIC_STORE_SAVE_CFG, test_cfg, 4, MSG_PRIO_MID);
    Msg_Publish(TOPIC_STORE_LOAD_CFG, NULL, 0, MSG_PRIO_MID);

    const char* log = "Test log message";
    Msg_Publish(TOPIC_STORE_WRITE_LOG, log, 16, MSG_PRIO_LOW);

    LOG_INFO("======= 测试完成 =======");
}
