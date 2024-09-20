/*!------------------------------------------------
@brief 出力ポート制御タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

OutputController::OutputController()
	:BaseTask(ST_OUTPUT_MAX, EVT_OUTPUT_MAX, TIMER_OUTPUT_MAX, OUTPUT_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_FOREVER, EVT_OUTPUT_NOTICE_TIMER);
	
	//停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_OUTPUT_DORMANT);
	st->AddEventHandler(EVT_OUTPUT_START, Dormant_Start );  //!< 開始
	
	//動作中状態へ各ハンドラ登録
	st = this->GetState(ST_OUTPUT_RUNNING);
	st->AddEventHandler(EVT_OUTPUT_IDLE, Running_Idle );                  //!< イベント無し
	st->AddEventHandler(EVT_OUTPUT_NOTICE_TIMER, Running_NoticeTimer );   //!< タイマー通知
	st->AddEventHandler(EVT_OUTPUT_LED, Running_Led );                    //!< LED更新指示
	st->AddEventHandler(EVT_OUTPUT_BUZZER, Running_Buzzer );              //!< BUZZER更新指示
	st->AddEventHandler(EVT_OUTPUT_AUTODOOR, Running_AutoDoor);           //!< AUTODOOR更新指示
	st->AddEventHandler(EVT_OUTPUT_FW_UPDATING, Running_FwUpdating );     //!< FW更新中通知
    st->AddEnterHandler(Running_Enter);	//!< Enter
	st->AddExitHandler(Running_Exit);	//!< Exit

	for(uint8_t i=0;i<LED_IDX_MAX;i++)
	{
		_ledControl[i].pCurrent = (DATA*)&_ledPattern[0][0];
		_ledControl[i].Command = 0;
		_ledControl[i].PatternIndex = 0;
		_ledControl[i].PrevIsDetectedCard = false;
		_ledControl[i].WorkData.Ctrl = IO_OFF;
		_ledControl[i].WorkData.Count = 0;
		_ledControl[i].LastData.Ctrl = IO_OFF;
		_ledControl[i].LastData.Count = 0;
		_ledControl[i].EndCommand = 0;
	}
	_isDetectedCard = false;
	_DetectedCardTimeout = 0;

	_buzzerControl.pCurrent = (DATA*)&_buzzerPattern[0][0];
	_buzzerControl.Command = 0;
	_buzzerControl.PatternIndex = 0;
	_buzzerControl.WorkData.Ctrl = IO_OFF;
	_buzzerControl.WorkData.Count = 0;
	_buzzerControl.Enable = false;
	
	_autoDoorControl.pCurrent = (DATA*)&_autoDoorPattern[0][0];
	_autoDoorControl.Command = 0;
	_autoDoorControl.PatternIndex = 0;
	_autoDoorControl.WorkData.Ctrl = IO_OFF;
	_autoDoorControl.WorkData.Count = 0;
	_autoDoorControl.Enable = false;

#ifdef WINDOWS_DEBUG
	_pSimPanel = new SimPanelClient();
#endif
}

/*!------------------------------------------------
@brief 停止状態 － 開始イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen : 
@return なし
@author
-------------------------------------------------*/
void OutputController::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    TYPE_ERROR err = E_OK;
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    OutputController* pMyTask = (OutputController*)pOwnTask;

#ifdef WINDOWS_DEBUG
	pMyTask->StartTimer(EVT_OUTPUT_NOTICE_TIMER, 250);	/* Windowsデバッグ時は、間に合わないので周期を遅くする */
#else
	pMyTask->StartTimer(EVT_OUTPUT_NOTICE_TIMER, 50);   /* 50msec 周期タイマー開始*/
#endif

	pMyTask->SetNextStateId(ST_OUTPUT_RUNNING);
#ifndef WINDOWS_DEBUG
	PWM *pPwm[]={
         G_pDrvPWM_LED_Left_G,
         G_pDrvPWM_LED_Center_G,
         G_pDrvPWM_LED_Right_G,
         G_pDrvPWM_LED_Frame_R,
         G_pDrvPWM_LED_Frame_G,
         G_pDrvPWM_LED_Frame_B,
         G_pDrvPWM_Buzzer
	};
	uint8_t cnt = sizeof(pPwm)/sizeof(PWM*);
	for(uint8_t i=0;i<cnt;i++)
	{
	    pPwm[i]->SetDuty(0);
	    err = pPwm[i]->Start();
	    if (E_OK != err)
	    {
	        //LEDが使用不可でも運用は可能であるためエラーをthrowしない
	    }
	}

#endif

    // 自動ドアの初期状態としてブレーク信号を出力
    pMyTask->SendBreakForInitAutoDoor();
}


/*!------------------------------------------------
@brief 動作中状態 － Idle処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)pOwnTask; (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
}

/*!------------------------------------------------
@brief 動作中状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_Enter(BaseTask* pOwnTask)
{
	OutputController* pMyTask = (OutputController*)pOwnTask;
	uint8_t Command = (uint8_t)'a';	//起動時コマンド'a'
	int8_t PatIndex = pMyTask->GetLedPatternIndex(Command);

	for(uint8_t i=0;i<LED_IDX_MAX;i++)
	{
		TYPE_LED_CTRL* p = &pMyTask->_ledControl[i];
		p->pCurrent = (DATA*)&pMyTask->_ledPattern[PatIndex][0];
		p->Command = Command;
		p->PatternIndex = 0;
		p->EndCommand = 0;
		p->WorkData = p->pCurrent[p->PatternIndex];
	}

	//DIPSW1-2の状態によってブザー鳴動決定
	pMyTask->_buzzerControl.Enable = G_pDataManager->IsSelFuncBuzzer() == 1 ? true : false;
}

/*!------------------------------------------------
@brief 動作中状態 － Exit処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_Exit(BaseTask* pOwnTask)
{
    (void)pOwnTask; // 未使用引数のワーニング対策
}

/*!------------------------------------------------
@brief 動作中状態 － タイマー通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param pState 現在の状態オブジェクトポインタ
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_NoticeTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	OutputController* pMyTask = (OutputController*)pOwnTask;

	TYPE_ERROR err = E_OK;

	//LED制御
	err = pMyTask->LEDControl();
	if (E_OK != err)
	{
	    //LEDが使用不可でも停止しない
	}

	//ブザー制御
#ifndef DEBUG_NFC_POWER_DIPSW //　DIPSWでアンテナを調整する際は、通常動作では鳴らさない
	err = pMyTask->BuzzerControl();
	if (E_OK != err)
	{
	    //ブザーが使用不可でも停止しない
	}
#endif

	//自動ドア制御
	err = pMyTask->AutoDoorControl();
	if (E_OK != err)
	{
        //自動ドアが使用不可でも停止しない
	}

	//カード検出イベント通知からタイムアウトならLEDのDuty100%制御をOFFする
	if(pMyTask->_DetectedCardTimeout > 0)
	{
	    pMyTask->_DetectedCardTimeout--;
	    if( 0 == pMyTask->_DetectedCardTimeout )
	    {
	        pMyTask->DetectedCard(false);
	    }
	}
}

/*!------------------------------------------------
@brief 動作中状態 －LED動作指示通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param pState 現在の状態オブジェクトポインタ
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_Led(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;  // 未使用引数のワーニング対策
	OutputController* pMyTask = (OutputController*)pOwnTask;
	
	//パラメータエラー
	if(NULL == pData || DataLen != 4)
	{
	    return;	//エラー無視
	}
	
	//LED指示コマンドは先頭からLeft(pData[0])、Center(pData[1])、Right(pData[2])、Frame(pData[3])の順で並んでいること
    for(uint8_t i=0;i<LED_IDX_MAX;i++)
	{
		TYPE_LED_CTRL* pLEDCtrl = &pMyTask->_ledControl[i];
		uint8_t Command = pData[i];
		int8_t PatIndex = pMyTask->GetLedPatternIndex(Command);
		if(PatIndex >= 0)
		{
	        //点灯パターンを更新
			pLEDCtrl->pCurrent = (DATA*)&pMyTask->_ledPattern[PatIndex][0];
			pLEDCtrl->Command = Command;
			pLEDCtrl->PatternIndex = 0;
			pLEDCtrl->WorkData = pLEDCtrl->pCurrent[pLEDCtrl->PatternIndex];

		    if((Command >= 'T' && Command <= 'X') || (Command == '0'))
		    {
		    	//常時点灯、常時消灯コマンドはパターン終了時実施コマンドとして保持
				pLEDCtrl->EndCommand = Command;
		    }
		}
	}
}

/*!------------------------------------------------
@brief 動作中状態 －ブザー動作指示通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param pState 現在の状態オブジェクトポインタ
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_Buzzer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;  // 未使用引数のワーニング対策
	OutputController* pMyTask = (OutputController*)pOwnTask;

	//パラメータエラー
	if (NULL == pData || DataLen != 1)
	{
		return;	//エラー無視
	}

	G_pDebugCommand->SetDebugTime(9);
	G_pDebugCommand->PrintDebugTime();

	//コマンド取得
	uint8_t Command = pData[0];     //指示コマンド

	int8_t PatIndex = pMyTask->GetBuzzerPatternIndex(Command);
	if (PatIndex >= 0)
	{
		//鳴動パターンを更新
		pMyTask->_buzzerControl.pCurrent = (DATA*)&pMyTask->_buzzerPattern[PatIndex];
		pMyTask->_buzzerControl.Command = Command;   //指示コマンドIDを記憶
		pMyTask->_buzzerControl.PatternIndex = 0;    //パターン更新時はパターンの先頭から開始
		pMyTask->_buzzerControl.WorkData = pMyTask->_buzzerControl.pCurrent[0];	//先頭のパターンを取得
	}
}

/*!------------------------------------------------
@brief 動作中状態 －ドア動作指示通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param pState 現在の状態オブジェクトポインタ
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_AutoDoor(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;  // 未使用引数のワーニング対策
	OutputController* pMyTask = (OutputController*)pOwnTask;
	
	//パラメータエラー
	if (NULL == pData || DataLen != 1)
	{
		return;	//エラー無視
	}

	//コマンド取得
	uint8_t Command = pData[0];     //指示コマンド

	int8_t PatIndex = pMyTask->GetAutoDoorPatternIndex(Command);
	if (PatIndex >= 0)
	{
	    //開閉指示回数記録
	    if( Command == (uint8_t)'0' )
	    {
	        //閉回数
	        G_pDataManager->AddEventCount(EVENT_RELAYCLOSE);
	    }
	    else
	    {
	        //開回数
	        G_pDataManager->AddEventCount(EVENT_RELAYOPEN);
	    }

		//開閉パターンを更新
		pMyTask->_autoDoorControl.pCurrent = (DATA*)&pMyTask->_autoDoorPattern[PatIndex];
		pMyTask->_autoDoorControl.Command = Command;   //指示コマンドIDを記憶
		pMyTask->_autoDoorControl.PatternIndex = 0;    //パターン更新時はパターンの先頭から開始
		pMyTask->_autoDoorControl.WorkData = pMyTask->_autoDoorControl.pCurrent[0];	//先頭のパターンを取得
	}
}

/*!------------------------------------------------
@brief 動作中状態 －FW更新中通知イベント処理
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param pState 現在の状態オブジェクトポインタ
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void OutputController::Running_FwUpdating(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    OutputController* pMyTask = (OutputController*)pOwnTask;

    // 自動ドアの初期状態としてブレーク信号を出力
    pMyTask->SendBreakForInitAutoDoor();

    uint8_t Command;
    int8_t PatIndex;

    //LED表示変更
    for(uint8_t i=0;i<LED_IDX_MAX;i++)
    {
        TYPE_LED_CTRL* p = &pMyTask->_ledControl[i];
        if(i==LED_IDX_FRAME)    //FrameのみFW更新用LED表示、他LEDはOFF
        {
            Command = (uint8_t)'b'; //FW更新時コマンド'b'
            PatIndex = pMyTask->GetLedPatternIndex(Command);
            p->pCurrent = (DATA*)&pMyTask->_ledPattern[PatIndex][0];
            p->Command = Command;
            p->PatternIndex = 0;
            p->EndCommand = 0;
            p->WorkData = p->pCurrent[p->PatternIndex];
        }
        else
        {
            Command = (uint8_t)'0'; //消灯コマンド'0'
            PatIndex = pMyTask->GetLedPatternIndex(Command);
            p->pCurrent = (DATA*)&pMyTask->_ledPattern[PatIndex][0];
            p->Command = Command;
            p->PatternIndex = 0;
            p->EndCommand = 0;
            p->WorkData = p->pCurrent[p->PatternIndex];
        }
    }
}

/*!------------------------------------------------
@brief カード検出通知イベント処理
@note LED4のみ制御対象
@param IsDetect FeliCa検出フラグ(true:検出、false：未検出)
@return なし
@author
-------------------------------------------------*/
void OutputController:: DetectedCard(bool IsDetect)
{
    TYPE_LED_CTRL* pLEDCtrl = &_ledControl[LED_IDX_FRAME];

    //起動点滅中はFeliCaを検出してもDuty変更しない
    if('a' == pLEDCtrl->Command)
    {
        return;
    }

    if(IsDetect)
    {
        G_pDebugCommand->SetDebugTime(0);      // 開始
    }

    _isDetectedCard = IsDetect;
    _DetectedCardTimeout = (true == IsDetect ? DETECT_CARD_TIMEOUT : 0);

    //Frameのデュティー比 を更新する
    if (_isDetectedCard != pLEDCtrl->PrevIsDetectedCard)
    {
        pLEDCtrl->PrevIsDetectedCard = _isDetectedCard;
        LEDOutput(LED_IDX_FRAME, pLEDCtrl->LastData.Ctrl, _isDetectedCard);
    }
}

/*!------------------------------------------------
@brief 初期状態として自動ドアにブレーク信号を出力
@note
@return なし
@author
-------------------------------------------------*/
void OutputController::SendBreakForInitAutoDoor()
{
    //ブレイク信号を出力
    uint8_t Command =(uint8_t)'0';   //ブレイク信号コマンド
    int8_t PatIndex = GetAutoDoorPatternIndex(Command);
    _autoDoorControl.pCurrent = (DATA*)&_autoDoorPattern[PatIndex];
    _autoDoorControl.Command = Command;   //指示コマンドIDを記憶
    _autoDoorControl.PatternIndex = 0;    //パターン更新時はパターンの先頭から開始
    _autoDoorControl.WorkData = _autoDoorControl.pCurrent[0]; //先頭のパターンを取得
}

/*!------------------------------------------------
@brief LED制御
@note　
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR OutputController::LEDControl(void)
{
	TYPE_ERROR err = E_OK;

    for(uint8_t LedIndex=0; LedIndex<LED_IDX_MAX; LedIndex++)
    {
		TYPE_LED_CTRL* pLEDCtrl = &_ledControl[LedIndex];

		if (IO_END == pLEDCtrl->WorkData.Ctrl)
		{
			//終端(何もしない)
		}
		else if (IO_KEP == pLEDCtrl->WorkData.Ctrl)
		{
			//現状維持(何もしない)
		}
		else
		{
			if (pLEDCtrl->WorkData.Count > 0)
			{
				//初回だけIO制御
				if (pLEDCtrl->WorkData.Count == pLEDCtrl->pCurrent[pLEDCtrl->PatternIndex].Count)
				{
					/////////////
					//出力指示
					/////////////
					pLEDCtrl->LastData = pLEDCtrl->WorkData;	//最終指示データを保持
					err = LEDOutput(LedIndex, pLEDCtrl->WorkData.Ctrl, _isDetectedCard);
#ifdef WINDOWS_DEBUG
					//std::cout << "LED ON/OFF Ctrl:" << (uint32_t)pLEDCtrl->WorkData.Ctrl << " LedIndex:" << (uint32_t)LedIndex << " PatternIndex:" << (uint32_t)pLEDCtrl->PatternIndex << " Count:" << (uint32_t)pLEDCtrl->WorkData.Count << "\r\n";
#endif /* WINDOWS_DEBUG */
				}
#ifdef WINDOWS_DEBUG
				//std::cout << "LED[" << (uint32_t)LedIndex << "] Count:" << (uint32_t)pLEDCtrl->WorkData.Count << "\r\n";
#endif /*WINDOWS_DEBUG*/
				pLEDCtrl->WorkData.Count--;
				if (pLEDCtrl->WorkData.Count == 0)
				{
					//次へ
					if (pLEDCtrl->PatternIndex < (LED_PATTERN_MAX - 1))
					{
						pLEDCtrl->PatternIndex++;
					}

					pLEDCtrl->WorkData = pLEDCtrl->pCurrent[pLEDCtrl->PatternIndex];

					if (IO_END == pLEDCtrl->WorkData.Ctrl)
					{
						//パターン終了時実施コマンド取得
						uint8_t Command = pLEDCtrl->EndCommand;
						int8_t PatIndex = GetLedPatternIndex(Command);
						if (PatIndex >= 0)
						{
							pLEDCtrl->PatternIndex = 0;
							pLEDCtrl->pCurrent = (DATA*)&_ledPattern[PatIndex][0];
							pLEDCtrl->Command = Command;
							pLEDCtrl->WorkData = pLEDCtrl->pCurrent[pLEDCtrl->PatternIndex];
						}
						else
						{
							//エラー(パターン終了時実施コマンドなし)
						}
#ifdef WINDOWS_DEBUG
						//std::cout << "LED END Ctrl:" << (uint32_t)pLEDCtrl->WorkData.Ctrl << " LedIndex:" << (uint32_t)LedIndex << " PatternIndex:" << (uint32_t)pLEDCtrl->PatternIndex << "\r\n";
#endif /* WINDOWS_DEBUG */
					}
					else if (IO_TOP == pLEDCtrl->WorkData.Ctrl)
					{
						//現在のパターンのままパターンの先頭から実施
						pLEDCtrl->PatternIndex = 0;
						pLEDCtrl->WorkData = pLEDCtrl->pCurrent[pLEDCtrl->PatternIndex];
					}
				}
			}
		}
    }
	
	return err;
}

/*!------------------------------------------------
@brief LED出力判定
@note　
@param LedIndex　LED種別
@param Data　LED出力データ
@param IsDetectCard　カード検出フラグ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR OutputController::LEDOutput(uint8_t LedIndex, ENUM_CTRL Data, bool IsDetectedCard)
{
    TYPE_ERROR err = E_OK;

#ifndef DEBUG_NFC_POWER_DIPSW //　DIPSWでアンテナを調整時、通常動作のLEDは表示させない

#ifdef DEBUG_NOT_USE_OTHER_THAN_LED4
	if(LedIndex != LED_IDX_FRAME){
	    return E_OK;
	}
#endif

	// デバッグ用　DIPSW2-1 ON の場合、カード読取改善調査用にLED4を常に検出状態（Duty 0% or 100%）で表示
    if( G_pDataManager->GetInfoDipSw2() & 0x80 )
    {
        IsDetectedCard = true;
    }

	//出力
	switch(Data)
    {
        case IO_RED:	//! 赤100%点灯
    		err = LEDRGB(LedIndex, 100, 0, 0, IsDetectedCard);
            break;
        case IO_GRN:	//! 緑100%点灯
    		err = LEDRGB(LedIndex, 0, 100, 0, IsDetectedCard);
            break;
        case IO_BLE:	//! 青100%点灯
    		err = LEDRGB(LedIndex, 0, 0, 100, IsDetectedCard);
            break;
        case IO_ORG:	//! 橙(赤100%、緑25%点灯)
            err = LEDRGB(LedIndex, 100, 25, 0, IsDetectedCard);
            break;
        case IO_WHE:	//! 白(赤100%、緑100%、青100%)
    		err = LEDRGB(LedIndex, 100, 100, 100, IsDetectedCard);
            break;
        case IO_OFF:	//! 消灯(赤0%、緑0%、青0%)
    		err = LEDRGB(LedIndex, 0, 0, 0, IsDetectedCard);
            break;
    	case IO_KEP:
    	case IO_END:
    		err = E_OK;	
    		break;
        default:
    		err = E_PAR;
            break;
    }
#endif

	return err;
}

/*!------------------------------------------------
@brief LED出力
@note　
@param LedIndex　LED種別
@param DutyR　LED赤出力デューティ(0-100%)
@param DutyG　LED緑出力デューティ(0-100%)
@param DutyB　LED青出力デューティ(0-100%)
@param IsDetectCard　カード検出フラグ
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR OutputController::LEDRGB(uint8_t LedIndex, uint8_t DutyR, uint8_t DutyG, uint8_t DutyB, bool IsDetectedCard)
{
	TYPE_ERROR err = E_OK;
	
#ifndef WINDOWS_DEBUG
	PWM *pPWM_R = NULL;
	PWM *pPWM_G = NULL;
	PWM *pPWM_B = NULL;
	bsp_io_port_pin_t PortR = LED_Left_R;
	bsp_io_port_pin_t PortB = LED_Left_B;

	if(DutyR > 100 || DutyG > 100 || DutyB > 100)
	{
		return E_PAR;
	}
	///////////////////////
	//出力先決定
	///////////////////////
	switch(LedIndex)
    {
    case LED_IDX_LEFT:
    	//LED_Left
    	PortR = LED_Left_R;
		pPWM_G = G_pDrvPWM_LED_Left_G;
		PortB = LED_Left_B;
    	break;
    case LED_IDX_CENTER:
    	//LED_Center
    	PortR = LED_Center_R;
		pPWM_G = G_pDrvPWM_LED_Center_G;
		PortB = LED_Center_B;
    	break;
    case LED_IDX_RIGHT:
    	//LED_Right
    	PortR = LED_Right_R;
		pPWM_G = G_pDrvPWM_LED_Right_G;
		PortB = LED_Right_B;
    	break;
    case LED_IDX_FRAME:
    	pPWM_R = G_pDrvPWM_LED_Frame_R;
		pPWM_G = G_pDrvPWM_LED_Frame_G;
		pPWM_B = G_pDrvPWM_LED_Frame_B;
    	break;
    default:
    	return E_PAR;
    	break;
    }
#endif /* WINDOWS_DEBUG */

	//FeliCa読取り障害対応
    if(LED_IDX_FRAME == LedIndex)
    {
        if(false == IsDetectedCard)
        {
            //カード未検出時、輝度を半分にする
            DutyR = DutyR/2;
            DutyG = DutyG/2;
            DutyB = DutyB/2;
        }
        //  else
        //  {
        //      //カード検出時にDutyを0% or 100%にする
        //      //FeliCa読出しを妨害するノイズ源を停止
        //      DutyR = (DutyR > 0 ? 100 : 0);
        //      DutyG = (DutyG > 0 ? 100 : 0);
        //      DutyB = (DutyB > 0 ? 100 : 0);
        //  }
    }
#ifndef WINDOWS_DEBUG
	///////////////////////
	//出力
	///////////////////////
	//赤出力
	if(NULL == pPWM_R)
	{
		if(DutyR > 0)
		{
			err = G_pDrvIOPort->Write(PortR, BSP_IO_LEVEL_HIGH);
		}
		else
		{
			err = G_pDrvIOPort->Write(PortR, BSP_IO_LEVEL_LOW);
		}
	}
	else
	{
		err = pPWM_R->SetDuty(DutyR);
	}
	if(E_OK != err)
	{
		return err;
	}
	
	//緑出力
	err = pPWM_G->SetDuty(DutyG);
	if(E_OK != err)
	{
		return err;
	}
	
	//青出力
	if(NULL == pPWM_B)
	{
		if(DutyB > 0)
		{
			err = G_pDrvIOPort->Write(PortB, BSP_IO_LEVEL_HIGH);
		}
		else
		{
			err = G_pDrvIOPort->Write(PortB, BSP_IO_LEVEL_LOW);
		}
	}
	else
	{
		err = pPWM_B->SetDuty(DutyB);
	}
#else
	//Windows上の動作
	//std::cout << "LED[" << (uint32_t)LedIndex << "] [DutyR]:" << (uint32_t)DutyR << " [DutyG]:" << (uint32_t)DutyG << " [DutyB]:" << (uint32_t)DutyB << " [IsDetectedCard]:" << IsDetectedCard << "\r\n";

	ENUM_SIMPANEL_INFO_NO infoNo = SIMPANEL_UNKNOWN;
	switch (LedIndex)
	{
	case LED_IDX_LEFT:
		infoNo = SIMPANEL_LED1;
		break;
	case LED_IDX_CENTER:
		infoNo = SIMPANEL_LED2;
		break;
	case LED_IDX_RIGHT:
		infoNo = SIMPANEL_LED3;
		break;
	case LED_IDX_FRAME:
		infoNo = SIMPANEL_LED4;
		break;
	default:
		return E_PAR;
		break;
	}

	// パネルシミュレータに通知
	_pSimPanel->Write(infoNo, DutyR, DutyG, DutyB);

#endif /* WINDOWS_DEBUG */

	return err;
}

/*!------------------------------------------------
@brief コマンド文字からLEDパターンインデックス取得
@note　'Z'は何もしないのでエラーとして返す
      'Y'はパターン未設定
@param Id　コマンド文字('0'-'9','A'-'X','a'-'b')
@return LEDパターンインデックス(エラーはマイナス値を返す)
@author
-------------------------------------------------*/
int8_t OutputController::GetLedPatternIndex(char Id)
{
    int8_t index = -1;
    if(Id >= '0' && Id <= '9')
    {
        index = (int8_t)(Id - '0'); //0-9を返す
    }
    else if(Id >= 'A' && Id <= 'X')
    {
    	index = (int8_t)(Id - 'A') + 0x0A; //10-33を返す
    }
    else if(Id >= 'a' && Id <= 'b')
    {
        index = (int8_t)(Id - 'a') + 0x22; //34-35を返す
    }
    return index;
}

/*!------------------------------------------------
@brief コマンド文字からブザーパターンインデックス取得
@note　'Z'は何もしないのでエラーとして返す
@param Id　コマンド文字('0'-'9','A')
@return ブザーパターンインデックス(エラーはマイナス値を返す)
@author
-------------------------------------------------*/
int8_t OutputController::GetBuzzerPatternIndex(char Id)
{
	int8_t index = -1;
	if (Id >= '0' && Id <= '9')
	{
		index = (int8_t)(Id - '0'); //0-9を返す
	}
	else if(Id == 'A')
    {
        index = (int8_t)(Id - 'A') + 0x0A; //10を返す
    }
	return index;
}

/*!------------------------------------------------
@brief コマンド文字から自動ドアパターンインデックス取得
@note　'Z'は何もしないのでエラーとして返す
@param Id　コマンド文字('0'-'9')
@return 自動ドアパターンインデックス(エラーはマイナス値を返す)
@author
-------------------------------------------------*/
int8_t OutputController::GetAutoDoorPatternIndex(char Id)
{
	int8_t index = -1;
	if (Id >= '0' && Id <= '9')
	{
		index = (int8_t)(Id - '0'); //0-9を返す
	}
	return index;
}
/*!------------------------------------------------
@brief ブザー制御
@note　
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR OutputController::BuzzerControl(void)
{
	TYPE_ERROR err = E_OK;

	if (IO_END == _buzzerControl.WorkData.Ctrl)
	{
		//終端(何もしない)
	}
	else if (IO_KEP == _buzzerControl.WorkData.Ctrl)
	{
		//現状維持(何もしない)
	}
	else
	{
		if (_buzzerControl.WorkData.Count > 0)
		{
			//初回だけIO制御
			if (_buzzerControl.WorkData.Count == _buzzerControl.pCurrent[_buzzerControl.PatternIndex].Count)
			{
				//出力指示
				if (IO_OFF == _buzzerControl.WorkData.Ctrl)
				{
					//出力OFF
#ifndef WINDOWS_DEBUG
				    err = G_pDrvPWM_Buzzer->SetDuty(0);
#else
					//Windows上の動作: パネルシミュレータに通知
					_pSimPanel->Write(SIMPANEL_BUZZER, 0);

					//std::cout << "Buzzer OFF  Count:" << (uint32_t)_buzzerControl.WorkData.Count << " PatternIndex:" << (uint32_t)_buzzerControl.PatternIndex << "\r\n";
#endif /*WINDOWS_DEBUG*/
				}
				else
				{
					//出力ON 出力が許可されている場合のみ出力
#ifndef WINDOWS_DEBUG
				    if( true == _buzzerControl.Enable )
				    {
				        // 毎回ボリュームのAD値を読んで、その割合をブザーのデューティ比としてセットし音量を変える
				        uint16_t duty = 100 - G_pDrvADC_Vol->ReadRate(MIN_VOL_AD_RANGE, MAX_VOL_AD_RANGE);
                        uint8_t duty2 = (uint8_t)((duty*duty)/142);  // ボリュームの変化に対し、累乗した値をデューティ比とし、音量低部分の変化を合わせれるようにする
				        err = G_pDrvPWM_Buzzer->SetDuty(duty2);
                        // ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "Duty: ", duty);
				    }
#else
					//Windows上の動作: パネルシミュレータに通知
					_pSimPanel->Write(SIMPANEL_BUZZER, 1);

					//std::cout << "Buzzer ON  Count:" << (uint32_t)_buzzerControl.WorkData.Count << " PatternIndex:" << (uint32_t)_buzzerControl.PatternIndex << "\r\n";
#endif /*WINDOWS_DEBUG*/
				}
			}
#ifdef WINDOWS_DEBUG
			//std::cout << "Buzzer Count:" << (uint32_t)_buzzerControl.WorkData.Count << "\r\n";
#endif /*WINDOWS_DEBUG*/
			_buzzerControl.WorkData.Count--;
			if (_buzzerControl.WorkData.Count == 0)
			{
				//次へ
				if (_buzzerControl.PatternIndex < (BUZZER_PATTERN_MAX-1))
				{
					_buzzerControl.PatternIndex++;
				}
				_buzzerControl.WorkData = _buzzerControl.pCurrent[_buzzerControl.PatternIndex];
#ifdef WINDOWS_DEBUG
				if (IO_END == _buzzerControl.WorkData.Ctrl)
				{
					//std::cout << "Buzzer END  Count:" << (uint32_t)_buzzerControl.WorkData.Count << " PatternIndex:" << (uint32_t)_buzzerControl.PatternIndex << "\r\n";
				}
#endif /*WINDOWS_DEBUG*/
			}
		}
	}
	return err;
}

/*!------------------------------------------------
@brief 自動ドア制御
@note　
@param なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR OutputController::AutoDoorControl(void)
{
	TYPE_ERROR err = E_OK;

	if (IO_END == _autoDoorControl.WorkData.Ctrl)
	{
		//終端(何もしない)
	}
	else if (IO_KEP == _autoDoorControl.WorkData.Ctrl)
	{
		//現状維持(何もしない)
	}
	else
	{
		if (_autoDoorControl.WorkData.Count > 0)
		{
			//初回だけIO制御
			if (_autoDoorControl.WorkData.Count == _autoDoorControl.pCurrent[_autoDoorControl.PatternIndex].Count)
			{
				//出力指示
				if (IO_OFF == _autoDoorControl.WorkData.Ctrl)
				{
					//出力OFF
#ifndef WINDOWS_DEBUG
					err = G_pDrvIOPort->Write(AutoDoor, BSP_IO_LEVEL_LOW);
#ifdef OUTPUT_ERROR_DEBUG_RELAY1
					err = E_SYS;
#endif /* OUTPUT_ERROR_DEBUG_RELAY1 */
					if(E_OK != err)
					{
						//データ管理へ書込み失敗通知
					    G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_RELAY_OFFSET, ERRORFLAG_PORT_RELAY1);
					}

#else
					//Windows上の動作: パネルシミュレータに通知
					_pSimPanel->Write(SIMPANEL_AUTO_DOOR, 0);
					//std::cout << "AutoDoor OFF  Count:" << (uint32_t)_autoDoorControl.WorkData.Count << " PatternIndex:" << (uint32_t)_autoDoorControl.PatternIndex << "\r\n";
#endif /*WINDOWS_DEBUG*/
				}
				else
				{
					//出力ON
#ifndef WINDOWS_DEBUG
					err = G_pDrvIOPort->Write(AutoDoor, BSP_IO_LEVEL_HIGH);
#ifdef OUTPUT_ERROR_DEBUG_RELAY1
					err = E_SYS;
#endif /* OUTPUT_ERROR_DEBUG_RELAY1 */
					if(E_OK != err)
					{
						//データ管理へ書込み失敗通知
					    G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_RELAY_OFFSET, ERRORFLAG_PORT_RELAY1);
					}

#else
					//Windows上の動作: パネルシミュレータに通知
					_pSimPanel->Write(SIMPANEL_AUTO_DOOR, 1);
					//std::cout << "AutoDoor ON  Count:" << (uint32_t)_autoDoorControl.WorkData.Count << " PatternIndex:" << (uint32_t)_autoDoorControl.PatternIndex << "\r\n";
#endif /*WINDOWS_DEBUG*/
				}
			}
#ifdef WINDOWS_DEBUG
			//std::cout << "AutoDoor Count:" << (uint32_t)_autoDoorControl.WorkData.Count << "\r\n";
#endif /*WINDOWS_DEBUG*/
			_autoDoorControl.WorkData.Count--;
			if (_autoDoorControl.WorkData.Count == 0)
			{
				//次へ
				if (_autoDoorControl.PatternIndex < (AUTODOOR_PATTERN_MAX-1))
				{
					_autoDoorControl.PatternIndex++;
				}
				_autoDoorControl.WorkData = _autoDoorControl.pCurrent[_autoDoorControl.PatternIndex];
#ifdef WINDOWS_DEBUG
				if (IO_END == _autoDoorControl.WorkData.Ctrl)
				{
					//std::cout << "AutoDoor END  Count:" << (uint32_t)_autoDoorControl.WorkData.Count << " PatternIndex:" << (uint32_t)_autoDoorControl.PatternIndex << "\r\n";
				}
#endif /*WINDOWS_DEBUG*/
			}
		}
	}
	return err;
}

/*!------------------------------------------------
@brief タスクの情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void OutputController::PrintInfo(void (*funcPrint)(const char*))
{
    (void)funcPrint;
}
