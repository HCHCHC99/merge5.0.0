/**
 * @file    ctrl_speed.c
 * @brief   速度环实现
 */

#include "ctrl_speed.h"
#include "pid_common.h"
#include "pid_common.h"

extern Axis_t g_axis;

#ifndef NULL
  #define NULL                                            0
#endif

void Ctrl_Speed_Init(void)
{
    // 调度所有轴 的 电流环 初始化
    for (int i = 0; i < MAX_AXIS_NUM; i++) {

        PID_Init(&mySystem.axis[i].spd_param,
                &mySystem.axis[i].spd_ctrl);

        mySystem.axis[i].spd_param.alg_type = PID_ALG_INCREMENT;
        mySystem.axis[i].spd_param.kp = 20;
        mySystem.axis[i].spd_param.ki = 0.02;
        mySystem.axis[i].spd_param.kd = 0;
        mySystem.axis[i].spd_param.integral_max = 2000;

        // 速度环限幅
        // 速度环的输出为 目标电流 == 电流环的输入
        mySystem.axis[i].spd_ctrl.out_max = 4000.0f;
        mySystem.axis[i].spd_ctrl.out_min = 0.0f;

        // 速度偏差阈值
        mySystem.axis[i].spd_ctrl.stall_thr = 0.5;
    }
}

float g_Speedset[1000] = {0};
float g_Speedfdb[1000] = {0};
float g_SpeedOut[1000] = {0};
float g_SpeedErr[1000] = {0};

float g_Spd2CurSet = 1500;      //电流环设定mA

float g_SpdFeedForward = 280.0f;

//  注册管理平台 调度运行 运行时间 约1ms级-1ms
void Ctrl_Speed_Task(void)
{
    static uint16_t j,k =0 ;

     // 调度所有轴 的 速度环算法
    for (int i = 0; i < MAX_AXIS_NUM; i++) {

        // if (mySystem.axis[i].enable)
        if (mySystem.axis[i].spd_ctrl.enable)
        {

            PID_Calc
            (&mySystem.axis[i].spd_param,
                    &mySystem.axis[i].spd_ctrl,
                    mySystem.axis[i].spd_ctrl.set,
                    mySystem.axis[i].spd_ctrl.fdb);

//            /*260624_RL_add:Test 测试存中间变量，测完删除*/
            if(i == 0)
            {
                if(k >= 5)
                {
                    k = 0;
                    if(j<1000)
                    {
                       g_Speedset[j] = mySystem.axis[i].spd_ctrl.set;
                       g_Speedfdb[j] = mySystem.axis[i].spd_ctrl.fdb;
                       g_SpeedOut[j] = mySystem.axis[i].spd_ctrl.out;
                       g_SpeedErr[j] = mySystem.axis[i].spd_ctrl.err;
                       j++;
                    }

//                    mySystem.axis[i].spd_ctrl.set += 0.5;       //速度环设定值自增
                }
                else
                {
                    k++;
                }
            }

//            // 输出 -> 电流环目标
//            mySystem.axis[i].curr_ctrl.set = mySystem.axis[i].spd_ctrl.out;
//            mySystem.axis[i].curr_ctrl.set = g_Spd2CurSet;

        }
    }
}

// 模块注册
static const SysModule_t CtrlSpeed_Module = {
    .name = "Ctrl_Speed",
    .prio = 4,
    .period_ms = 1,
    .enabled = 1,
    .init = Ctrl_Speed_Init,
    // .task = Ctrl_Speed_Task,
    .task = NULL,
};

void Ctrl_Speed_Register(void)
{
    Sys_Scheduler_RegisterModule(&CtrlSpeed_Module);
}
