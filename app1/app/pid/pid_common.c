/**
 * @file    pid_common.c
 * @brief   可切换式通用PID算法实现
 * @note    纯算法，无任何硬件/状态机依赖
 */

#include "pid_common.h"
#include <math.h>

/**
 * @brief  内部函数：位置式PID实现
 * @param  p: 参数指针
 * @param  c: 运行数据指针
 * @return 输出值
 */
static float PID_Position(PID_Param_t *p, PID_Ctrl_t *c)
{
    // 积分累加
    c->integral += c->err;

    // 积分限幅，防止饱和
    if(fabs(c->integral) > p->integral_max)
    {
        c->integral = p->integral_max * (c->integral > 0 ? 1 : -1);
    }

    // 位置式PID输出公式
    c->out = p->kp * c->err + p->ki * c->integral;

    return c->out;
}

/**
 * @brief  内部函数：增量式PID实现
 * @param  p: 参数
 * @param  c: 数据
 * @return 输出
 */
static float PID_Increment(PID_Param_t *p, PID_Ctrl_t *c)
{
    // 增量计算
    // float inc = p->kp * (c->err - c->err_last) + p->ki * c->err + p->kd * (c->err - 2*c->err_last + c->err_last_2 );
    float inc = p->kp * (c->err - c->err_last) + p->ki * c->err ;

    // 输出累加
    c->out += inc;

    c->err_last_2 = c->err_last;

    c->err_last = c->err;

    return c->out;
}

/**
 * @brief  PID统一计算入口（上层无需关心内部算法）
 */
float PID_Calc(PID_Param_t *p, PID_Ctrl_t *c, float set, float fdb)
{
    // 保存设定值
    c->set = set;

    // 保存反馈值
    c->fdb = fdb;

    // 计算误差
    c->err = set - fdb;

    // 根据配置的算法类型分支执行
    switch(p->alg_type)
    {
        case PID_ALG_POSITION:
            c->out = PID_Position(p, c);
        break;

        case PID_ALG_INCREMENT:
            c->out =  PID_Increment(p, c);
        break;

        default:
            return 0.0f;
    }
    if (c->out < c->out_min)
    {
        c->out = c->out_min;
    }else if (c->out > c->out_max)
    {
        c->out = c->out_max;
    }
    // c->out = ( c->out > c->out_max) ? c->out_max : c->out;
    // c->out = ( c->out < c->out_min) ? c->out_min : c->out;

    return c->out;
}

/**
 * @brief  PID初始化
 */
void PID_Init(PID_Param_t *param, PID_Ctrl_t *ctrl)
{
    // 调用复位函数清空状态
    PID_Reset(ctrl);
}

/**
 * @brief  PID复位，清空所有中间变量
 */
void PID_Reset(PID_Ctrl_t *ctrl)
{
    ctrl->set = 0;
    ctrl->fdb = 0;
    ctrl->err = 0;
    ctrl->err_last = 0;
    ctrl->integral = 0;
    ctrl->out = 0;
}

/**
 * @brief  设置PID输出限幅范围
 * @param  ctrl: PID运行数据句柄
 * @param  out_min: 输出最小值
 * @param  out_max: 输出最大值
 * @retval 无
 */
void PID_SetOutputLimit(PID_Ctrl_t *ctrl, float out_min, float out_max)
{
    if(NULL == ctrl)
    {
        return;
    }
    // 容错：防止下限大于上限
    if(out_min < out_max)
    {
        ctrl->out_min = out_min;
        ctrl->out_max = out_max;
    }
    else
    {
        ctrl->out_min = out_max;
        ctrl->out_max = out_min;
    }
}

/**
 * @brief  单独设置PID目标设定值（仅更新，不运算）
 * @param  ctrl: PID运行数据句柄
 * @param  target: 目标设定值set
 * @retval 无
 */
void PID_SetTarget(PID_Ctrl_t *ctrl, float target)
{
    if(NULL == ctrl)
    {
        return;
    }
    ctrl->set = target;
    // 同步更新误差缓存，可选，看业务需求
    ctrl->err = target - ctrl->fdb;
}

//260708_add:pid清空复位
void PID_ResetOutput(PID_Ctrl_t *ctrl)
{
//    ctrl->set = 0.0f;
    ctrl->err = 0.0f;
    ctrl->err_last = 0.0f;
    ctrl->err_last_2 = 0.0f;
    ctrl->integral = 0.0f;
    ctrl->out = 0.0f;
	ctrl->fdb = 0.0f;
}
