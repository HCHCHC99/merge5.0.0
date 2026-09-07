#include "sys_sched.h"
#include "key.h"

static const SysModule_t Key_Module = {
    .name = "Key",
    .prio = 3,
    .period_ms = 1,
    .enabled = 1,
    .init = Key_Init,
    .task = Key_Task,
};

void Key_Module_Register(void) {
    Sys_Scheduler_RegisterModule(&Key_Module);
}
