// /**
//  * @file    ctrl_position.c
//  * @brief   位置环PID控制实现
//  */

#include "ctrl_position.h"
#include "pid_common.h"
#include <math.h>

extern Axis_t g_axis;

extern System_t mySystem;

void Ctrl_Pos_Init(void)
{
    // 调度所有轴 的 位置环 初始化
    for (int i = 0; i < MAX_AXIS_NUM; i++) {

        PID_Init(&mySystem.axis[i].pos_param,
                &mySystem.axis[i].pos_ctrl);

        mySystem.axis[i].pos_param.alg_type = PID_ALG_INCREMENT;
        mySystem.axis[i].pos_param.kp = 1.0f;        //260622_add:0.1++ 最后停止的位置和设定位置的err
        mySystem.axis[i].pos_param.ki = 0.01;
        mySystem.axis[i].pos_param.kd = 0;
        mySystem.axis[i].pos_param.integral_max = 0;

        // 位置环参数、限幅初始化
        // 位置环的输出为 目标速度 == 速度环的输入
        mySystem.axis[i].pos_ctrl.out_max = 8.0f;   //26622_RL_fix:
        mySystem.axis[i].pos_ctrl.out_min = -8.0f;   //26622_Rl_fix:

        // 位置偏差阈值
        mySystem.axis[i].pos_ctrl.stall_thr = 0.8;      // 0.5mm
    }
}
float g_PosOut[800] = {0};
float g_PosErr[800] = {0};
float g_Pos2Spd = 3.0;

//  注册管理平台 调度运行 运行时间 约10ms级  -5ms
void Ctrl_Pos_Task(void)
{
    static uint16_t j,k =0 ;
    // 调度所有轴 的 位置环算法
    for (int i = 0; i < MAX_AXIS_NUM; i++) {

        // if (mySystem.axis[i].enable && mySystem.axis[i].motor.enable)
        if (mySystem.axis[i].pos_ctrl.enable)
        {
            PID_Calc(&mySystem.axis[i].pos_param,
                    &mySystem.axis[i].pos_ctrl,
                    mySystem.axis[i].pos_ctrl.set,
                    mySystem.axis[i].pos_ctrl.fdb);

//            /*260624_RL_add:Test 测试存中间变量*/
            if(i == 0)
            {
                if(k >= 2)
                {
                    k = 0;
                    if(j<400)
                    {
                       g_PosOut[j] = mySystem.axis[i].pos_ctrl.out;
                       g_PosErr[j] = mySystem.axis[i].pos_ctrl.err;
                       j++;
                    }
                }
                else
                {
                    k++;
                }
            }

//             if (mySystem.axis[i].pos_ctrl.out < 0 && mySystem.axis[i].motor.dir == MOT_DIR_REVERSE)
//             {
//                 mySystem.axis[i].pos_ctrl.out *= -1;
//
//             }
//            // 输出 -> 速度环目标
//             mySystem.axis[i].spd_ctrl.set = mySystem.axis[i].pos_ctrl.out;
////             //mySystem.axis[i].curr_ctrl.set = mySystem.axis[i].pos_ctrl.out * mySystem.axis[i].pos_ctrl.out_nor;      //速度环不使用，位置环直接输出给电流环

//            /*260630_add:测试-false*/
//            float dist = fabsf(mySystem.axis[i].pos_ctrl.err);
//            float speed_limit;

//            if (dist > 15.0f) {
//                speed_limit = 8.0f;                          // 远距离：全速
//            } else if (dist > 8.0f) {
//                speed_limit = 2.0f + (dist - 2.0f) * 2.0f;  // 5→2mm: 线性 8→2 mm/s
//            } else if (dist > 3.0f) {
//                speed_limit = 1.0f + (dist - 0.5f) * 0.67f; // 2→0.5mm: 线性 2→1 mm/s
//            } else {
//                speed_limit = 1.0f;                          // <0.5mm: 最低 1mm/s 蠕行
//            }
//            // 对 pos_ctrl.out 限速（不改变 PID 内部状态）
//            if (mySystem.axis[i].pos_ctrl.out > speed_limit) {
//                mySystem.axis[i].pos_ctrl.out = speed_limit;
//            } else if (mySystem.axis[i].pos_ctrl.out < -speed_limit) {
//                mySystem.axis[i].pos_ctrl.out = -speed_limit;
//            }

            float dist = fabsf(mySystem.axis[i].pos_ctrl.err);
            float speed_limit;

            if (dist > 10.0f) {
                speed_limit = 5.0f;
            } else if (dist > 5.0f) {
                speed_limit = 3.0f;
            } else if (dist > 2.0f) {
                speed_limit = 1.5f;
            } else {
                speed_limit = 0.8f;
            }

           float spd_set = fabsf(mySystem.axis[i].pos_ctrl.out);

//            /*260624_RL_add:方向门控——ok */
            if (mySystem.axis[i].motor.dir == MOT_DIR_FORWARD)
            {
                // FORWARD：只允许正 out 通过
                if (mySystem.axis[i].pos_ctrl.out > 0)
                {
                    //mySystem.axis[i].spd_ctrl.set = mySystem.axis[i].pos_ctrl.out;
                    spd_set = mySystem.axis[i].pos_ctrl.out;
                }
                else
                {
                    //mySystem.axis[i].spd_ctrl.set = 0.0f;
                    spd_set = 0;
                }
            }
            else if (mySystem.axis[i].motor.dir == MOT_DIR_REVERSE)
            {
                // REVERSE：只允许负 out 通过，取反后输出
                if (mySystem.axis[i].pos_ctrl.out < 0)
                {
                    //mySystem.axis[i].spd_ctrl.set = -(mySystem.axis[i].pos_ctrl.out);
                    spd_set = -mySystem.axis[i].pos_ctrl.out;
                }
                else
                {
                    //mySystem.axis[i].spd_ctrl.set = 0.0f;
                    spd_set = 0;
                }
            }

        if (spd_set > speed_limit) {
            spd_set = speed_limit;
        }

         //mySystem.axis[i].spd_ctrl.set = spd_set;

                    mySystem.axis[i].spd_ctrl.set = g_Pos2Spd;

        }
    }
}

// // 模块注册
// static const SysModule_t CtrlPos_Module = {
//     .name = "Ctrl_Pos",
//     .prio = 5,
//     .period_ms = 5,
//     .enabled = 1,
//     .init = Ctrl_Pos_Init,
//     .task = Ctrl_Pos_Task,
// };

// void Ctrl_Pos_Register(void)
// {
//     Sys_Scheduler_RegisterModule(&CtrlPos_Module);
// }
