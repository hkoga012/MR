
#include "TaskControl.h"


/*!------------------------------------------------
@brief コンストラクタ
@note
@param MaxTask タスク最大数
@return なし
@author
-------------------------------------------------*/
TaskControl::TaskControl(uint32_t MaxTask) : _maxTask(MaxTask), _taskIndex(0), _isTimerEnabled(ON)
{
    //状態の領域作成
    _pTask = new BaseTask * [_maxTask];

    //タスクポインタ格納変数初期化
    for (uint32_t i = 0; i < _maxTask; i++)
    {
        _pTask[i] = NULL;
    }
}

/*!------------------------------------------------
@brief タスク登録
@note　タスクインスタンス登録
@param pTask　　タスクインスタンス
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR TaskControl::AddTask(BaseTask* pTask)
{

    //登録可能数を超過
    if (_taskIndex >= _maxTask)
    {
        //std::cout << "Error: Full Task: " << pTask->GetId();
        return E_MAX;
    }

    //既に登録済み
    for (uint32_t i = 0; i < _taskIndex; i++)
    {
        if (pTask == _pTask[i])
        {
            return E_REGISTERED;
        }
    }

    //タスク登録
    _pTask[_taskIndex] = pTask;
    
    _taskIndex++;

    return E_OK;
}

/*!------------------------------------------------
@brief 登録タスク実行
@note
@param なし
@return なし
@author
-------------------------------------------------*/
void TaskControl::DoTask(void) 
{
    for (uint32_t i = 0; i < _taskIndex; i++)
    {
        if (NULL != _pTask[i])
        {
            _pTask[i]->Update();
        }
    }
}

/*!------------------------------------------------
@brief タイマーカウント
@note　各状態がセットした時間経過でイベント通知する
@param なし
@return なし
@author
-------------------------------------------------*/
void TaskControl::TimerCount(void)
{
    if (_isTimerEnabled) // 通常はON。OFF になるのは、FATAL ERROR 発生時
    {
        for (uint32_t i = 0; i < _taskIndex; i++)
        {
            if (NULL != _pTask[i])
            {
                _pTask[i]->CountTimer();
            }
        }
    }
}
