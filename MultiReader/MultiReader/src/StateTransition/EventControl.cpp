/*!------------------------------------------------
@brief イベント管理
@note
@author
-------------------------------------------------*/
#include "BaseTask.h"
#include "EventControl.h"
#include "FatalException.h"


/*!------------------------------------------------
@brief コンストラクタ
@note
@param MaxTask　　最大タスク数
@return
@author
-------------------------------------------------*/
EventControl::EventControl(BaseTask* pTask, const uint32_t MaxEvent)
    :_pOwnTask(pTask)
{
    //指定数分領域確保
    _pQueue = new Queue<CtrlData>(MaxEvent);
}

/*!------------------------------------------------
@brief イベントセット
@note  指定タスクへイベントを登録する
@param EventId	イベントID
@param pData	イベント通知時に渡すデータ、またはデータの先頭ポインタ
@return なし
@author
-------------------------------------------------*/
void EventControl::SetEvent(const uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    CtrlData data;

    //イベントキューが満なら受信しない
    if (_pQueue->IsFull())
    {
        THROW_FATAL_ERROR_QUE_FULL(_pOwnTask->GetTaskName());
    }

    //イベント格納
    data.EventId = EventId;
    data.pData = pData;
    data.DataLen = DataLen;

    _pQueue->Add(data);
}

/*!------------------------------------------------
@brief イベント取得
@note  指定タスクキューへ登録されたイベントを古い順に取り出す
@param pEventId  イベント格納先頭ポインタ
@param pData     イベントデータ格納先頭ポインタのポインタ
@param pDataLen	 イベントデータ長格納先頭ポインタ
@return E_OK:イベントあり / E_EMP:イベント無し
@author
-------------------------------------------------*/
TYPE_ERROR EventControl::GetEvent(uint32_t* pEventId, const uint8_t** pData, uint32_t* pDataLen)
{
    CtrlData data;

    //タスクID格納先がNULL
    if (pEventId == NULL)
    {
        THROW_FATAL_ERROR_PAR(_pOwnTask->GetTaskName());
    }

    //イベントなし
    if (_pQueue->Count() == 0)
    {
        *pEventId = 0;
        return E_EMP;
    }

    //イベントを返す
    if (E_OK == _pQueue->Get(&data))
    {
        *pEventId = data.EventId;
        *pData = data.pData;
        *pDataLen = data.DataLen;
    }
    else
    {
        THROW_FATAL_ERROR_PROGRAM_LOGIC(_pOwnTask->GetTaskName());
    }

    return E_OK;
}

/*!------------------------------------------------
@brief イベントキューのオーバー検知
@note 　登録されている、イベントの数がlimitの数以上であれば
       true,その他はfalseを返す
@param limit  オーバーのリミット値
@return true:オーバーである,false:オーバーでない
@author
-------------------------------------------------*/
bool EventControl::IsOver(uint32_t limit)
{
    if (_pQueue->Count() >= limit)
    {
        //登録済みイベント数がlimit以上
        return true;
    }
    else
    {
        //登録済みイベント数がlimit未満
        return false;
    }
}
