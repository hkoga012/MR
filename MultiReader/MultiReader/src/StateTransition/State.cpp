/*!------------------------------------------------
@brief 状態基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
#include "State.h"

/*!------------------------------------------------
@brief コンストラクタ
@note
@param pOwnTask 自タスクポインタ
@param StateId 状態ID
@param MaxEvent イベント最大登録可能数
@return　なし
@author
-------------------------------------------------*/
State::State(BaseTask* pTask, uint32_t StateId, uint32_t MaxEvent)
    :_pOwnTask(pTask), _stateId(StateId), _maxEvent(MaxEvent),
    _pEventHandlerList(new BaseEventHandler* [MaxEvent]),
    _eventIndex(0), _enterHandler(NULL), _exitHandler(NULL)
{
    for (uint32_t i = 0; i < _maxEvent; i++)
    {
        _pEventHandlerList[i] = NULL;
    }
}


/*!------------------------------------------------
@brief 状態IDを取得
@note
@param　なし
@return　自状態ID
@author
-------------------------------------------------*/
const uint32_t& State::GetId(void) { return _stateId; }

/*!------------------------------------------------
@brief 状態に入る際に実行
@note
@param　なし
@return　なし
@author
-------------------------------------------------*/
void State::Enter(void)
{
    if (_enterHandler != NULL)
    {
        if (_enterHandler->doEnterFunc != NULL)
        {
            _enterHandler->doEnterFunc(_pOwnTask);
        }
    }
}

/*!------------------------------------------------
@brief 状態更新
@note
@param EventId イベントID
@param pData イベントデータ格納先頭ポインタ
@param DataLen イベントデータ長
@return なし
@author
-------------------------------------------------*/
void State::Update(const uint32_t& EventId, const uint8_t* pData, uint32_t DataLen)
{
    //指定IDが登録可能位置を超えている
    if (EventId >= _maxEvent)
    {
        THROW_FATAL_ERROR_ID(_pOwnTask->GetTaskName());
    }

    //イベントハンドラが登録されているならハンドラ実行
    //（未登録の場合、イベントハンドラを実行しないだけでエラーにはしない）
    if (NULL != _pEventHandlerList[EventId])
    {
        //イベントIDに該当するイベントハンドラ実行
        if (_pEventHandlerList[EventId]->doEventFunc != NULL)
        {
            _pEventHandlerList[EventId]->doEventFunc(_pOwnTask, EventId, pData, DataLen);
        }
    }
}

/*!------------------------------------------------
@brief 状態から出る際に実行
@note
@param　なし
@return　なし
@author
-------------------------------------------------*/
void State::Exit(void)
{
    if (_exitHandler != NULL)
    {
        if (_exitHandler->doExitFunc != NULL)
        {
            _exitHandler->doExitFunc(_pOwnTask);
        }
    }
}

/*!------------------------------------------------
@brief イベントハンドラ登録
@note
@param EventId  イベントID
@param pEnterHandler  イベントハンドラクラスポインタ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR State::AddEventHandler(uint32_t EventId, PF_DO_EVENT pFunc)
{
    //登録可能数を超過
    if (_eventIndex >= _maxEvent)
    {
        //std::cout << "Error: Full EventHandler: " << EventId;
        return E_MAX;
    }

    //指定IDが登録可能位置を超えている
    if (EventId >= _maxEvent)
    {
        return E_ID;
    }

    //既に登録済み
    if (NULL != _pEventHandlerList[EventId])
    {
        return E_REGISTERED;
    }

    _pEventHandlerList[EventId] = new BaseEventHandler(pFunc);
    _eventIndex++;

    return E_OK;
}

/*!------------------------------------------------
@brief Enterハンドラ登録
@note
@param pEnterHandler	Enterハンドラクラスポインタ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR State::AddEnterHandler(PF_DO_ENTER pFunc)
{
    if (NULL != _enterHandler)
    {
        return E_REGISTERED;    //登録済み
    }
    _enterHandler = new BaseEnterHandler(pFunc);
    return E_OK;
}

/*!------------------------------------------------
@brief Exitハンドラ登録
@note
@param pExitHandler	Exitハンドラクラスポインタ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR State::AddExitHandler(PF_DO_EXIT pFunc)
{
    if (NULL != _exitHandler)
    {
        return E_REGISTERED;    //登録済み
    }
    _exitHandler = new BaseExitHandler(pFunc);
    return E_OK;
}
