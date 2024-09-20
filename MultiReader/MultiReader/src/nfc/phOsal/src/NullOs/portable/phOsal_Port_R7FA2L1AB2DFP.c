/*!------------------------------------------------
@brief NFC用NULLOS用タイマー制御
@note　phOsal_Port_CM3.cをベースに作成
@author
-------------------------------------------------*/


#include "phOsal.h"
#include "phOsal_NullOs_Port.h"
#include "hal_data.h"
#include "stdio.h"
#ifdef PH_OSAL_NULLOS

#define __ENABLE_IRQ() __asm volatile ("cpsie i")
#define __DISABLE_IRQ() __asm volatile ("cpsid i")
//#define __WFE() __asm volatile ("wfe")
//#define __SEV() __asm volatile ("sev")
#define TIMER_MAX_COUNT (0x0000FFFF)
static pphOsal_TickTimerISRCallBck_t pTickCallBack;
static timer_info_t  one_shot_info =
{
 .clock_frequency = 0,
 .count_direction = (timer_direction_t) 0,
 .period_counts = 0,
};
uint32_t agt_counts = 0;	//タイマーカウント数格納

//ワンショットタイマー初期化
phStatus_t phOsal_InitTickTimer(pphOsal_TickTimerISRCallBck_t pTickTimerCallback)
{
    fsp_err_t err = FSP_SUCCESS;
    pTickCallBack = pTickTimerCallback;
	
	err = R_AGT_Open(&g_timer3_ctrl, &g_timer3_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }
	
    return PH_OSAL_SUCCESS;
}
//ワンショットタイマーカウント数設定、タイマー開始
static phStatus_t phOsal_ConfigTick(uint32_t *pAgtCounts)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_AGT_Stop(&g_timer3_ctrl);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }
    err = R_AGT_Disable(&g_timer3_ctrl);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }

    //待ち時間設定
	//AGTタイマーは16bitのため、最大で0xFFFF/(24Mhz/8)=0.021845secまでしか計測できない
	//この値を超える待ち時間はAGTタイマー最大計測時間を繰り返して目的の時間が経過したかを検出する
    uint32_t raw_count = 0;
    if(*pAgtCounts > TIMER_MAX_COUNT)
    {
        raw_count = TIMER_MAX_COUNT;
        *pAgtCounts -= TIMER_MAX_COUNT;
    }
    else
    {
        raw_count = *pAgtCounts & TIMER_MAX_COUNT;
        *pAgtCounts = 0;
    }
    err = R_AGT_PeriodSet(&g_timer3_ctrl, raw_count);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }

    err = R_AGT_Enable(&g_timer3_ctrl);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }
    
    //タイマー開始
    err = R_AGT_Start(&g_timer3_ctrl);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }

    return PH_OSAL_SUCCESS;
}
//ワンショットタイマー開始
phStatus_t phOsal_StartTickTimer(uint32_t dwTimeMilliSecs)
{
	fsp_err_t err = FSP_SUCCESS;
	
	err = R_AGT_InfoGet(&g_timer3_ctrl, &one_shot_info);
    if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }

    //指定待ち時間からタイマーのカウント数を計算
	agt_counts = (uint32_t)((dwTimeMilliSecs * one_shot_info.clock_frequency ) / 1000); // 1/1000 = 1ms
	err = phOsal_ConfigTick(&agt_counts);
	if (FSP_SUCCESS != err)
    {
        return PH_OSAL_FAILURE;
    }
	
    return PH_OSAL_SUCCESS;
}

//タイマー停止
phStatus_t phOsal_StopTickTimer(void)
{
    fsp_err_t err = R_AGT_Stop(&g_timer3_ctrl);
	if (FSP_SUCCESS != err)
    {
    	return PH_OSAL_FAILURE;
    }

    return PH_OSAL_SUCCESS;
}

void phOsal_EnterCriticalSection(void)
{
    __DISABLE_IRQ();
}

void phOsal_ExitCriticalSection(void)
{
    __ENABLE_IRQ();
}
extern void NFCDoTask(void);
void phOsal_Sleep(void)
{
	//__WFE();
    //状態遷移タスクを実行
    NFCDoTask();
}

void phOsal_WakeUp(void)
{
    //__SEV();
}

//ワンショットタイマー割り込みハンドラ
void SysTick_Handler(void)
{
    if(agt_counts > 0)
    {
        if(agt_counts > TIMER_MAX_COUNT)
        {
            //タイマーカウント減算
            agt_counts -= TIMER_MAX_COUNT;
        }
        else
        {
            //タイマー再設定
            phOsal_ConfigTick(&agt_counts);
        }
    }
    else
    {
        //指定時間経過で停止
        phOsal_StopTickTimer();

        if(NULL != pTickCallBack)
        {
            pTickCallBack();
        }
    }
}

//Timer1コールバック関数
void g_timer3_callback(timer_callback_args_t *p_args)
{
	if(TIMER_EVENT_CYCLE_END == p_args->event)
	{
		SysTick_Handler();
	}
}

#endif /*PH_OSAL_NULLOS*/

