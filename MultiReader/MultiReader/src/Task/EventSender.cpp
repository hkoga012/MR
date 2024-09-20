/*!------------------------------------------------
@brief イベント・応答・送信タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"
#include "../version.h"


extern "C" void CallbackUart0_Rs485DeControl(uint8_t isDeOn);


/*!------------------------------------------------
@brief RS485 DE 制御コールバック(r_sci_uart.c から呼ばれる)
@param isDeOn 1:ON 0:OFF
@return 無し
@author
-------------------------------------------------*/
static uint8_t ___Rs485DeControlError = 0;

extern "C" void CallbackUart0_Rs485DeControl(uint8_t isDeOn)
{
    static uint8_t isDeOnBefore = 0;
    if(isDeOnBefore != isDeOn)
    {
        isDeOnBefore = isDeOn;
        TYPE_ERROR err = G_pDrvIOPort->Write(RS485_DE, isDeOn ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
        if(err != E_OK)
        {
            ___Rs485DeControlError = 1;
        }
    }
}

/*!------------------------------------------------
@brief コンストラクタ
-------------------------------------------------*/
EventSender::EventSender()
	:BaseTask(ST_EVTSND_MAX, EVT_EVTSND_MAX, TIMER_EVTSND_MAX, EVTSND_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_ONESHOT, EVT_EVTSND_TMOUT_RESEND_INTERVAL);
	this->RegisterTimer(TIMER_ONESHOT, EVT_EVTSND_REQ_RESET_DATA_POWER_TIMER);

	//停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_EVTSND_DORMANT);
	st->AddEventHandler(EVT_EVTSND_START, Dormant_Start);	//!< 開始

	//送信指示待ちへ各ハンドラ登録
	st = this->GetState(ST_EVTSND_WAIT_ORDER);
	st->AddEventHandler(EVT_EVTSND_REQ_READ_DATA, WaitOrder_ReqReadData);						//!< 読み取りデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_POWER, WaitOrder_ReqResetData);               //!< リセットデータ送信指示-電源投入時
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_INIT, WaitOrder_ReqResetData);				//!< リセットデータ送信指示-イニシャルコマンド受信時
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_STATUS, WaitOrder_ReqResetData);				//!< リセットデータ送信指示-ステータス要求コマンド受信時
	st->AddEventHandler(EVT_EVTSND_REQ_MEMORY_DATA, WaitOrder_ReqMemoryData);					//!< メモリデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_VER_DATA, WaitOrder_ReqVerData);							//!< バージョン・チェックサムデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_VER_INFO_DATA, WaitOrder_ReqVerInfoData);                            //!< バージョン情報送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_POWER_TIMER, WaitOrder_ReqResetDataPowerTimer);          //!< 電源投入時のリセットデータ送信遅延タイマー

	//送信中（正常送信、衝突検知の判定待ち）へ各ハンドラ登録
	st = this->GetState(ST_EVTSND_SENDING);
	st->AddEventHandler(EVT_EVTSND_IDLE, Sending_Idle);											//!< Idle
	st->AddEventHandler(EVT_EVTSND_REQ_READ_DATA, Sending_ReqReadData);							//!< 読み取りデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_INIT, Sending_ReqResetData);					//!< リセットデータ送信指示-イニシャルコマンド受信時
	st->AddEventHandler(EVT_EVTSND_REQ_RESET_DATA_STATUS, Sending_ReqResetData);				//!< リセットデータ送信指示-ステータス要求コマンド受信時
	st->AddEventHandler(EVT_EVTSND_REQ_MEMORY_DATA, Sending_ReqMemoryData);						//!< メモリデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_VER_DATA, Sending_ReqVerData);							//!< バージョン・チェックサムデータ送信指示
	st->AddEventHandler(EVT_EVTSND_REQ_VER_INFO_DATA, Sending_ReqVerInfoData);                  //!< バージョン情報送信指示

	//再送待ち（リーダアドレスに応じて待ち時間が変わる）へ各ハンドラ登録
	st = this->GetState(ST_EVTSND_WAIT_RESEND);
	st->AddEnterHandler(WaitResend_Enter);														//!< Enter 再送インターバルタイマースタート、送信回数の管理
	st->AddEventHandler(EVT_EVTSND_TMOUT_RESEND_INTERVAL, WaitResend_TmoutResendInterval);		//!< 再送インターバル経過通知

	//送信済みデータバッファの初期化
	ResetSendedDataBuf();
	//再送信のカウント変数初期化
	_SendCounter = 0;

}

/*!------------------------------------------------
@brief 停止状態 － 開始イベント処理
@note  電源投入時のリセットデータ送信タイマースタート
　　　　　　データ衝突回避の為、(アドレス＋１)×100(ms)待機した後送信させる
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen : 
@return 無し
@author
-------------------------------------------------*/
void EventSender::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;
	EventSender* pMyTask = (EventSender*)pOwnTask;
	// 送信指示待ちに遷移
    pMyTask->SetNextStateId(ST_EVTSND_WAIT_ORDER);
    //内部イベント：電源投入時のリセットデータ送信
	G_pEventSender->SendEvent(EVT_EVTSND_REQ_RESET_DATA_POWER);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － 読取データを送信
@note　送信中状態に遷移
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqReadData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない

	//読取データの送信処理
	pMyTask->SendReqReadData(pData);
	//	送信中に遷移
	pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － リセットデータを送信
@note　送信中状態に遷移
       ステータス要求コマンド受信時 
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqResetData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	//リセットデータ送信処理
	switch (EventId)
	{
	    case EVT_EVTSND_REQ_RESET_DATA_POWER:
            //電源投入時
	        pMyTask->StartTimer(EVT_EVTSND_REQ_RESET_DATA_POWER_TIMER, (G_pDataManager->GetReaderAddress()+1)*100);
            return;
	    case EVT_EVTSND_REQ_RESET_DATA_INIT:
	        if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない
            //イニシャルコマンド受信時
	        pMyTask->SendReqResetData(0x34);
	        break;
	    case EVT_EVTSND_REQ_RESET_DATA_STATUS:
	        if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない
	        //ステータス要求コマンド受信時
	        pMyTask->SendReqResetData(0x30);
	        break;
	}
	//	送信中に遷移
	pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － メモリデータを送信
@note　送信中状態に遷移
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : メモリデータ要求のデータ部ポインタ
@param DataLen : メモリデータ要求のデータ部の長さ
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqMemoryData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない

	//メモリデータ送信処理
	pMyTask->SendReqMemoryData(pData);

	//	送信中に遷移
	pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － ver+チェックサムを送信
@note　Ver+チェックサムデータを送信する
       送信中状態に遷移
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqVerData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない

	//ver+チェックサムデータ送信
	pMyTask->SendReqVerData();

	//	送信中に遷移
	pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － バージョン情報を送信
@note　バージョン情報を送信
       送信中状態に遷移
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqVerInfoData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
    EventSender* pMyTask = (EventSender*)pOwnTask;
    if(pMyTask->_isInitCompleted==0){return;}    //初期化未完了の場合は、何もしない

    //ver+チェックサムデータ送信
    pMyTask->SendReqVerInfoData();

    //  送信中に遷移
    pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信指示待ち状態 － タイマー通知処理
@note  電源投入後のリセットデータを送信する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen :
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitOrder_ReqResetDataPowerTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
    EventSender* pMyTask = (EventSender*)pOwnTask;
    //リセットデータ送信
    pMyTask->SendReqResetData(0x33);
    //  送信中に遷移
    pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if (pMyTask->_isCollisionDetected == 1)	//衝突した場合
	{
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
		//衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
	}
	else									//衝突していない場合
	{
	    //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
	    //読み取りデータの場合は、イベントを送信
	    pMyTask->SendedDataCheck();
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_ORDER);
	}

}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定、データの送信
@note  ステータス要求コマンド受信時 
       衝突していない場合は、読取データを送信する
	　  衝突した場合は、再送待ちに遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_ReqReadData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if (pMyTask->_isCollisionDetected == 1)	//衝突した場合
	{
		//送信データは破棄=何もしない
		//再送待ち状態に遷移
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
		//衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
	}
	else									//衝突していない場合
	{
	    //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
	    //読み取りデータの場合は、イベントを送信
	    pMyTask->SendedDataCheck();
		//読取データを送信する
		pMyTask->SendReqReadData(pData);
	}
}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定、データの送信
@note　衝突していない場合は、リセットデータを送信する
	　 衝突した場合は、再送待ちに遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_ReqResetData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if (pMyTask->_isCollisionDetected == 1)	//衝突した場合
	{
		//送信データは破棄=何もしない
		//再送待ち状態に遷移
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
        //衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
	}
	else									//衝突していない場合
	{
	    //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
	    //読み取りデータの場合は、イベントを送信
	    pMyTask->SendedDataCheck();
		//リセットデータ送信処理
		if (EventId == EVT_EVTSND_REQ_RESET_DATA_INIT)
		{
			//イニシャルコマンド受信時
			pMyTask->SendReqResetData(0x34);
		}
		else
		{
			//ステータス要求コマンド受信時
			pMyTask->SendReqResetData(0x30);
		}
	}
}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定、データの送信
@note　衝突していない場合は、メモリデータを送信する
	　 衝突した場合は、再送待ちに遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_ReqMemoryData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if (pMyTask->_isCollisionDetected == 1)	//衝突した場合
	{
		//送信データは破棄=何もしない
		//再送待ち状態に遷移
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
        //衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
	}
	else									//衝突していない場合
	{
	    //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
	    //読み取りデータの場合は、イベントを送信
	    pMyTask->SendedDataCheck();
		//メモリデータ送信処理
		pMyTask->SendReqMemoryData(pData);
	}
}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定、データの送信
@note　衝突していない場合は、ver+チェックサムデータを送信する
	　 衝突した場合は、再送待ちに遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_ReqVerData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
	if (pMyTask->_isCollisionDetected == 1)	//衝突した場合
	{
		//送信データは破棄=何もしない
		//再送待ち状態に遷移
		pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
        //衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
	}
	else									//衝突していない場合
	{
	    //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
	    //読み取りデータの場合は、イベントを送信
	    pMyTask->SendedDataCheck();
		//ver+チェックサムデータを送信する
		pMyTask->SendReqVerData();
	}
}

/*!------------------------------------------------
@brief 送信中状態 － 衝突の判定、データの送信
@note　衝突していない場合は、バージョン情報を送信する
    　 衝突した場合は、再送待ちに遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::Sending_ReqVerInfoData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
    EventSender* pMyTask = (EventSender*)pOwnTask;
    if (pMyTask->_isCollisionDetected == 1) //衝突した場合
    {
        //送信データは破棄=何もしない
        //再送待ち状態に遷移
        pMyTask->SetNextStateId(ST_EVTSND_WAIT_RESEND);
        //衝突検知変数リセット
        pMyTask->ClearCollisionDetected();
    }
    else                                    //衝突していない場合
    {
        //正常送信の場合は累積回数の対象データかどうかチェックし、対象であれば累積回数を＋１する
        //読み取りデータの場合は、イベントを送信
        pMyTask->SendedDataCheck();
        //バージョン情報を送信する
        pMyTask->SendReqVerInfoData();
    }
}

/*!------------------------------------------------
@brief 再送待ち状態 － Enter処理
@note　 計３回送信して全て衝突した場合は再起動する
       ３回未満は再送インターバルを考慮したタイマーをスタートする
       インターバル=アドレス*20ms
@param pOwnTask 自タスクオブジェクトのポインタ
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitResend_Enter(BaseTask* pOwnTask)
{
	EventSender* pMyTask = (EventSender*)pOwnTask;
	pMyTask->_SendCounter++;
    if (pMyTask->_SendCounter < 3)
    {
        //累積回数：データ衝突回数を＋１
        G_pDataManager->AddEventCount(EVENT_COLLISION);
        //再送インターバルタイマースタート
        pMyTask->StartTimer(EVT_EVTSND_TMOUT_RESEND_INTERVAL, G_pDataManager->GetReaderAddress()*20);
    }
    else
    {
        //送信回数３回(３連続衝突)
        //累積回数：データ衝突回数を＋１
        G_pDataManager->AddEventCount(EVENT_COLLISION);
        //累積回数：送信データ衝突リトライオーバの回数を＋１
        G_pDataManager->AddEventCount(EVENT_SENDTIMEOUT);
        //エラーイベント：衝突リトライオーバフラグを立てる
        G_pDataManager->SetErrorFlag(ERRORFLAG_HOST_PRI_OFFSET,ERRORFLAG_HOST_COLLISION);
        //再起動
        G_pDataManager->SystemSaveAndReboot();
    }
}

/*!------------------------------------------------
@brief 再送待ち状態 － タイマー通知イベント処理
@note　データを再送信する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return 無し
@author
-------------------------------------------------*/
void EventSender::WaitResend_TmoutResendInterval(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	EventSender* pMyTask = (EventSender*)pOwnTask;
    //データを再送する
    pMyTask->SendData(pMyTask->_SendedDataBufLen);
    pMyTask->SetNextStateId(ST_EVTSND_SENDING);
}

/*!------------------------------------------------
@brief 上位に読取データを送信する
@note　読取データのデータ部を送信済みデータバッファに格納する
       データ送信カウンタをリセットする
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendReqReadData(const uint8_t* pData)
{
    G_pDebugCommand->SetDebugTime(4);

	//送信済みデータバッファを初期化する
	ResetSendedDataBuf();
	//製品識別コード(固定：0x51)　adrs:[0]
	_SendedDataBuf[0] = 0x51;
	//読み取ったデータの識別用コード 
	_SendedDataBuf[1] = *pData;
	pData++;
	//読み取りデータ 2～17
	for (uint8_t i = 2; i <= 17; i++)
	{
		_SendedDataBuf[i] = *pData;
		pData++;
	}
	//読み取りデータ2 18～33
	for (uint8_t i = 18; i <= 33; i++)
	{
	    _SendedDataBuf[i] = *pData;
        pData++;
	}
	//DLE(固定)
	_SendedDataBuf[34] = 0x10;
	//未使用(固定)
	_SendedDataBuf[35] = 0x30;
	//ボタン情報(固定)
	_SendedDataBuf[36] = 0x30;
	//送信済みデータ長変数に長さをセット
	_SendedDataBufLen = DT_LEN_REQ_READ;
	//上位システムにデータ送信
	this->SendData(DT_LEN_REQ_READ);
	//データ送信カウンタ
	_SendCounter = 0;
}

/*!------------------------------------------------
@brief 上位にリセットデータを送信する
@note　リセットデータのデータ部を送信済みデータバッファに格納する
       データ送信カウンタをリセットする
@param Factor　リセットデータ送信要因 
	0x30：ステータス要求コマンド受信時
	0x33：リーダー電源投入時
	0x34：イニシャルコマンド受信時
@return 
@author
-------------------------------------------------*/
void EventSender::SendReqResetData(uint8_t Factor)
{
	//送信済みデータバッファを初期化する
	ResetSendedDataBuf();
	//製品識別コード(固定：0x51)　adrs:[0]
	_SendedDataBuf[0] = 0x51;
	//データ送信要因　adrs:[1]
	_SendedDataBuf[1] = Factor;
	//DLE（固定：0x10）
	_SendedDataBuf[2] = 0x10;
	//未使用（固定：0x30）
	_SendedDataBuf[3] = 0x30;
	//DipSW1情報（上位+下位）
	ClsCommon::I2HexAsciiByte(G_pDataManager->GetInfoDipSw1(), (char*) & _SendedDataBuf[4], (char*) & _SendedDataBuf[5]);
	//DipSW2情報（上位+下位）
	ClsCommon::I2HexAsciiByte(G_pDataManager->GetInfoDipSw2(), (char*)&_SendedDataBuf[6], (char*)&_SendedDataBuf[7]);

	//LED1～4の点灯状態
	const uint8_t* pled = G_pDataManager->GetLedStatusForInit();
	memcpy(&_SendedDataBuf[8], pled, SZ_LED_BUF_SIZE);

	//バージョン
    _SendedDataBuf[12] = (MAJOR_VERSION<=9) ? ('0' + MAJOR_VERSION) : ('A' + (MAJOR_VERSION - 10)); //<! 上位
    _SendedDataBuf[13] = (MINER_VERSION<=9) ? ('0' + MINER_VERSION) : ('A' + (MINER_VERSION - 10)); //<! 下位

    //リーダー状態
	_SendedDataBuf[14] = G_pDataManager->GetReaderHwStatus();
	//送信済みデータ長変数に長さをセット
	_SendedDataBufLen = DT_LEN_REQ_RESET;
	//上位システムにデータ送信
	this->SendData(DT_LEN_REQ_RESET);
	//データ送信カウンタ
	_SendCounter = 0;
}

/*!------------------------------------------------
@brief 上位にメモリデータを送信する
@note　メモリデータのデータ部を送信済みデータバッファに格納する
       データ送信カウンタをリセットする
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendReqMemoryData(const uint8_t* pData)
{
	const uint8_t* pdata = pData;
	uint32_t result = 0; //offset値を格納する
	//送信済みデータバッファを初期化する
	ResetSendedDataBuf();
	//製品識別コード(固定：0x51)
	_SendedDataBuf[0] = 0x51;
	//メンテコード
	_SendedDataBuf[1] = 0xfe;
	//Type(固定値)
	_SendedDataBuf[2] = 0x50;
	//未使用（固定：0x30）
	_SendedDataBuf[3] = 0x30;
	//Data:32byteを送信する、先頭はCommandReceiverから貰うこと

	pdata += 3;// 要求されたオフセットアドレスの位置まで進める
	ClsCommon::AsciiHex2Uint32((const char*)pdata, 8, &result);
	//メモリバッファーの先頭ポインタを格納
	const uint8_t* MemoryAdrs = G_pDataManager->GetRomDataBuf();
	//メモリバッファの先頭アドレスをオフセットする
	MemoryAdrs += result;
	//送信済みデータバッファに32byte格納：要求データ数は10H(16Byte)で固定で考える。
	for (uint8_t index = 4; index < 36; index+=2)
	{
	    ClsCommon::I2HexAsciiByte(*MemoryAdrs,(char*)&_SendedDataBuf[index],(char*)&_SendedDataBuf[index+1]);
		MemoryAdrs++;
	}
	//送信済みデータ長変数に長さをセット
	_SendedDataBufLen = DT_LEN_REQ_MEMORY;
	//上位システムにデータ送信
	this->SendData(DT_LEN_REQ_MEMORY);
	//データ送信カウンタ
	_SendCounter = 0;
}

/*!------------------------------------------------
@brief 上位にVer+チェックサムデータを送信する
@note　Ver+チェックサムデータのデータ部を送信済みデータバッファに格納する
       データ送信カウンタをリセットする
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendReqVerData()
{
	//送信済みデータバッファを初期化する
	ResetSendedDataBuf();
	//製品識別コード(固定：0x51)
	_SendedDataBuf[0] = 0x51;
	//メンテコード（固定：0xFE）
	_SendedDataBuf[1] = 0xFE;
	//Type（固定：0x20）
	_SendedDataBuf[2] = 0x20;
	//予備（固定値：0x30）
	_SendedDataBuf[3] = 0x30;
	//Version
	ClsCommon::I2HexAsciiByte(MAJOR_VERSION, (char*)&_SendedDataBuf[4], (char*)&_SendedDataBuf[5]);//上位２桁
	ClsCommon::I2HexAsciiByte(MINER_VERSION, (char*)&_SendedDataBuf[6], (char*)&_SendedDataBuf[7]);//下位２桁
	//CheckSum(4桁)
	ClsCommon::I2HexAscii(G_pDataManager->GetCheckSum(), 4, (char*)&_SendedDataBuf[8]);
	//送信済みデータ長変数に長さをセット
	_SendedDataBufLen = DT_LEN_REQ_VER;
	//上位システムにデータ送信
	this->SendData(DT_LEN_REQ_VER);
	//データ送信カウンタ
	_SendCounter = 0;
}

/*!------------------------------------------------
@brief バージョン情報を送信する
@note　バージョン情報のデータ部を送信済みデータバッファに格納する
       データ送信カウンタをリセットする
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendReqVerInfoData()
{
    //送信済みデータバッファを初期化する
    ResetSendedDataBuf();
    //製品識別コード(固定：0x51)
    _SendedDataBuf[0] = 0x51;
    //メンテコード（固定：0xFE）
    _SendedDataBuf[1] = 0xFE;
    //Type:バージョン情報取得('C')
    _SendedDataBuf[2] = 'C';
    //予備（固定値：0x30）
    _SendedDataBuf[3] = 0x30;
    //バージョン情報
    const char* p = G_pDebugCommand->GetVerInfo();  //VersionInfoの先頭アドレス
    memcpy(&_SendedDataBuf[4],p,14);    //14はNull無しVERSION_INFOの長さ（固定）
    //送信済みデータ長変数に長さをセット
    _SendedDataBufLen = DT_LEN_REQ_VER_INFO;
    //上位システムにデータ送信
    this->SendData(DT_LEN_REQ_VER_INFO);
    //データ送信カウンタ
    _SendCounter = 0;
}

/*!------------------------------------------------
@brief 送信済みデータバッファの長さを返す
@note　
	送信中状態の時以外は0を返す
	送信中状態の場合は送信済みデータバッファの長さを返す
@return 長さ
@author
-------------------------------------------------*/
uint32_t EventSender::GetLatestSendLen()
{
	//送信中状態以外は非公開
	if (this->GetState()->GetId() != ST_EVTSND_SENDING)
	{
		return 0;
	}
	else
	{
		//送信中状態の時は長さを公開
		return _SendedDataBufLen;
	}
}

/*!------------------------------------------------
@brief STX,ETX付与,BCC計算後　送信
@note　 送信済みデータバッファにSTX+ETX+BCCを付与して送信
@param DataLen  送信済みデータバッファの長さ
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendData(uint8_t DataLen)
{
	const uint8_t ASCII_STX = 0x02;
	const uint8_t ASCII_ETX = 0x03;
	uint8_t adrsDataBuf[2];
	ClsCommon::ZeroSuppress(G_pDataManager->GetReaderAddress(), 2,(char*)&adrsDataBuf,sizeof(adrsDataBuf));

	//BCC演算
	uint8_t bcc = 0;
	bcc = bcc ^ adrsDataBuf[0];
	bcc = bcc ^ adrsDataBuf[1];
	for (uint32_t index = 0;index < DataLen;index++)
	{
		bcc= bcc ^_SendedDataBuf[index];
	}
	bcc = bcc ^ ASCII_ETX;

	//データ送信
	SendPrintSci(&ASCII_STX,1);						// STX
	SendPrintSci(adrsDataBuf, sizeof(adrsDataBuf));	// アドレス
	SendPrintSci(_SendedDataBuf, DataLen);			// データ部
	SendPrintSci(&ASCII_ETX,1);						// ETX
	SendPrintSci(&bcc,1);							// BCC

#ifndef WINDOWS_DEBUG

    if(G_pDrvUART_RS485->IsSending() == true)
    {
    //送信中の場合→（受信側で衝突確認を行う為）送信完了までブロックする
    //タイムアウトは設計上50msだが、timer割り込みが1ms間隔の為、50msを保証する為に _timeOutVal を51msにしている
      uint32_t startTime = GetTimerTick();
      //タイムアウト処理
      while(IsTimerIsExpired(startTime,_timeOutVal) == 0)
      {
         if(G_pDrvUART_RS485->IsSending() == false)
         {
             //送信終了なら抜ける
             break;
         }
      }
    }

    if(___Rs485DeControlError)
    {
        //RS485：プライマリDE異常フラグを立てる
        G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_485_OFFSET,ERRORFLAG_PORT_485_DE_A);
        ___Rs485DeControlError = 0;
    }

    // フレーミングエラー評価用デバッグ出力
#ifdef USE_PRINT_DEBUG_FER
    {
        static uint32_t debugSendCount = 0;
        debugSendCount ++;

        DebugCommand::SendPrintSci("FER=");
        ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, G_pDataManager->GetCount_EVENT_ERROR_HOST_PRI_FER());
        DebugCommand::SendPrintSci(" Send Count=");
        ClsCommon::DebugPrintNum(DebugCommand::SendPrintSci, debugSendCount);
        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " tick=", this->GetTimerTick());
    }
#endif

#endif

    G_pDebugCommand->SetDebugTime(5);
}
/*!------------------------------------------------
@brief シリアルポート出力
@note　Windows、RAマイコン実行時で処理を変更すること
@param str 出力文字列
@return 無し
@author 
-------------------------------------------------*/
void EventSender::SendPrintSci(const uint8_t* pData,uint8_t Len)
{
#ifdef WINDOWS_DEBUG
	//Windows動作時
	WinSerialPort::Write(COM_PORT0_RS485,pData,Len);

	//std::cout << str;
#else
	//マイコン動作時
    G_pDrvUART_RS485->Write((uint8_t*)pData, Len);
#endif
}

/*!------------------------------------------------
@brief 正常送信済みデータを確認し、累積回数の更新+イベントを送信する
@note　 正常送信済みデータから、対象項目かどうか判断し、対象であればその項目の累積回数を＋１
       正常送信済みデータが読取データだった場合は、読取データ送信完イベントを送信
@param 無し
@return 無し
@author
-------------------------------------------------*/
void EventSender::SendedDataCheck()
{
    //各対象の累積回数を＋１する(対象：読み取りデータQR＋NFC、リセットデータ)
    //対象でなければ何もしない
    switch(_SendedDataBufLen)//データ部の長さで対象データを判定する
    {
        case(DT_LEN_REQ_READ)://読み取りデータ
            //読み取りデータの場合はCommandReceiverに読取データ送信完イベント送信
            G_pCommandReceiver->SendEvent(EVT_CMDRCV_RD_DATA_SENDED);

            if(_SendedDataBuf[1] == 0x35)//QRデータ([1]はアドレスを識別用コードの位置に合わせている)
            {
                //累積回数：QRデータ送信回数＋１
                G_pDataManager->AddEventCount(EVENT_SENDQR);
            }
            else
            {
                //NFCデータ
                //累積回数：カード ID 送信回数＋１
                G_pDataManager->AddEventCount(EVENT_SENDCARD);
            }
            break;
        case(DT_LEN_REQ_RESET)://リセットデータ
            //初期化フラグが未完了の時にリセットデータを正常送信した場合、初期化完了フラグをセット
            if(_isInitCompleted==0)
            {
                _isInitCompleted=1;
                //電源投入時のリセットデータ送信完了通知
                G_pDataManager->SendEvent(EVT_DATA_RESETDATA_SENDED);
            }
            //累積回数：リセットデータ 送信回数＋１
            G_pDataManager->AddEventCount(EVENT_SENDRESET);
            break;
    }
}
