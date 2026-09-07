/**
 * @file    key_state.c
 * @brief   按键标定状态机实现
 * @note    1.订阅key.c发布的按键事件消息
 *          2.使用项目统一静态软件定时器做5s长按计时
 *          3.状态切换入口统一发送事件组信号
 */
#include "key_state.h"
#include "event_def.h"
#include "msg_pubsub.h"
#include "axis_typedef.h"
#include "hc32_ll_utility.h"
#include "msg_topics.h"
#include "act_state.h"
#include "state_engine.h"

/* 全局系统对象 */
extern System_t mySystem;

/* 全局状态机实例 */
static StateMachine_t g_key_sm;

/* 按键硬件编号定义（和key.c映射对应） */
#define KEY_M1_CODE     (0U)
#define KEY_M2_CODE     (1U)
#define KEY_M1_M2_CODE  (0x20)

/* 内部按键电平缓存（仅保存按键按下状态，不再保存tick计时） */
typedef struct {
    uint8_t m1_press;           // M1当前是否按下 1按下 0松开
    uint8_t m2_press;           // M2当前是否按下 1按下 0松开
    uint8_t m1_m2_press;        // M1 且 M2 当前是否 同时按下
} Key_Detect_Cache_t;
static Key_Detect_Cache_t g_key_cache = {0};

/* ===================== 软件定时器句柄（静态注册，4路单次定时器） ===================== */
static SoftTimer_Handle_t htim_m1_enter;    // M1进入标定5s计时
static SoftTimer_Handle_t htim_m1_exit;     // M1退出标定5s计时
static SoftTimer_Handle_t htim_m2_enter;    // M2进入标定5s计时
static SoftTimer_Handle_t htim_m2_exit;     // M2退出标定5s计时

/* ===================== 软定时器回调函数声明 ===================== */
static void TimerCb_M1Enter(SoftTimer_Handle_t htimer);
static void TimerCb_M1Exit(SoftTimer_Handle_t htimer);
static void TimerCb_M2Enter(SoftTimer_Handle_t htimer);
static void TimerCb_M2Exit(SoftTimer_Handle_t htimer);

/**
 * @brief 按键状态枚举
 */
typedef enum {
    KEY_STATE_INIT = 0,                   // 初始化状态
    KEY_STATE_IDLE ,                      // 空闲状态

    KEY_STATE_CALIB_M1_ENTERTING,         // M1 标定进入中
    KEY_STATE_CALIB_M1_ENTERED,        	 // M1 标定已进入，但是按键未松手
    KEY_STATE_CALIB_M1_IDLE,              // M1 标定 空闲状态
    KEY_STATE_CALIB_M1_MOVEOUT,           // M1 标定 伸出状态
    KEY_STATE_CALIB_M1_MOVEIN,            // M1 标定 缩回状态
    KEY_STATE_CALIB_M1_EXITING,           // M1 标定退出中

    KEY_STATE_CALIB_M2_ENTERTING,         // M2 标定进入中
    KEY_STATE_CALIB_M2_ENTERED,        	 // M2 标定已进入，但是按键未松手
    KEY_STATE_CALIB_M2_IDLE,              // M2 标定 空闲状态
    KEY_STATE_CALIB_M2_MOVEOUT,           // M2 标定 伸出状态
    KEY_STATE_CALIB_M2_MOVEIN,            // M2 标定 缩回状态
    KEY_STATE_CALIB_M2_EXITING,           // M2 标定退出中
} KeyState_t;

/* 状态入口函数声明 */
static void key_enter_idle(void);
static void key_enter_evt(void);

/**
 * @brief 按键状态跳转表（完全保留原有业务逻辑，无修改）
 */
static const StateJumpTable_t key_jump[] = {
    {KEY_STATE_INIT,                  EVT_KEY_MODEL_ENABLE,      KEY_STATE_IDLE},

    /* ===================== M1标定流程 ===================== */
    {KEY_STATE_IDLE,                  EVT_KEY_M1_ENTERING,       KEY_STATE_CALIB_M1_ENTERTING},

//    {KEY_STATE_CALIB_M1_ENTERTING,    EVT_KEY_M1_ENTERED,        KEY_STATE_CALIB_M1_IDLE},
	{KEY_STATE_CALIB_M1_ENTERTING,    EVT_KEY_M1_ENTERED,        KEY_STATE_CALIB_M1_ENTERED},
    {KEY_STATE_CALIB_M1_ENTERTING,    EVT_KEY_M1_CANCEL,         KEY_STATE_IDLE},
    {KEY_STATE_CALIB_M1_ENTERTING,    EVT_KEY_RESET,         KEY_STATE_IDLE},

	{KEY_STATE_CALIB_M1_ENTERED,      EVT_KEY_M1_CANCEL,        KEY_STATE_CALIB_M1_IDLE},
    {KEY_STATE_CALIB_M1_ENTERED,		EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M1_IDLE,         EVT_KEY_M1_EXITING,        KEY_STATE_CALIB_M1_EXITING},
    {KEY_STATE_CALIB_M1_IDLE,         EVT_KEY_M1_MOVEOUT,        KEY_STATE_CALIB_M1_MOVEOUT},
    {KEY_STATE_CALIB_M1_IDLE,         EVT_KEY_M1_MOVEIN,         KEY_STATE_CALIB_M1_MOVEIN},
    {KEY_STATE_CALIB_M1_IDLE,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M1_EXITING,      EVT_KEY_M1_EXITED,         KEY_STATE_IDLE},
    {KEY_STATE_CALIB_M1_EXITING,      EVT_KEY_M1_CALIB_CANCEL,   KEY_STATE_CALIB_M1_IDLE},
    {KEY_STATE_CALIB_M1_EXITING,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M1_MOVEOUT,      EVT_KEY_M1_EXITING,        KEY_STATE_CALIB_M1_EXITING},
    {KEY_STATE_CALIB_M1_MOVEOUT,      EVT_KEY_M1_CALIB_CANCEL,   KEY_STATE_CALIB_M1_IDLE},
    {KEY_STATE_CALIB_M1_MOVEOUT,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M1_MOVEIN,       EVT_KEY_M1_EXITING,        KEY_STATE_CALIB_M1_EXITING},
    {KEY_STATE_CALIB_M1_MOVEIN,       EVT_KEY_M1_CALIB_CANCEL,   KEY_STATE_CALIB_M1_IDLE},
    {KEY_STATE_CALIB_M1_MOVEIN,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    /* ===================== M2标定流程 ===================== */
    {KEY_STATE_IDLE,                  EVT_KEY_M2_ENTERING,       KEY_STATE_CALIB_M2_ENTERTING},

//    {KEY_STATE_CALIB_M2_ENTERTING,    EVT_KEY_M2_ENTERED,        KEY_STATE_CALIB_M2_IDLE},
	{KEY_STATE_CALIB_M2_ENTERTING,    EVT_KEY_M2_ENTERED,        KEY_STATE_CALIB_M2_ENTERED},
    {KEY_STATE_CALIB_M2_ENTERTING,    EVT_KEY_M2_CANCEL,         KEY_STATE_IDLE},
    {KEY_STATE_CALIB_M2_ENTERTING,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

	{KEY_STATE_CALIB_M2_ENTERED,      EVT_KEY_M2_CANCEL,        KEY_STATE_CALIB_M2_IDLE},
    {KEY_STATE_CALIB_M2_ENTERED,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M2_IDLE,         EVT_KEY_M2_EXITING,        KEY_STATE_CALIB_M2_EXITING},
    {KEY_STATE_CALIB_M2_IDLE,         EVT_KEY_M2_MOVEOUT,        KEY_STATE_CALIB_M2_MOVEOUT},
    {KEY_STATE_CALIB_M2_IDLE,         EVT_KEY_M2_MOVEIN,         KEY_STATE_CALIB_M2_MOVEIN},
    {KEY_STATE_CALIB_M2_IDLE,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M2_EXITING,      EVT_KEY_M2_EXITED,         KEY_STATE_IDLE},
    {KEY_STATE_CALIB_M2_EXITING,      EVT_KEY_M2_CALIB_CANCEL,   KEY_STATE_CALIB_M2_IDLE},
    {KEY_STATE_CALIB_M2_EXITING,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M2_MOVEOUT,      EVT_KEY_M2_EXITING,        KEY_STATE_CALIB_M2_EXITING},
    {KEY_STATE_CALIB_M2_MOVEOUT,      EVT_KEY_M2_CALIB_CANCEL,   KEY_STATE_CALIB_M2_IDLE},
    {KEY_STATE_CALIB_M2_MOVEOUT,		 EVT_KEY_RESET,         KEY_STATE_IDLE},

    {KEY_STATE_CALIB_M2_MOVEIN,       EVT_KEY_M2_EXITING,        KEY_STATE_CALIB_M2_EXITING},
    {KEY_STATE_CALIB_M2_MOVEIN,       EVT_KEY_M2_CALIB_CANCEL,   KEY_STATE_CALIB_M2_IDLE},
    {KEY_STATE_CALIB_M2_MOVEIN,		 EVT_KEY_RESET,         KEY_STATE_IDLE},
};

/**
 * @brief 按键状态入口函数表（无修改）
 */
static const StateFuncTable_t key_func[] = {
   {KEY_STATE_INIT,                key_enter_idle,     0},
   {KEY_STATE_IDLE,                key_enter_idle,     0},

   {KEY_STATE_CALIB_M1_ENTERTING,  key_enter_evt,      0},
   {KEY_STATE_CALIB_M1_ENTERED,  	key_enter_evt,      0},
   {KEY_STATE_CALIB_M1_IDLE,       key_enter_evt,      0},
   {KEY_STATE_CALIB_M1_MOVEOUT,    key_enter_evt,      0},
   {KEY_STATE_CALIB_M1_MOVEIN,     key_enter_evt,      0},
   {KEY_STATE_CALIB_M1_EXITING,    key_enter_evt,      0},

   {KEY_STATE_CALIB_M2_ENTERTING,  key_enter_evt,      0},
   {KEY_STATE_CALIB_M2_ENTERED,  	key_enter_evt,      0},
   {KEY_STATE_CALIB_M2_IDLE,       key_enter_evt,      0},
   {KEY_STATE_CALIB_M2_MOVEOUT,    key_enter_evt,      0},
   {KEY_STATE_CALIB_M2_MOVEIN,     key_enter_evt,      0},
   {KEY_STATE_CALIB_M2_EXITING,    key_enter_evt,      0},
};

/* ===================== 软定时器回调实现 ===================== */
static void TimerCb_M1Enter(SoftTimer_Handle_t htimer)
{
   /* 5s长按到达，发送M1进入标定完成事件 */
   StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_ENTERED);
}

static void TimerCb_M1Exit(SoftTimer_Handle_t htimer)
{
   /* 双按键按住5s，退出标定回到空闲 */
   StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_EXITED);
}

static void TimerCb_M2Enter(SoftTimer_Handle_t htimer)
{
   StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_ENTERED);
}

static void TimerCb_M2Exit(SoftTimer_Handle_t htimer)
{
   StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_EXITED);
}

/**
 * @brief 按键状态机 初始化
 * @note  1.绑定跳转表、函数表
 *        2.注册4路软件定时器、绑定回调
 *        3.订阅按键消息主题 TOPIC_KEYS_STATE
 */
void Key_State_Init(void)
{
    StateMachine_t *sm_temp = &g_key_sm;
    sm_temp->jump_table = key_jump;
    sm_temp->jump_table_size = sizeof(key_jump)/sizeof(StateJumpTable_t);
    sm_temp->func_table = key_func;
    sm_temp->func_table_size = sizeof(key_func)/sizeof(StateFuncTable_t);
    sm_temp->init_state = KEY_STATE_IDLE;

    StateMachine_Init(sm_temp);

    /* 注册4个单次软件定时器，绑定对应回调 */
    htim_m1_enter = SoftTimer_Register();
    htim_m1_exit  = SoftTimer_Register();
    htim_m2_enter = SoftTimer_Register();
    htim_m2_exit  = SoftTimer_Register();

    if(htim_m1_enter != NULL) SoftTimer_SetCallback(htim_m1_enter, TimerCb_M1Enter);
    if(htim_m1_exit  != NULL) SoftTimer_SetCallback(htim_m1_exit,  TimerCb_M1Exit);
    if(htim_m2_enter != NULL) SoftTimer_SetCallback(htim_m2_enter, TimerCb_M2Enter);
    if(htim_m2_exit  != NULL) SoftTimer_SetCallback(htim_m2_exit,  TimerCb_M2Exit);

    /* 订阅key.c发布的按键消息，绑定回调 */
    Msg_Subscribe(TOPIC_KEYS_STATE, State_Key_Callback);
}

/**
 * @brief 系统状态 - 位置信息回调处理函数
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的位置状态数据
 */
void State_Key_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio)
{
    if(len != sizeof(Key_Report_t))
    {
        return;
    }
    Key_Report_t *p_key_report = (Key_Report_t *)data;

    /* 更新按键按下/松开标记 */
    if(p_key_report->key_code == KEY_M1_CODE)
    {
        if(p_key_report->event == KEY_EVENT_CLICK || p_key_report->event == KEY_EVENT_LONG)
        {
            g_key_cache.m1_press = 1U;
        }
        else if(p_key_report->event == KEY_EVENT_RELEASE)
        {
            g_key_cache.m1_press = 0U;
			g_key_cache.m1_m2_press = 0U;
            /* M1松开：停止M1 相关计时，取消 进入流程 */
            SoftTimer_Stop(htim_m1_enter);

            KeyState_t cur = g_key_sm.cur_state;
            if(cur == KEY_STATE_CALIB_M1_ENTERTING || cur == KEY_STATE_CALIB_M1_ENTERED)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_CANCEL);
            }

			//取消退出标定状态
			if(cur == KEY_STATE_CALIB_M1_EXITING)
            {
				g_key_cache.m1_m2_press = 0U;
				SoftTimer_Stop(htim_m1_exit);
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_CALIB_CANCEL);
            }
            if(cur == KEY_STATE_CALIB_M2_EXITING)
            {
				g_key_cache.m1_m2_press = 0U;
				SoftTimer_Stop(htim_m2_exit);
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_CALIB_CANCEL);
			}
//            if(cur == KEY_STATE_CALIB_M1_MOVEOUT || cur == KEY_STATE_CALIB_M1_MOVEIN)//20260714-zjw
//            {
//                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_MOVE_CANNEL);
//            }
        }
    }
    else if(p_key_report->key_code == KEY_M2_CODE)
    {
        if(p_key_report->event == KEY_EVENT_CLICK || p_key_report->event == KEY_EVENT_LONG)
        {
            g_key_cache.m2_press = 1U;
        }
        else if(p_key_report->event == KEY_EVENT_RELEASE)
        {
            g_key_cache.m2_press = 0U;
			g_key_cache.m1_m2_press = 0U;
            /* M2松开：停止M2相关计时，取消 进入流程 */
            SoftTimer_Stop(htim_m2_enter);

            KeyState_t cur = g_key_sm.cur_state;
            if(cur == KEY_STATE_CALIB_M2_ENTERTING  || cur == KEY_STATE_CALIB_M2_ENTERED)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_CANCEL);
            }

			//取消退出标定状态
			if(cur == KEY_STATE_CALIB_M1_EXITING)
            {
				g_key_cache.m1_m2_press = 0U;
				SoftTimer_Stop(htim_m1_exit);
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_CALIB_CANCEL);
            }
            if(cur == KEY_STATE_CALIB_M2_EXITING)
            {
				g_key_cache.m1_m2_press = 0U;
				SoftTimer_Stop(htim_m2_exit);
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_CALIB_CANCEL);
            }
//            if(cur == KEY_STATE_CALIB_M2_MOVEOUT || cur == KEY_STATE_CALIB_M2_MOVEIN)//20260714-zjw
//            {
//                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_MOVE_CANNEL);
//            }
        }
    }
    else if (p_key_report->key_code == KEY_M1_M2_CODE)
    {
        if(p_key_report->event == KEY_EVENT_CLICK || p_key_report->event == KEY_EVENT_LONG)
        {
            g_key_cache.m1_m2_press = 1U;//zjw - 2026-7-17加：手动标定伸出/缩回过程中，接收到此按键状态应该停止
        }
        else if(p_key_report->event == KEY_EVENT_RELEASE)
        {
            g_key_cache.m1_m2_press = 0U;
            /* M1+M2松开：停止相关计时，取消 退出流程 */
            SoftTimer_Stop(htim_m1_exit);
            SoftTimer_Stop(htim_m2_exit);
            KeyState_t cur = g_key_sm.cur_state;
            if(cur == KEY_STATE_CALIB_M1_EXITING)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_CALIB_CANCEL);
            }
            if(cur == KEY_STATE_CALIB_M2_EXITING)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_CALIB_CANCEL);
            }

        }
    }

    /* 单次按键按下事件，启动对应单次软定时器5s计时 */
    KeyState_t cur_state = (KeyState_t)g_key_sm.cur_state;
    switch(cur_state)
    {
        case KEY_STATE_IDLE:
            /* 仅M1按下：进入M1标定，启动5s单次定时器 */
            if(g_key_cache.m1_press && !g_key_cache.m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_ENTERING);
                SoftTimer_Start(htim_m1_enter, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
            }
            /* 仅M2按下：进入M2标定，启动5s单次定时器 */
            if(g_key_cache.m2_press && !g_key_cache.m1_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_ENTERING);
                SoftTimer_Start(htim_m2_enter, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
            }

            break;

        case KEY_STATE_CALIB_M1_IDLE:
            if(g_key_cache.m1_press && !g_key_cache.m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_MOVEOUT);
            }
            if(g_key_cache.m2_press && !g_key_cache.m1_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_MOVEIN);
            }
            /* M1+M2同时按下，启动退出标定5s计时 */
            if(g_key_cache.m1_m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_EXITING);
                SoftTimer_Start(htim_m1_exit, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
            }
            break;

        case KEY_STATE_CALIB_M2_IDLE:
            if(g_key_cache.m2_press && !g_key_cache.m1_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_MOVEIN);
            }
            if(g_key_cache.m1_press && !g_key_cache.m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_MOVEOUT);
            }
            /* M1+M2同时按下，启动退出标定5s计时 */
            if(g_key_cache.m1_m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_EXITING);
                SoftTimer_Start(htim_m2_exit, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
            }
            break;

		//增加case条件 标定伸出/标定缩回状态也要能够响应退出事件
		case KEY_STATE_CALIB_M1_MOVEOUT:
		case KEY_STATE_CALIB_M1_MOVEIN:
			if(g_key_cache.m1_m2_press)
			{
				StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_EXITING);
                SoftTimer_Start(htim_m1_exit, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
			}

		case KEY_STATE_CALIB_M2_MOVEOUT:
		case KEY_STATE_CALIB_M2_MOVEIN:
			if(g_key_cache.m1_m2_press)
			{
				StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_EXITING);
                SoftTimer_Start(htim_m2_exit, SOFT_TIMER_MODE_ONCE, KEY_CALIB_HOLD_MS);
			}

        default:
            break;
    }
}

/**
 * @brief 按键状态机周期任务
 * @note 10ms周期调用，仅负责状态机驱动、动作态松开检测，不再处理计时差值
 */
void Key_State_Task(void)
{
    KeyState_t cur_state = (KeyState_t)g_key_sm.cur_state;

    /* M1/M2 伸出/缩回运行态，松开任意键取消动作 */
    switch(cur_state)
    {
        case KEY_STATE_CALIB_M1_MOVEOUT:
        case KEY_STATE_CALIB_M1_MOVEIN:
            if(!g_key_cache.m1_press && !g_key_cache.m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M1_CALIB_CANCEL);
            }
            break;
        case KEY_STATE_CALIB_M2_MOVEOUT:
        case KEY_STATE_CALIB_M2_MOVEIN:
            if(!g_key_cache.m1_press && !g_key_cache.m2_press)
            {
                StateMachine_SendEvent(&g_key_sm, EVT_KEY_M2_CALIB_CANCEL);
            }
            break;
        default:
            break;
    }
}

/* ===================== 状态入口函数 ===================== */
/**
 * @brief 空闲状态统一入口
 */
static void key_enter_idle(void)
{
   /* 停止所有4路软定时器，清空按键缓存 */
   SoftTimer_Stop(htim_m1_enter);
   SoftTimer_Stop(htim_m1_exit);
   SoftTimer_Stop(htim_m2_enter);
   SoftTimer_Stop(htim_m2_exit);
   memset(&g_key_cache, 0, sizeof(Key_Detect_Cache_t));

	KeyState_t cur = g_key_sm.cur_state;
	switch(cur)
	{
       case KEY_STATE_IDLE:
		   if (KEY_STATE_CALIB_M1_ENTERTING == g_key_sm.previous_state) //进入标定中，按键没按到5s，不进了
		   {
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INTO_MANU_CANCEL);//20260714-zjw

		   }else if (KEY_STATE_CALIB_M2_ENTERTING == g_key_sm.previous_state)
		   {
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INTO_MANU_CANCEL);
		   }else if ((KEY_STATE_CALIB_M1_EXITING == g_key_sm.previous_state))//标定状态中 -- 退出标定模式
		   {
			   EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_DONE);
		   }else if ((KEY_STATE_CALIB_M2_EXITING == g_key_sm.previous_state))//标定状态中 -- 退出标定模式
		   {
			   EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_DONE);
		   }

           break;

        default:
            break;
	}
}

/**
 * @brief 所有标定相关状态统一入口函数
 */
static void key_enter_evt(void)
{
   KeyState_t cur = g_key_sm.cur_state;
   switch(cur)
   {
//       case KEY_STATE_IDLE:
//           EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INTO_MANU_CANCEL);//20260714-zjw
//           EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INTO_MANU_CANCEL);
//           break;
       case KEY_STATE_CALIB_M1_ENTERTING:
           EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INTO_MANU_ING);
           break;
	   case KEY_STATE_CALIB_M1_ENTERED:
			EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INTO_MANU);
		   break;

       case KEY_STATE_CALIB_M1_IDLE://20260714-zjw
			if (KEY_STATE_CALIB_M1_ENTERED == g_key_sm.previous_state)//刚进入标定状态
			{
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INTO_MANU_ED);
			}else if (KEY_STATE_CALIB_M1_EXITING == g_key_sm.previous_state)//取消退出标定状态
			{
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_EXIT_MANU_CANCEL);
			}else if (KEY_STATE_CALIB_M1_MOVEOUT == g_key_sm.previous_state || KEY_STATE_CALIB_M1_MOVEIN == g_key_sm.previous_state)
			{
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_MANU_STOP);
			}
           break;
       case KEY_STATE_CALIB_M1_MOVEOUT:
           EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_MANU_OUT);
           break;
       case KEY_STATE_CALIB_M1_MOVEIN:
           EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_MANU_IN);
           break;
       case KEY_STATE_CALIB_M1_EXITING:
           EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_EXIT_MANU_ING);
           break;

       case KEY_STATE_CALIB_M2_ENTERTING:
           EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INTO_MANU_ING);
           break;
	   case KEY_STATE_CALIB_M2_ENTERED:
		   EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INTO_MANU);
		   break;
       case KEY_STATE_CALIB_M2_IDLE:
           if (KEY_STATE_CALIB_M2_ENTERED == g_key_sm.previous_state)//刚进入标定状态
			{
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INTO_MANU_ED);
			}else if (KEY_STATE_CALIB_M2_EXITING == g_key_sm.previous_state)//取消退出标定状态
			{
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_EXIT_MANU_CANCEL);
			}else if (KEY_STATE_CALIB_M2_MOVEOUT == g_key_sm.previous_state || KEY_STATE_CALIB_M2_MOVEIN == g_key_sm.previous_state)
			{
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_MANU_STOP);
			}
           break;
       case KEY_STATE_CALIB_M2_MOVEOUT:
           EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_MANU_OUT);
           break;
       case KEY_STATE_CALIB_M2_MOVEIN:
           EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_MANU_IN);
           break;
       case KEY_STATE_CALIB_M2_EXITING:
           EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_EXIT_MANU_ING);
           break;
        default:
            break;
   }
}

//复位
void key_state_reset(void)
{
	StateMachine_SendEvent(&g_key_sm, EVT_KEY_RESET);
}
