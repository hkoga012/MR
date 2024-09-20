/*!------------------------------------------------
@brief 状態遷移管理クラス
@note
@param
@return
@author
-------------------------------------------------*/
#include "BaseTask.h"
#include "FatalException.h"

/*!------------------------------------------------
@brief 自分自身のポインタを取得
@note
@param なし
@return　イベント管理クラスポインタ
@author
-------------------------------------------------*/
BaseTask* BaseTask::GetTask(void)
{
    return _myPtr;
}

/*!------------------------------------------------
@brief コンストラクタ
@note
@param MaxState　状態の最大数
@param MaxEvent　イベントの最大数
@param SizeEventQue　イベント格納バッファのサイズ
@param MaxTimer　タイマー最大登録可能数
@param pStateString[]　状態文字列配列先頭ポインタ
@param pEventString[]　イベント文字列配列先頭ポインタ
@return なし
@author
    -------------------------------------------------*/
BaseTask::BaseTask(uint32_t MaxState, uint32_t MaxEvent, uint32_t MaxTimer, uint32_t SizeEventQue, const char *pStateString[], const char* pEventString[])
    :_maxState(MaxState), _maxEvent(MaxEvent), _ppStateString(pStateString), _ppEventString(pEventString), _stateIndex(0), _pStateList(new State* [MaxState]),
    _pState(NULL), _myPtr(this), _nextStateId(0), _isFirstTimeUpdate(true)
{
    //タイマー制御クラスインスタンス作成
    _pTimerControl = new TimerControl<BaseTask>(*this, MaxTimer);

    //タスク用のイベントバッファ領域確保
    _pEventControl = new EventControl(this, SizeEventQue);

    //!< 指定数分の状態作成
    for (uint32_t i = 0; i < _maxState; i++)
    {
        _pStateList[_stateIndex] = new State(this, i, MaxEvent);
        _stateIndex++;
    }

    //ログ用リングバッファインスタンス作成
    _pTaskLog = new RingBuffer<Log>(TASK_LOG_COUNT, RING_W_OVERWRITE);

    //ログ初期化
    memset((uint8_t*)&_log,0,sizeof(Log));

    //起動からの累積実行時間初期化
    _executeTime = 0;
}

/*!------------------------------------------------
@brief 指定IDの状態が登録されているかチェック
@note
@param 状態ID
@return 0:未登録、1:登録済み
@author
-------------------------------------------------*/
bool BaseTask::IsRegistered(uint32_t StateId)
{
    for (uint32_t i = 0; i < _stateIndex; i++)
    {
        if (_pStateList[i]->GetId() == StateId)
        {
            return true;
        }
    }
    return false;
}

/*!------------------------------------------------
@brief 指定の状態へ移行
@note　
@param StateId 状態ID
@return なし
@author
-------------------------------------------------*/
void BaseTask::SetState(uint32_t StateId)
{
    bool changeFlag = false;

    if (_stateIndex == 0)
    {
        THROW_FATAL_ERROR_STATUS(GetTaskName());
    }
    if (!IsRegistered(StateId))
    {
        THROW_FATAL_ERROR_STATUS(GetTaskName());
    }

    //状態が変化する場合はExit処理
    if (_pState != NULL)
    {
        changeFlag = _pState->GetId() != StateId;
        if (changeFlag == true)
        {
            _log.time = this->GetTimerTick();
            _log.PrevStateId = _pState->GetId();
            _log.StateId = StateId;

            _pState->Exit();
        }
    }
    else
    {
        //初回登録時も変化したとみなす
        changeFlag = true;
        _log.time = this->GetTimerTick();
        _log.PrevStateId = 0;
        _log.StateId = StateId;
    }

    //状態更新
    _pState = _pStateList[StateId];

    if (changeFlag == true)
    {
        //ログ記録
        _pTaskLog->Add(_log);
        //状態が変化した場合はEnter処理
        _pState->Enter();
    }
    else
    {
        //状態変化なしでもイベント発生ならログ記録
        if (0 != _log.EventId)
        {
            _pTaskLog->Add(_log);
        }
    }
}

/*!------------------------------------------------
@brief ログ取得
@note　
@param Offset 読出し位置
@return　ログクラス
@author
-------------------------------------------------*/
#ifndef WINDOWS_DEBUG
#pragma GCC diagnostic ignored "-Waggregate-return"
#endif
Log BaseTask::GetLog(uint32_t Offset)
{
    // #### 【注意】 Get関数で渡されているのは 関数内のローカルバッファ。
    // #### ”function returns an aggregate” のメッセージが出ているが、安易にポインタに変更せず、現行ロジックのオブジェクトのコピーを渡すか、大元のGet関数を変えるべき
    return _pTaskLog->Get(Offset);
}
#ifndef WINDOWS_DEBUG
#pragma GCC diagnostic warning "-Waggregate-return"
#endif

/*!------------------------------------------------
@brief ログ格納数取得
-------------------------------------------------*/
uint32_t BaseTask::GetLogCount(void)
{
    return _pTaskLog->Count();
}

/*!------------------------------------------------
@brief 指定状態取得
@note　
@param StateId 状態ID
@return　指定した状態IDに該当する状態クラスポインタ
@author
-------------------------------------------------*/
State* BaseTask::GetState(uint32_t StateId)
{
    if (_stateIndex == 0)
    {
        return NULL;
    }
    if (!IsRegistered(StateId))
    {
        return NULL;
    }
    return _pStateList[StateId];
}
/*!------------------------------------------------
@brief 現在の状態取得
@note　
@return　指定した状態IDに該当する状態クラスポインタ
@author
-------------------------------------------------*/
State* BaseTask::GetState(void)
{
    return _pState;
}
/*!------------------------------------------------
@brief 次に遷移する状態IDを取得
@note
@param
@return
@author
-------------------------------------------------*/
uint32_t BaseTask::GetNextStateId(void)
{
    return _nextStateId;
}

/*!------------------------------------------------
@brief 次に遷移する状態IDをセット
@note
@param　NextStateId　次に遷移する状態ID
@return
@author
-------------------------------------------------*/
void BaseTask::SetNextStateId(const uint32_t NextStateId)
{
    _nextStateId = NextStateId;
}

/*!------------------------------------------------
@brief 現在の状態を更新
@note　
@param なし
@return なし
@author
-------------------------------------------------*/
void BaseTask::Update(void)
{

    TYPE_ERROR err = E_OK;
    uint32_t EventId = 0;
    const uint8_t* pData = NULL;
    uint32_t DataLen = 0;
   
    //状態が未登録なら処理しない
    if (_stateIndex == 0)
    {
        THROW_FATAL_ERROR_STATUS(GetTaskName());
    }

    //初回のみ状態をセット(タスク登録後の初回Update呼出し時のみ動作)
    if (true == _isFirstTimeUpdate)
    {
        _isFirstTimeUpdate = false;
        SetState(_nextStateId);
    }
    
    //イベント取得
    err = _pEventControl->GetEvent(&EventId, &pData, &DataLen);
    if (err == E_OK)
    {
        _log.time = this->GetTimerTick();
        _log.EventId = EventId;
        _log.DataLen = DataLen;
        _log.pData = pData;
    }
    else
    {
        //イベント無しイベントとする
        EventId = 0;
        _log.time = this->GetTimerTick();
        _log.EventId = EventId;
        _log.DataLen = 0;
        _log.pData = NULL;
    }

    //状態更新処理実行
    uint32_t beforeTick = this->GetTimerTick();
    _pState->Update(EventId, pData, DataLen);
    uint32_t afterTick = this->GetTimerTick();
    this->_executeTime += (afterTick - beforeTick); // 実行時間を累積計測

    //次の状態へ変更
    SetState(_nextStateId);
}

/*!------------------------------------------------
@brief イベント送信
@note イベント制御クラスのSetEventをラップ
@param TaskId	送信対象タスクID
@param EventId	イベントID
@param pData イベントデータ格納先頭ポインタ
@param DataLen イベントデータ長
@return なし
@author
-------------------------------------------------*/
void BaseTask::SendEvent(const uint32_t EventId, const uint8_t * pData, uint32_t DataLen)
{
    return _pEventControl->SetEvent(EventId, pData, DataLen);
}

/*!------------------------------------------------
@brief タイマー登録
@note タイマー制御クラスのRegisterをラップ
@param TimerMode タイマー動作モード
@param EventId	イベントID
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR BaseTask::RegisterTimer(const ENUM_TIMER_MODE TimerMode, const uint32_t EventId)
{
    return _pTimerControl->Register(TimerMode, EventId);
}

/*!------------------------------------------------
@brief タイマー開始
@note タイマー制御クラスのStartをラップ
@param EventId	イベントID
@param Msec	待ち時間(ミリ秒)
@return なし
@author
-------------------------------------------------*/
void BaseTask::StartTimer(const uint32_t EventId, const uint32_t Msec)
{
    _pTimerControl->Start(EventId, Msec);
}

/*!------------------------------------------------
@brief タイマー強制停止
@note タイマー制御クラスのStopをラップ
@param EventId	イベントID
@return なし
@author
-------------------------------------------------*/
void BaseTask::StopTimer(const uint32_t EventId)
{
    _pTimerControl->Stop(EventId);
}

/*!------------------------------------------------
@brief タイマー停止中確認
@note タイマー制御クラスのStopをラップ
@param EventId	イベントID
@return なし
@author
-------------------------------------------------*/
bool BaseTask::IsStoppedTimer(const uint32_t EventId)
{
    return _pTimerControl->IsStopped(EventId);
}

/*!------------------------------------------------
@brief タイマーカウント
@note タイマー制御クラスのCountをラップ
@param なし
@return なし
@author
-------------------------------------------------*/
void BaseTask::CountTimer(void)
{
    _pTimerControl->Count();
}

/*!------------------------------------------------
   @brief タイマーカウンタ値取得
   -------------------------------------------------*/
/*!------------------------------------------------
@brief タイマーカウンタ値取得
@note タイマー制御クラスのGetTickをラップ
@param なし
@return タイマーカウンタ値
@author
-------------------------------------------------*/
uint32_t BaseTask::GetTimerTick(void)
{
    return _pTimerControl->GetTick();
}

/*!------------------------------------------------
@brief 状態IDに対応した状態文字列を返す
@note　
@param　StateId　状態ID
@return 状態文字列
@author
-------------------------------------------------*/
const char* BaseTask::GetStateString(uint32_t StateId)
{
    if (StateId >= _maxState)
    {
        return NULL;
    }
    return _ppStateString[StateId];
}
/*!------------------------------------------------
@brief イベントIDに対応したイベント文字列を返す
@note　
@param　EventId　イベントID
@return イベント文字列
@author
-------------------------------------------------*/
const char* BaseTask::GetEventString(uint32_t EventId)
{
    if (EventId >= _maxEvent)
    {
        return NULL;
    }
    return _ppEventString[EventId];
}

/*!------------------------------------------------
@brief イベントの最大数を返す
@note　
@param　なし
@return イベントの最大数
@author
-------------------------------------------------*/
uint32_t BaseTask::GetEventMax(void)
{ 
    return _maxEvent;
};

/*!------------------------------------------------
@brief 状態の最大数を返す
@note　
@param　なし
@return 状態の最大数
@author
-------------------------------------------------*/
uint32_t BaseTask::GetStateMax(void)
{ 
    return _maxState; 
};

/*!------------------------------------------------
@brief 時間経過を返す
@note  startTime：開始時間(ms)
       time:指定する経過時間(ms)
@param　なし
@return 0: 未だ過ぎていない　1: 時間が経過
@author
-------------------------------------------------*/
uint32_t BaseTask::IsTimerIsExpired(uint32_t startTime, uint32_t time)
{
    uint32_t tickNow = GetTimerTick();
    if(startTime > tickNow)
    {
        //tickNowの値がuint32の最大値 0xffffffffを超えると
        //0に戻る為、補正処理を行う
        startTime -= 0x80000000;
        tickNow += 0x80000000;
    }

    if((tickNow-startTime) < time)
    {
        //未だ過ぎていない
        return 0;
    }
    else
    {
        //時間が経過
        return 1;
    }
};

/*!------------------------------------------------
@brief イベントキューのオーバー検知
@note 　登録されている、イベントの数がlimitの数以上であれば
       true,その他はfalseを返す
@param limit  オーバーのリミット値
@return true:オーバーである,false:オーバーでない
@author
-------------------------------------------------*/
bool BaseTask::IsOver(uint32_t limit)
{
    return _pEventControl->IsOver(limit);
}
