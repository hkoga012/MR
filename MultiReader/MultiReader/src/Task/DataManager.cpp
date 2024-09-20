/*!------------------------------------------------
@brief データ管理タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

#ifndef WINDOWS_DEBUG

/*!------------------------------------------------
@brief データFlash 管理ドライバ User defined callback function
@note
@param p_args
@return なし
@author
-------------------------------------------------*/
static volatile uint8_t g_write_flag = OFF;
extern "C" void vee_callback (rm_vee_callback_args_t * p_args)
{
    if ((NULL != p_args) && (RM_VEE_STATE_READY == p_args->state))
    {
        g_write_flag = ON;
    }
}
constexpr uint32_t RM_VEE_REC_ID = 1; //!< データフラッシュ管理ドライバで読み書きするレコードのID（１種類なので固定）

#endif

/*!------------------------------------------------
@brief データ管理タスク コンストラクタ
@note
@return なし
@author
-------------------------------------------------*/
DataManager::DataManager()
	:BaseTask(ST_DATA_MAX, EVT_DATA_MAX, TIMER_DATA_MAX, DATA_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	// 初期化
	_DipSw1Info = 0;            // DIPSW1(機能選択）設定情報
	_DipSw2Info = 0;            // DIPSW2(リーダアドレス）設定情報
    _ChecksumRunningFw = 0;     // 起動中FWのチェックサム
	_IsNfcStarted = 0;          // NFCリーダ起動完了？  ： 初期値は異常。NFCリーダが起動したら正常にする。使用しない場合も途中のロジックで正常にしている。
	_IsQrStarted = 0;           // QRリーダ起動完了？   : 初期値は異常。QRリーダが起動したら正常にする。使用しない場合も途中のロジックで正常にしている。
	_IsDoorNormality = 1;       // 自動扉接点出力正常？	: 初期値は正常。エラーとなったら異常にする。
	_IsEepromNormality = 0;     // EEPROM正常？：初期値は異常。正常にROMを読めたら正常にする。

	//タイマー登録
	this->RegisterTimer(TIMER_ONESHOT, EVT_DATA_TMOUT_WAIT_READER_STARTED);
	this->RegisterTimer(TIMER_ONESHOT, EVT_DATA_TMOUT_WAIT_BOOT);
	this->RegisterTimer(TIMER_ONESHOT, EVT_DATA_TM_SHORTEST_TIME_TO_START);
	this->RegisterTimer(TIMER_FOREVER, EVT_DATA_TM_INTERVAL_ROM_UPDATE);

	//停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_DATA_DORMANT);
	st->AddEventHandler(EVT_DATA_START, Dormant_Start);		//!< 開始

	//リーダ起動待ち（NFC、QR 両方待ち）状態へ各ハンドラ登録
    st = this->GetState(ST_DATA_WAIT_READER_STARTED);
	st->AddEnterHandler(WaitReaderStarted_Enter);	//!< Enter
	st->AddExitHandler(WaitReaderStarted_Exit);	//!< Exit
	st->AddEventHandler(EVT_DATA_QR_STARTED, WaitReaderStarted_NfcOrQrReaderStarted);	//!< QRリーダ起動完了
	st->AddEventHandler(EVT_DATA_NFC_STARTED, WaitReaderStarted_NfcOrQrReaderStarted);	//!< NFCリーダ起動完了
	st->AddEventHandler(EVT_DATA_TMOUT_WAIT_READER_STARTED, WaitReaderStarted_TmoutWaitReaderStarted);	//!< リーダ起動待ちタイムアウト

	//リセット送信リーダ送信待ち状態へ各ハンドラ登録
	st = this->GetState(ST_DATA_WAIT_RESETDATA_SENDED);
	st->AddEnterHandler(WaitResetdataSended_Enter);	//!< Enter
	st->AddEventHandler(EVT_DATA_TM_SHORTEST_TIME_TO_START, WaitResetdataSended_TmShortestTimeToStart);	//!< 最短起動時間経過通知
	st->AddEventHandler(EVT_DATA_RESETDATA_SENDED, WaitResetdataSended_ResetDataSended);	//!< リセットデータ送信完了
	st->AddEventHandler(EVT_DATA_TMOUT_WAIT_BOOT, WaitResetdataSended_TmoutWaitBoot);		//!< 装置起動待ちタイムアウト

	//動作中状態へ各ハンドラ登録
	st = this->GetState(ST_DATA_RUNNING);
	st->AddEnterHandler(Running_Enter);	//!< Enter
	st->AddEnterHandler(Running_Exit);	//!< Exit
	st->AddEventHandler(EVT_DATA_LED, Running_Led);	//!< LED更新指示
	st->AddEventHandler(EVT_DATA_SET_INITIAL, Running_SetInitial);	//!< イニシャル設定
	st->AddEventHandler(EVT_DATA_CLS_CUMULATIVE, Running_ClsCumulative);	//!< 累積回数クリア
	st->AddEventHandler(EVT_DATA_TM_INTERVAL_ROM_UPDATE, Running_TmIntervalRomUpdate);	//!< ROM更新インターバル通知

	//停止中（FW更新タスク動作中）状態 へのハンドル登録はない
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
void DataManager::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
    DataManager* pMyTask = (DataManager*)pOwnTask;

    // DIPSWから設定値ロード、FWチェックサム値を計算する
	pMyTask->LoadDipSw();
	pMyTask->CalculateCheckSum();
	pMyTask->PrintInfo_Board(DebugCommand::SendPrintSci);

	// ROMから保持値をロードする
	TYPE_ERROR err = pMyTask->_DataStoredInDataFlash.Load();
	if(err == E_OK)
    {   // 前回の情報が保持されていたら、デバッグコマンド表示に出力する
#ifndef EEPROM_ERROR_DEBUG_EEPROM_NORMALITY
	    pMyTask->_IsEepromNormality = 1;    // EEPROM有効にセット
#endif
        DummyDelay(0xffff);
	    pMyTask->PrintInfo_DataStoredInDataFlash(DebugCommand::SendPrintSci);
	}
	else
	{
	    DebugCommand::SendPrintSci("## Data is not retained in Data Flash\r\n");
	}

	// ROMから過去のシステムエラー情報をロードする。過去のシステムエラー情報が無くても構わない。
	err = pMyTask->_SystemErrorStoredInDataFlash.Load();
    if(err == E_OK)
    {   // システムエラー情報が保持されていたら、デバッグコマンド表示に出力する
        DummyDelay(0xffff);
        pMyTask->PrintInfo_SystemErrorStoredInDataFlash(DebugCommand::SendPrintSci);
    }

	// リーダ起動待ちに遷移
	pMyTask->SetNextStateId(ST_DATA_WAIT_READER_STARTED);
}


/*!------------------------------------------------
@brief リーダ起動待ち状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitReaderStarted_Enter(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;
//    DebugCommand::SendPrintSci("DataManager::WaitReaderStarted_Enter\r\n");

    // リーダ起動待ちタイムアウト監視タイマー動作開始
    pMyTask->StartTimer(EVT_DATA_TMOUT_WAIT_READER_STARTED, MS_TMOUT_WAIT_READER_STARTED);

	// 装置起動待ちタイムアウト監視タイマー動作開始（これは保険として利用）
	pMyTask->StartTimer(EVT_DATA_TMOUT_WAIT_BOOT, MS_TMOUT_WAIT_BOOT);

    // 最短起動時間経過通知タイマー動作開始
    pMyTask->StartTimer(EVT_DATA_TM_SHORTEST_TIME_TO_START, MS_TM_SHORTEST_TIME_TO_START);

	// NFCリーダ開始通知（ Ferica 又は Mifareが有効の場合 ）
	if ((pMyTask->GetSelFuncFelica() != SEL_FUNC_FELICA_NO_READ)
		|| (pMyTask->GetSelFuncMifare() != SEL_FUNC_MIFARE_NO_READ))
	{
#ifndef DEBUG_NO_NFC
		G_pNfcReader->SendEvent(EVT_NFC_START);
#endif
	}
	else
	{
		pMyTask->_IsNfcStarted = 1;	// 強制的にNFCリーダを起動完了にする
	}

	// QRリーダ開始通知（ QRリーダ有効の場合 ）
	if (pMyTask->IsSelFuncQrReader())
	{
		G_pQrReader->SendEvent(EVT_QR_START);
	}
	else
	{
		pMyTask->_IsQrStarted = 1;	// 強制的にQRリーダを起動完了にする
	}

    if (pMyTask->_IsNfcStarted && pMyTask->_IsQrStarted)
    {   // 両方のリーダが共に無効な為、起動したことにていたら、リセットデータ送信待ちに遷移
        pMyTask->SetNextStateId(ST_DATA_WAIT_RESETDATA_SENDED);
    }
}


/*!------------------------------------------------
@brief リーダ起動待ち状態 － Exit処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitReaderStarted_Exit(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;
//    DebugCommand::SendPrintSci("DataManager::WaitReaderStarted_Exit\r\n");

	// リーダ起動待ちタイムアウト監視タイマー動作終了
	pMyTask->StopTimer(EVT_DATA_TMOUT_WAIT_READER_STARTED);
}


/*!------------------------------------------------
@brief リーダ起動待ち状態 － リーダ起動完了 
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitReaderStarted_NfcOrQrReaderStarted(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

//    DebugCommand::SendPrintSciEx("_XxxStarted　EventId=%d\r\n", EventId);

	switch (EventId) {
		case EVT_DATA_NFC_STARTED:
			pMyTask->_IsNfcStarted = 1;	// NFCリーダ開始
			break;

		case EVT_DATA_QR_STARTED:
			pMyTask->_IsQrStarted = 1;	// QRリーダ開始
			break;
	}

	if (pMyTask->_IsNfcStarted && pMyTask->_IsQrStarted)
	{	// 両方のリーダが共に起動していたら、リセットデータ送信待ちに遷移
		pMyTask->SetNextStateId(ST_DATA_WAIT_RESETDATA_SENDED);
	}
}

/*!------------------------------------------------
@brief リーダ起動待ち状態 － リーダ起動待ちタイムアウト
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitReaderStarted_TmoutWaitReaderStarted(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	(void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

	//	DebugCommand::SendPrintSci("DataManager::TmoutWaitReaderStarted\r\n");

	// リーダ起動待ちタイムアウトなら、リセットデータ送信待ちに遷移
	pMyTask->SetNextStateId(ST_DATA_WAIT_RESETDATA_SENDED);
}


/*!------------------------------------------------
@brief リセットデータ送信待ち状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitResetdataSended_Enter(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;
//    DebugCommand::SendPrintSci("DataManager::WaitResetdataSended_Enter\r\n");

	pMyTask->__WaitResetdataSended_IfDoneShortestTimeToStartCommandReceiver();
}


/*!------------------------------------------------
@brief リセットデータ送信待ち状態 － Exit処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitResetdataSended_Exit(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;
//    DebugCommand::SendPrintSci("DataManager::WaitResetdataSended_Exit\r\n");

	// 最短起動時間経過通知タイマー動作停止
	pMyTask->StopTimer(EVT_DATA_TM_SHORTEST_TIME_TO_START);
}


/*!------------------------------------------------
@brief リセットデータ送信待ち状態 － リーダ起動待ちタイムアウト
@note　
@param pOwnTask : 自タスクオブジェクトのポインタ
@param EventId : 使用しない
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitResetdataSended_TmShortestTimeToStart(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	(void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;
	pMyTask->__WaitResetdataSended_IfDoneShortestTimeToStartCommandReceiver();
}


/*!------------------------------------------------
@brief 最短起動時間を超えていたら、コマンド受信タスクを開始
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::__WaitResetdataSended_IfDoneShortestTimeToStartCommandReceiver()
{
//    DebugCommand::SendPrintSci("..._IfDoneShortestTimeToStartCommandReceiver()\r\n");

	if (IsStoppedTimer(EVT_DATA_TM_SHORTEST_TIME_TO_START))
	{
	    if (_IsNfcStarted)
		{
			// コマンド送信タスク 動作開始指示
//            DebugCommand::SendPrintSci("SendEvent(EVT_CMDRCV_START)\r\n");
			G_pCommandReceiver->SendEvent(EVT_CMDRCV_START);
		}
		else
		{
			// NFCリーダが初期化に失敗しているので再起動
			SystemSaveAndReboot();    // この関数を呼んでも、まだDataManagerが起動完了状態にないので、データFlashは更新しない
		}
	}
}

/*!------------------------------------------------
@brief リセットデータ送信待ち状態 － リセットデータ送信完了
@note　
@param pOwnTask : 自タスクオブジェクトのポインタ
@param EventId : 使用しない
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitResetdataSended_ResetDataSended(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	(void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

//    DebugCommand::SendPrintSci("WaitResetdataSended_ResetDataSended()\r\n");

	// 動作中に遷移
	pMyTask->SetNextStateId(ST_DATA_RUNNING);
}


/*!------------------------------------------------
@brief リセットデータ送信待ち状態 － 装置起動待ちタイムアウト
@note　あくまで保険として実装。このタイムアウトが発生する前に正常に起動するか、再起動が発生する筈。
@param pOwnTask : 自タスクオブジェクトのポインタ
@param EventId : 使用しない
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::WaitResetdataSended_TmoutWaitBoot(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	(void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

//    DebugCommand::SendPrintSci("WaitResetdataSended_TmoutWaitBoot()\r\n");

	// 再起動
	pMyTask->SystemSaveAndReboot();
}


/*!------------------------------------------------
@brief 動作中状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_Enter(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;

//    DebugCommand::SendPrintSci("DataManager::Running_Enter\n");

	// LED点灯状態、ブザー鳴動 を OutputController に通知する
	pMyTask->NoticeInitLedInfoToOutputController();

	// 装置起動カウンタを＋１し、起動直後の状態をROMに更新
	pMyTask->AddEventCount(EVENT_RESET);
	pMyTask->_DataStoredInDataFlash.Update();

	// ROM更新インターバルタイマー動作開始
	pMyTask->StartTimer(EVT_DATA_TM_INTERVAL_ROM_UPDATE, MS_TMOUT_ROM_UPDATE);
}

/*!------------------------------------------------
@brief 動作中状態 － Exit処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_Exit(BaseTask* pOwnTask)
{
	DataManager* pMyTask = (DataManager*)pOwnTask;

//    DebugCommand::SendPrintSci("DataManager::Running_Exit\n");

	// ROM更新インターバルタイマー動作停止
	pMyTask->StopTimer(EVT_DATA_TM_INTERVAL_ROM_UPDATE);
}


/*!------------------------------------------------
@brief 動作中状態 － LED更新指示
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_Led(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

	// 受信した各LEDの情報を確認し '0'、'T'～'X'なら次回起動時の情報としてセットする
	for (uint8_t index = 0; index < 4; index++) 
	{
		uint8_t stLed = pData[index];
		if ((stLed == '0') || ('T' <= stLed && stLed <= 'X'))
		{
			pMyTask->_DataStoredInDataFlash.info.ArrayLedStatusForInit[index] = stLed;
		}
	}
}

/*!------------------------------------------------
@brief 動作中状態 － イニシャル設定
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_SetInitial(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)DataLen;  // 未使用引数のワーニング対策
    DataManager* pMyTask = (DataManager*)pOwnTask;
	InitialInfo* pInf = &(pMyTask->_InitialInfo);

	if(pMyTask->GetSelFuncFelica() == SEL_FUNC_FELICA_INITIAL_COM)
	{   // DIPSW1がイニシャル設定を受け付ける設定になっている場合のみ、受信したイニシャル設定を DataManagerのバッファに格納する

	    // 現在のイニシャル設定を一旦クリア
	    pInf->Clear();

	    // Mifare1の設定を取り込み
	    pInf->Mifare1_KeyType = *(pData + POS_MIFARE1_KEY_TYPE);
	    ClsCommon::AsciiHex2Bin((char*)pData + POS_MIFARE1_KEY_DATA, sizeof(pInf->Mifare1_KeyData) * 2, pInf->Mifare1_KeyData, sizeof(pInf->Mifare1_KeyData));
	    ClsCommon::AsciiHex2Bin((char*)pData + POS_MIFARE1_BLOCK, 2, &(pInf->Myfare1_Block), 1);
	    if (pInf->IsValidData(pData + POS_MIFARE1_KEY_DATA, sizeof(pInf->Mifare1_KeyData) * 2)
	        && pInf->IsValidData(pData + POS_MIFARE1_BLOCK, 2)
	        && ((pInf->Mifare1_KeyType == INITIAL_MIFARE_KEYA) || (pInf->Mifare1_KeyType == INITIAL_MIFARE_KEYB)) )
	    {
	        // Mifare1が有効なら、Mifare2を取り込み
	        pInf->Mifare2_KeyType = *(pData + POS_MIFARE2_KEY_TYPE);
	        ClsCommon::AsciiHex2Bin((char*)pData + POS_MIFARE2_KEY_DATA, sizeof(pInf->Mifare2_KeyData) * 2, pInf->Mifare2_KeyData, sizeof(pInf->Mifare2_KeyData));
	        ClsCommon::AsciiHex2Bin((char*)pData + POS_MIFARE2_BLOCK, 2, &(pInf->Myfare2_Block), 1);
	        if (pInf->IsValidData(pData + POS_MIFARE2_KEY_DATA, sizeof(pInf->Mifare2_KeyData) * 2)
	            && pInf->IsValidData(pData + POS_MIFARE2_BLOCK, 2)
	            && ((pInf->Mifare2_KeyType == INITIAL_MIFARE_KEYA) || (pInf->Mifare2_KeyType == INITIAL_MIFARE_KEYB)))
	        {
	            // 全て有効なのでデータ修正無し
	        }
	        else
	        {
	            pInf->Mifare2_KeyType = INITIAL_FERICA_INVALID; // 無効
	        }
	    }
	    else
	    {
	        pInf->Mifare1_KeyType = INITIAL_FERICA_INVALID; // 無効
	    }

	    ClsCommon::AsciiHex2Bin((char*)pData + POS_FERICA_SYSTEM_CD, sizeof(pInf->Ferica_SystemCd) * 2, pInf->Ferica_SystemCd, sizeof(pInf->Ferica_SystemCd));
	    ClsCommon::AsciiHex2Bin((char*)pData + POS_FERICA_SERVICE_CD, sizeof(pInf->Ferica_ServiceCd) * 2, pInf->Ferica_ServiceCd, sizeof(pInf->Ferica_ServiceCd));
	    ClsCommon::AsciiHex2Bin((char*)pData + POS_FERICA_BLOCK1, 2, &(pInf->Ferica_Block1), 1);
	    ClsCommon::AsciiHex2Bin((char*)pData + POS_FERICA_BLOCK2, 2, &(pInf->Ferica_Block2), 1);
	    pInf->Ferica_ValidBlock1 = INITIAL_FERICA_INVALID;
	    pInf->Ferica_ValidBlock2 = INITIAL_FERICA_INVALID;
	    if (pInf->IsValidData(pData + POS_FERICA_SYSTEM_CD, sizeof(pInf->Ferica_SystemCd) * 2)
	        && pInf->IsValidData(pData + POS_FERICA_SERVICE_CD, sizeof(pInf->Ferica_ServiceCd) * 2)
	        && pInf->IsValidData(pData + POS_FERICA_BLOCK1, 2))
	    {
	        pInf->Ferica_ValidBlock1 = INITIAL_FERICA_VALID;

	        if (pInf->IsValidData(pData + POS_FERICA_BLOCK2, 2))
	        {
	            pInf->Ferica_ValidBlock2 = INITIAL_FERICA_VALID;
	        }
	    }

	    // イニシャル設定が変更になった旨、NFCリーダに通知
#ifndef DEBUG_NO_NFC
	    G_pNfcReader->SendEvent(EVT_NFC_INITIAL_UPDATED);
#endif

	}

}

/*!------------------------------------------------
@brief 動作中状態 － 累積回数クリア
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_ClsCumulative(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

	// クリア
	pMyTask->_DataStoredInDataFlash.Clear();

	// ROMデータ更新
	pMyTask->_DataStoredInDataFlash.Update();
}

#ifdef DFLASH_DRV_DEBUG_FORCED_STOP
    // 書き込み中の電源断評価開始（ドライバの書込処理途中で強制リセットさせる）　実体は、ra/fsp/src/r_flash_lp/r_flash_lp.c に置いた
    extern "C" void DebugStart_ForDflashDrvDebugForcedStop();
#endif

/*!------------------------------------------------
@brief 動作中状態 － ROM更新インターバル通知
@note
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DataManager::Running_TmIntervalRomUpdate(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DataManager* pMyTask = (DataManager*)pOwnTask;

#ifdef DFLASH_DRV_DEBUG_FORCED_STOP
	// 書き込み中の電源断評価開始（ドライバの書込処理途中で強制リセットさせる）
	DebugStart_ForDflashDrvDebugForcedStop();
    DebugCommand::SendPrintSci("DFLASH_DRV_DEBUG_FORCED_STOP\r\n");
#endif

	// ROMデータ更新
	pMyTask->_DataStoredInDataFlash.Update();
}


/*!------------------------------------------------
@brief 機能選択 Felica情報を取得
@note 
@return 選択された機能
@author
-------------------------------------------------*/
ENUM_SEL_FUNC_FELICA DataManager::GetSelFuncFelica() 
{
    ENUM_SEL_FUNC_FELICA ret;
    switch ((_DipSw1Info & 0x3c) >> 2) 
    {
        case 0xf: ret = SEL_FUNC_FELICA_FTSIC; break;       // 1111
        case 0x1: ret = SEL_FUNC_FELICA_SURPASS; break;     // 0001
        case 0x3: ret = SEL_FUNC_IDM; break;                // 0011
        case 0x9: ret = SEL_FUNC_FELICA_SURPASS_IDM; break; // 1001
        case 0x5: ret = SEL_FUNC_FELICA_FTSIC_IDM; break;   // 0101
        case 0x0: ret = SEL_FUNC_FELICA_INITIAL_COM; break; // 0000
        default:  ret = SEL_FUNC_FELICA_NO_READ; break;     // 設定無効も含む
    }
    return ret;
}

/*!------------------------------------------------
@brief 機能選択 Mifare情報を取得
@note 
@return 選択された機能
@author
-------------------------------------------------*/
ENUM_SEL_FUNC_MIFARE DataManager::GetSelFuncMifare()
{
    ENUM_SEL_FUNC_MIFARE ret;

    switch (GetSelFuncFelica()) {
        case SEL_FUNC_FELICA_INITIAL_COM:
            ret = SEL_FUNC_MIFARE_INITIAL_COM;
            break;
        case SEL_FUNC_FELICA_NO_READ:
            ret = (((_DipSw1Info & 0x3e) == 0x32) ? SEL_FUNC_MIFARE_READ : SEL_FUNC_MIFARE_NO_READ);
            break;
        default:
            ret = ((_DipSw1Info & 0x02) ? SEL_FUNC_MIFARE_READ : SEL_FUNC_MIFARE_NO_READ);
            break;
    }

    return ret;
}


/*!------------------------------------------------
@brief 機能選択 ブザー有無
@note
@return 1:ブザー有り 0:ブザー無し
@author
-------------------------------------------------*/
int8_t DataManager::IsSelFuncBuzzer()
{
	return (_DipSw1Info & 0x40) ? 0 : 1;
}


/*!------------------------------------------------
@brief 機能選択 QRリーダー 有無
@note
@return 1:QRリーダー有り 0:QRリーダー無し
@author
-------------------------------------------------*/
int8_t DataManager::IsSelFuncQrReader()
{
    return (_DipSw1Info & 0x01) ? 1 : 0;
}


/*!------------------------------------------------
@brief リーダーアドレスを取得
@note 起動時のSW2の情報を 1～16 で返す
@return リーダーアドレス 1～16 (01H～10H)
@author
-------------------------------------------------*/
uint8_t DataManager::GetReaderAddress()
{
    return (_DipSw2Info & 0x0f) == 0
        ? 0x10
        : (_DipSw2Info & 0x0f);
}

/*!------------------------------------------------
@brief リーダーハードウエア状態を取得
@note 
@return リーダ状態 0x30～0x3F
@author
-------------------------------------------------*/
uint8_t DataManager::GetReaderHwStatus()
{
    return (uint8_t) (0x30
        | (_IsQrStarted ? 0x00 : 0x01)          // QRリーダ起動完了/異常
        | (_IsNfcStarted ? 0x00 : 0x02)         // NFCリーダ起動完了/異常
        | (_IsDoorNormality ? 0x00 : 0x04)      // 自動扉接点出力正常/異常
        | (_IsEepromNormality ? 0x00 : 0x08));  // EEPROM正常/異常
}

/*!------------------------------------------------
@brief イベントフラグをセットする
@param pos : イベントフラグ項目位置
@param val : セットする値
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::SetErrorFlag(ERRORFLAG_OFFSET pos, ERRORFLAG val)
{
    _DataStoredInDataFlash.info.MemoryBuf[EEPROM_MAP_OUTPUT_ERROR_OFFSET + pos] |= val;

	if ((pos == ERRORFLAG_PORT_RELAY_OFFSET) && (val == ERRORFLAG_PORT_RELAY1)) 
	{
		_IsDoorNormality = 0;	// 自動扉接点出力異常をセット
	}
}

/*!------------------------------------------------
@brief 累積回数を＋１する
@note  ビックエンディアン
@param pos : 累積回数項目位置
@return なし
@author
-------------------------------------------------*/
void DataManager::AddEventCount(EVENT_COUNT pos)
{
    uint8_t* pVal_H = (uint8_t*)(&_DataStoredInDataFlash.info.MemoryBuf[EEPROM_MAP_COUNTER_OFFSET + (pos * 2)]);
    uint8_t* pVal_L = pVal_H + 1;
    uint16_t val = (uint16_t)(*pVal_H << 8) + *pVal_L;  //値に変換する
    val++;  //累積回数＋１
    *pVal_H = (uint8_t)(val / 256); //256で割ったを値代入
    *pVal_L = (uint8_t)(val % 256); //256で割った余りを代入

    if(pos==EVENT_ERROR_HOST_PRI_FER)
    {	// デバッグ用にフレーミングエラー発生をデバッグコンソール出力
        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "EVENT_ERROR_HOST_PRI_FER Count=", val);
    }
}

/*!------------------------------------------------
@brief 現在のメモリデータ（イベントフラグ・累積回数）を取得する
@note 
@return メモリデータ格納領域の先頭アドレス
@author
-------------------------------------------------*/
const uint8_t* DataManager::GetRomDataBuf()
{
	return _DataStoredInDataFlash.info.MemoryBuf;
}

/*!------------------------------------------------
@brief DIP SW1 の情報を返す
@note リセットデータ用
@return DIP SW1 の値
@author
-------------------------------------------------*/
uint8_t DataManager::GetInfoDipSw1()
{
	return _DipSw1Info;
}

/*!------------------------------------------------
@brief DIP SW2 の情報を返す
@note リセットデータ用
@return DIP SW2 の値
@author
-------------------------------------------------*/
uint8_t DataManager::GetInfoDipSw2()
{
	return _DipSw2Info;
}

/*!------------------------------------------------
@brief LED点灯状態を返す
@note リセットデータ用
@return LED点灯状態配列の先頭ポインタ
@author
-------------------------------------------------*/
const uint8_t* DataManager::GetLedStatusForInit()
{
	return _DataStoredInDataFlash.info.ArrayLedStatusForInit;
}

/*!------------------------------------------------
@brief 現在実行中のチェックサム値を取得する
@note 
@return チェックサム値 2byte
@author
-------------------------------------------------*/
uint16_t DataManager::GetCheckSum()
{
	return _ChecksumRunningFw;
}

/*!------------------------------------------------
@brief イニシャル設定情報を取得する
@note
@return イニシャル設定情報のアドレスを返す
@author
-------------------------------------------------*/
const InitialInfo* DataManager::GetInitialInfo()
{
	return &_InitialInfo;
}

/*!------------------------------------------------
@brief FW更新開始により強制停止
@note
@return 
@author
-------------------------------------------------*/
void DataManager::SetForcedStop()
{
	SetNextStateId(ST_DATA_STOP);
}

/*!------------------------------------------------
@brief システムエラー情報をROMに書き込みリブート
@note 
@param pFileName 発生ファイル名
@param line 発生行
@param pMessage 発生エラー説明
@return なし
@author
-------------------------------------------------*/
void DataManager::SetSystemErrorInfoAndReboot(const char *pFileName, uint32_t line, const char *pTaskName, const char *pMessage)
{
#ifdef WINDOWS_DEBUG
	std::cout << "#### DataManager::SetSystemErrorInfoAndReboot() #### \r\n";
	std::cout << "  File: " << _SystemErrorStoredInDataFlash.info[0].FileNameWhenSysErrOccurs << "\r\n";
	std::cout << "  Line: " << _SystemErrorStoredInDataFlash.info[0].LineWhenSysErrOccurs << "\r\n";
	std::cout << "  TaskName: " << _SystemErrorStoredInDataFlash.info[0].TaskNameWhenSysErrOccurs << "\r\n";
	std::cout << "  Message: " << _SystemErrorStoredInDataFlash.info[0].MessageWhenSysErrOccurs << "\r\n";
#endif
	
	// システムエラー情報をセットして、データFlashに書き込む
	_SystemErrorStoredInDataFlash.SetInfoSysErrOccurs(pFileName, line, pTaskName, pMessage, GetTimerTick());
    _SystemErrorStoredInDataFlash.Update();

	// データFlashに書き込みリブート（起動完了の場合のみデータFlashに書く）
	SystemSaveAndReboot();
}


/*!------------------------------------------------
@brief システム情報をROMに書き込みリブート（書くかどうかは、DataManagerの状態に依存する）
@note 
@return なし
@author
-------------------------------------------------*/
void DataManager::SystemSaveAndReboot()
{

#ifdef WINDOWS_DEBUG
	std::cout << "#### DataManager::SystemSaveAndReboot() #### \r\n";
#else
	// データFlashの更新
	// ・ 更新前後でリフレッシュする。
	// ・ 起動完了（リセットデータ送信完了）前にリブートする場合は書かない
	uint32_t st = GetState()->GetId();
	if ((st == ST_DATA_RUNNING) || (st == ST_DATA_STOP))
	{
		G_pDrvWDT->Refresh();
		_DataStoredInDataFlash.Update();
	}
	G_pDrvWDT->Refresh();
#endif

	// リブート
	while (1) {
		// ウォッチドッグをリフレッシュしないのでリブートされる
		DummyDelay(0xffff);
		DebugCommand::SendPrintSci(".");
	}
}


/*!------------------------------------------------
@brief データFlashの強制フォーマット（初期１レコード付き）
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::DataFlashFormat()
{
    // データFlash消去
    BaseInDataFlash::Format();
#ifndef WINDOWS_DEBUG
	G_pDrvWDT->Refresh();
#endif

    // データフラッシュに初期レコード書き込み
    _DataStoredInDataFlash.Clear();
    _DataStoredInDataFlash.Update();

    // 装置をリブート
    while (1) {
        // ウォッチドッグをリフレッシュしないのでリブートされる
        DummyDelay(0xffff);
        DebugCommand::SendPrintSci(".");
    }
}


/*!------------------------------------------------
@brief データFlashの消去（全てFF）－ あくまで評価用（基板部品実装直後の状態）
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::DataFlashErase()
{
#ifndef WINDOWS_DEBUG

    //割り込み禁止
    DEFINE_INT();
    DISABLE_INT();

    const char *pMsg = "Erase Done\r\n";

    fsp_err_t err_t = R_FLASH_LP_Open(g_flash1.p_ctrl, g_flash1.p_cfg);
    if(err_t == FSP_SUCCESS)
    {
        // データフラッシュのP/Eアドレスは0xFE000000だが、ライブラリにて変換している為、リードアドレスを指定、全8ブロックを消去
        err_t = R_FLASH_LP_Erase(g_flash1.p_ctrl, 0x40100000, 8);
        if(err_t != FSP_SUCCESS)
        {
            pMsg = "Erase Error\r\n";
        }

        R_FLASH_LP_Close(g_flash1.p_ctrl);
    }
    else
    {
        pMsg = "Open Error\r\n";
    }
    DebugCommand::SendPrintSci(pMsg);

    //割り込み許可
    ENABLE_INT();

#endif

    // 装置をリブート
    while (1) {
        // ウォッチドッグをリフレッシュしないのでリブートされる
        DummyDelay(0xffff);
        DebugCommand::SendPrintSci(".");
    }
}

/*!------------------------------------------------
@brief データFlashを強制的に更新
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::DataFlashUpdateForce()
{
    _DataStoredInDataFlash.Update();
}

/*!------------------------------------------------
@brief DIPSWの設定情報をロードする
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::LoadDipSw()
{
    // DIPSW は ON が BSP_IO_LEVEL_LOW
    // [DIPSW1]
    // pin1(0x80): NFC RF出力 ON:High Power / OFF:Low Power
    // pin2(0x40): ON:ブザー無し　/ OFF:ブザー有り
    // pin3-7(0x3E): カード種別
    // pin8(0x01): ON:QRリーダ読取あり / OFF:読取無し
    // [DIPSW2]
    // pin1-4(0xF0): 未使用
    // pin5-8(0x0F): リーダアドレス 但し、0はアドレス16

    _DipSw1Info = 0x00;
    _DipSw2Info = 0x00;

#ifdef WINDOWS_DEBUG
    // Windows上でのデバッグ用 (実体は task.h に設定)
#ifdef DEBUG_DUMMY_DIPSW
    SET_DUMMY_DIPSW();
#endif
#else
    for(uint32_t swNo=1; swNo<=2; swNo++)
    {
        for(uint32_t pinNo=1; pinNo<=8; pinNo++)
        {
            bsp_io_port_pin_t portPin
            = (swNo==1)
                ? ((pinNo==1) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_00
                    : (pinNo==2) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_01
                    : (pinNo==3) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_02
                    : (pinNo==4) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_03
                    : (pinNo==5) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_04
                    : (pinNo==6) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_05
                    : (pinNo==7) ? bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_06
                    : bsp_io_port_pin_t::BSP_IO_PORT_00_PIN_07 )
                : ((pinNo==1) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_00
                    : (pinNo==2) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_01
                    : (pinNo==3) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_02
                    : (pinNo==4) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_03
                    : (pinNo==5) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_04
                    : (pinNo==6) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_05
                    : (pinNo==7) ? bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_06
                    : bsp_io_port_pin_t::BSP_IO_PORT_04_PIN_07 );

            bsp_io_level_t val = bsp_io_level_t::BSP_IO_LEVEL_HIGH;
            if(E_OK == G_pDrvIOPort->Read(portPin, &val))
            {
                if(val == bsp_io_level_t::BSP_IO_LEVEL_LOW)
                {
                    if(swNo==1){
                        _DipSw1Info |= (uint8_t)(0x80 >> (pinNo-1));
                    }else{
                        _DipSw2Info |= (uint8_t)(0x80 >> (pinNo-1));
                    }
                }
            }
            else
            {   // DIPSWが読めないのは、HWエラー
                THROW_FATAL_ERROR(GetTaskName(), "DIPSW ReadError");
            }
        }
    }

#ifdef DEBUG_DUMMY_DIPSW
    // DIPSW未実装のHW利用時は、ここで強制的に値をセットしてデバッグ (実体は task.h に設定)
    SET_DUMMY_DIPSW();
#endif

#endif
}


/*!------------------------------------------------
@brief 前回終了時のLED表示状態、ブザー音をOutputControllerに通知する
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::NoticeInitLedInfoToOutputController()
{
//	DebugCommand::SendPrintSci("NoticeInitLedInfoToOutputController()\n");

	G_pOutputController->SendEvent(EVT_OUTPUT_LED, GetLedStatusForInit(), SZ_LED_BUF_SIZE);
	static const uint8_t command = 'A';//起動コマンド
	G_pOutputController->SendEvent(EVT_OUTPUT_BUZZER,&command,LEN_BUZZER_IN_INST_CMD);
}

/*!------------------------------------------------
@brief 現在起動中のＦＷチェックサムを計算する
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::CalculateCheckSum()
{
#ifndef WINDOWS_DEBUG
        // 現在起動中のＦＷチェックサムを計算（コード領域 120Kをバイト単位で加算）
        _ChecksumRunningFw = 0;
        for (uint8_t* p = (uint8_t*)_adrs_Region1_Begin; p <= (uint8_t*)_adrs_Region1_End; p++)
        {
            _ChecksumRunningFw += (*p);
        }
#endif
}


/*!------------------------------------------------
@brief [StoredDataInfoクラス] コンストラクタ
@note
@author
-------------------------------------------------*/
StoredDataInfo::StoredDataInfo()
{
    Clear();
    memset(ArrayLedStatusForInit, '0', sizeof(ArrayLedStatusForInit));  // LED表示状態配列 '0':消灯
}

/*!------------------------------------------------
@brief [StoredDataInfoクラス] データ初期化
@note
@author
-------------------------------------------------*/
void StoredDataInfo::Clear()
{
	memset(MemoryBuf, 0, sizeof(MemoryBuf));
	SetHeaderData();
}

void StoredDataInfo::SetHeaderData()
{
    // ヘッダ部 8Byte 機器種別、残りはゼロで初期化
    memset(MemoryBuf, 0x20, 16);    // 先頭16バイトはスペースで初期化
    const char* deviceTypeName = DEVICE_TYPE_NAME;
    memcpy(MemoryBuf, deviceTypeName, GetMin(8, strlen(deviceTypeName)));

    //          +8Byte 製品シリアルNo
    const char* pSerNo = DataManager::GetSerialNo();
    memcpy(MemoryBuf + 8, pSerNo, GetMin(8, strlen(pSerNo)));

}

/*!------------------------------------------------
@brief [DataStoredInDataFlashクラス] コンストラクタ
@note
@author
-------------------------------------------------*/
DataStoredInDataFlash::DataStoredInDataFlash()
	:BaseInDataFlash()
{
}

/*!------------------------------------------------
@brief [SystemErrorInfoクラス] コンストラクタ
@note
@author
-------------------------------------------------*/
SystemErrorInfo::SystemErrorInfo()
{
    Clear();
}

/*!------------------------------------------------
@brief [SystemErrorInfoクラス] 初期化
@note
@author
-------------------------------------------------*/
void SystemErrorInfo::Clear()
{
    CounterSysErrorOcuurs = 0;
    TimeTickWhenSysErrorOcuurs = 0;
    memset(FileNameWhenSysErrOccurs, 0, sizeof(FileNameWhenSysErrOccurs));
    LineWhenSysErrOccurs = 0;
    memset(TaskNameWhenSysErrOccurs, 0, sizeof(TaskNameWhenSysErrOccurs));
    memset(MessageWhenSysErrOccurs, 0, sizeof(MessageWhenSysErrOccurs));
}


/*!------------------------------------------------
@brief [SystemErrorStoredInDataFlashクラス] コンストラクタ
@note
@author
-------------------------------------------------*/
SystemErrorStoredInDataFlash::SystemErrorStoredInDataFlash()
    :BaseInDataFlash()
{
}


/*!------------------------------------------------
@brief [DataStoredInDataFlashクラス] システムエラー発生箇所の情報セット
@param pFileName 発生ファイル名
@param line 発生行
@param pTaskName タスク名
@param pMessage 発生エラー説明
@return 0:
@note
@author
-------------------------------------------------*/
TYPE_ERROR SystemErrorStoredInDataFlash::SetInfoSysErrOccurs(const char* pFileName, uint32_t line, const char* pTaskName, const char* pMessage, uint32_t timeTick)
{
	TYPE_ERROR err = E_ELSE;

	// デバッグコマンドに出力
	DebugCommand::SendPrintSci("### SYSTEM ERROR ### \r\n");
	ClsCommon::PrintInfoLine(DebugCommand::SendPrintSci, " - FileName: ", pFileName);
	ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " - Line: ", line);
	ClsCommon::PrintInfoLine(DebugCommand::SendPrintSci, " - TaskName: ", pTaskName);
	ClsCommon::PrintInfoLine(DebugCommand::SendPrintSci, " - Message: ", pMessage);
    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " - TimeTick: ", timeTick);

	if (info[0].CounterSysErrorOcuurs <= MS_LIMIT_SYS_ERROR_ROM_UPDATE) 
	{
		// 履歴にコピー
		info[2] = info[1];
		info[1] = info[0];

		// 情報をセット
		info[0].CounterSysErrorOcuurs++;
		info[0].TimeTickWhenSysErrorOcuurs = timeTick;
		memset(info[0].FileNameWhenSysErrOccurs, 0, sizeof(info[0].FileNameWhenSysErrOccurs));
		memcpy(info[0].FileNameWhenSysErrOccurs, pFileName, GetMin((uint32_t)strlen(pFileName), (uint32_t)sizeof(info[0].FileNameWhenSysErrOccurs) - 1));  // \0を確保
		info[0].LineWhenSysErrOccurs = line;
		memset(info[0].TaskNameWhenSysErrOccurs, 0, sizeof(info[0].TaskNameWhenSysErrOccurs));
		memcpy(info[0].TaskNameWhenSysErrOccurs, pTaskName, GetMin((uint32_t)strlen(pTaskName), (uint32_t)sizeof(info[0].TaskNameWhenSysErrOccurs) - 1)); // \0を確保
		memset(info[0].MessageWhenSysErrOccurs, 0, sizeof(info[0].MessageWhenSysErrOccurs));
		memcpy(info[0].MessageWhenSysErrOccurs, pMessage, GetMin((uint32_t)strlen(pMessage), (uint32_t)sizeof(info[0].MessageWhenSysErrOccurs) - 1)); // \0を確保

		err = E_OK;
	}

	return err;
}

/*!------------------------------------------------
@brief [DataStoredInDataFlashクラス] ROMに保存されている情報をロードする。必要に応じてチェックサムも計算する。
@note　
@return 正常 E_OK、異常：エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR BaseInDataFlash::Load()
{
    // データROMに保存されている前回終了時の情報をロードする

#ifdef WINDOWS_DEBUG
    std::cout << "#### DataStoredInDataFlash::Load() #### \r\n";

	if (GetVeeRecId() == 1) 
	{
		DataStoredInDataFlash* pObj = (DataStoredInDataFlash*)this;

		// 前回終了時データデバッグ用データ
		pObj->info.ArrayLedStatusForInit[0] = 'U'; // 赤
		pObj->info.ArrayLedStatusForInit[1] = 'T'; // 緑
		pObj->info.ArrayLedStatusForInit[2] = 'V'; // 青
		pObj->info.ArrayLedStatusForInit[3] = 'W'; // 橙
	}

    TYPE_ERROR err = E_OK;
#else

//    DebugCommand::SendPrintSci("\r\n###### Load() Data ROM use RM_VEE_FLASH START ######\r\n");
//    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "VeeRecId = ", GetVeeRecId());
//    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci,"Size=", GetRecSize());

    TYPE_ERROR err = E_ELSE;

    fsp_err_t err_t = RM_VEE_FLASH_Open(g_vee0.p_ctrl, g_vee0.p_cfg);
//    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\nRM_VEE_FLASH_Open() = ", err_t);
    if(FSP_SUCCESS == err_t)
    {
        // データFlashに格納されているデータのポインタを得て、Flashからデータをロード
        uint32_t len = 0;
        uint8_t* pBufOnFlash = 0;
        err = E_ELSE;

        err_t = RM_VEE_FLASH_RecordPtrGet(g_vee0.p_ctrl, GetVeeRecId(), &pBufOnFlash, &len);
//        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\nRM_VEE_FLASH_RecordPtrGet() = ", err_t);

        if(FSP_SUCCESS == err_t)
        {
//            ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "len = ", len);
            if(len == GetRecSize())
            {
//                DebugCommand::SendPrintSci("\r\n ## Restore previous data OK ##\r\n");
                ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "VeeRecId = ", GetVeeRecId());
                ClsCommon::PrintInfoHex(DebugCommand::SendPrintSci, "pBufOnFlash = ", (uint32_t)pBufOnFlash);

                // Flashからデータをロード（コピー中は割込禁止）
                DEFINE_INT();
                DISABLE_INT();
                memcpy((void*)GetBufferPointer(), pBufOnFlash, len);
                ENABLE_INT();

                // コピー後処理
                AfterLoad();

                err = E_OK;
            }
        }

        RM_VEE_FLASH_Close(g_vee0.p_ctrl);
	}

//    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\n Done Load()  = ", err);

#endif

	return err;
}


static uint32_t __debugCounterDataFlashInterval = 0;
static uint32_t __debugCounterDataFlashCheckd = 0;
static uint32_t __debugCounterDataFlashUpdated = 0;

/*!------------------------------------------------
@brief [DataStoredInDataFlashクラス] 情報が変化していたら、データROMを更新する
@note　
@return なし
@author
-------------------------------------------------*/
void BaseInDataFlash::Update()
{
#ifdef WINDOWS_DEBUG
	std::cout << "#### DataStoredInDataFlash::Update() #### \r\n";
    __debugCounterDataFlashInterval ++;
    __debugCounterDataFlashCheckd ++;
    __debugCounterDataFlashUpdated ++;

#else

//    DebugCommand::SendPrintSci("\r\n###### Update() Data ROM use RM_VEE_FLASH START ######\r\n");

    __debugCounterDataFlashInterval ++;

    fsp_err_t err_t = RM_VEE_FLASH_Open(g_vee0.p_ctrl, g_vee0.p_cfg);
//    ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\nRM_VEE_FLASH_Open() = ", err_t);
    if(FSP_SUCCESS == err_t)
    {
        __debugCounterDataFlashCheckd ++;

        // データFlashに格納されているデータのポインタを得る
		uint32_t len = 0;
		uint8_t* pBufOnFlash = 0;
		uint8_t isChanged = ON;

        err_t = RM_VEE_FLASH_RecordPtrGet(g_vee0.p_ctrl, GetVeeRecId(), &pBufOnFlash, &len);
//        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\nRM_VEE_FLASH_RecordPtrGet() =", err_t);
        if (FSP_SUCCESS == err_t)
		{
//            ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "len = ", len);

            if (len == GetRecSize())
			{
//	            ClsCommon::PrintInfoHex(DebugCommand::SendPrintSci, "pBufOnFlash = ", (uint32_t)pBufOnFlash);

                // Flashからデータをロードし、前回更新した値と違いがないか確認する（確認中は割込み禁止）
                DEFINE_INT();
                DISABLE_INT();
                isChanged = (0==memcmp(GetBufferPointer(), pBufOnFlash, len)) ? OFF : ON;
                ENABLE_INT();
			}
//			ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "isChanged = ", isChanged);
		}

		if(isChanged)   // ROMに書き込んでいるデータとバッファとに差があれば書き込む
		{
            // データFlashに MemoryBuf の中身を書き込む
            g_write_flag = OFF;

            {   // 排他対応の為、一時バッファにコピーした後に書き込む
                // RM_VEE_FLASH が割込を使っているので、RM_VEE_FLASH_RecordWrite関数の前後で割込禁止に出来ない為
                static uint8_t tempBuffer[SZ_ROM_WTMP_BUF_SIZE];
                if(GetRecSize() > SZ_ROM_WTMP_BUF_SIZE){
                    THROW_FATAL_ERROR_PROGRAM_LOGIC("DataManager");
                }

                // 一時バッファにコピー（コピー中は割込み禁止）
                DEFINE_INT();
                DISABLE_INT();
                memcpy(tempBuffer, (uint8_t *)GetBufferPointer(), GetRecSize());
                ENABLE_INT();

                // データFlashに書き込む
                err_t = RM_VEE_FLASH_RecordWrite(g_vee0.p_ctrl, GetVeeRecId(), tempBuffer, GetRecSize());
            }

	        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\nRM_VEE_FLASH_RecordWrite() =", err_t);
	        if(err_t == FSP_SUCCESS)
	        {
	            /* Wait for the Virtual EEPROM callback to indicate that it has finished writing data and vee flash is in ready state. */
	            volatile uint16_t write_time_out = 0xFFFF;
	            while (ON != g_write_flag)
	            {
                    G_pDrvWDT->Refresh();
	                write_time_out --;
	                if (0 == write_time_out)
	                {
	                    // callback event did not occur
		                DebugCommand::SendPrintSci("\r\n## DataFlash Write Timeout ##\r\n");

	                    // エラー
						break;
	                }
	            }

	            __debugCounterDataFlashUpdated ++;
	        }
	        else
	        {
                DebugCommand::SendPrintSci("\r\n## DataFlash Write Error ##\r\n");

	            // エラー
	        }

	        /// ここからは、テストコード
//	        err_t = RM_VEE_FLASH_RecordPtrGet(g_vee0.p_ctrl, GetVeeRecId(), &pBufOnFlash, &len);
//	        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "\r\n[Try] RM_VEE_FLASH_RecordPtrGet() =", err_t);
//	        if (FSP_SUCCESS == err_t)
//	        {
//	            ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "len = ", len);
//	            ClsCommon::PrintInfoHex(DebugCommand::SendPrintSci, "pBufOnFlash = ", (uint32_t)pBufOnFlash);
//
//	            if (len == GetRecSize())
//	            {
//	                DebugCommand::SendPrintSci("## SIZE OK ##\r\n");
//	            }
//	        }

		}

        RM_VEE_FLASH_Close(g_vee0.p_ctrl);
    }

//    DebugCommand::SendPrintSci("\r\n Done UPDATE()\r\n");

#endif
}

/*!------------------------------------------------
@brief [DataStoredInDataFlashクラス] データFlashの強制消去
@note　通常はライブラリが自動的に消去する
@return なし
@author
-------------------------------------------------*/
void BaseInDataFlash::Format()
{
#ifndef WINDOWS_DEBUG
    static rm_vee_status_t p_status = {(rm_vee_state_t)0, 0, 0, 0};
    uint8_t ref_data = 0;
	fsp_err_t err_t = RM_VEE_FLASH_Open(g_vee0.p_ctrl, g_vee0.p_cfg);
	if (FSP_SUCCESS == err_t)
	{
		/* Start a manual format operation.*/
		err_t = RM_VEE_FLASH_Format(g_vee0.p_ctrl, &ref_data);
		if (FSP_SUCCESS != err_t)
		{
			DebugCommand::SendPrintSci("\r\n## RM_VEE_FLASH_Format() Failed ##\r\n");
			return;
		}

		/* Get the current status of the driver.*/
		err_t = RM_VEE_FLASH_StatusGet(g_vee0.p_ctrl, &p_status);
		if (FSP_SUCCESS != err_t)
		{
            DebugCommand::SendPrintSci("\r\n## RM_VEE_FLASH_StatusGet() Failed ##\r\n");
			return;
		}

		/* Compare Last ID written with Default ID.*/
		if (0xffff == p_status.last_id)
		{
            DebugCommand::SendPrintSci("\r\n## Format OK ##\r\n");
		}
		else
		{
	        ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "p_status.last_id = ", p_status.last_id);
            DebugCommand::SendPrintSci("\r\n## Not Erased ##\r\n");
		}
	}

#endif
}


/*!------------------------------------------------
@brief タスクの情報をデバッグコマンドでデバッグ出力する
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo(void (*funcPrint)(const char*))
{
#ifndef USE_PRINT_DEBUG_DATAMANAGER
    (void) funcPrint;
#else
    PrintInfo_InitialInfo(funcPrint);
    DummyDelay(0xffff);
    PrintInfo_DataStoredInDataFlash(funcPrint);
    DummyDelay(0xffff);
    PrintInfo_SystemErrorStoredInDataFlash(funcPrint);
    DummyDelay(0xffff);
    PrintInfo_Board(funcPrint);
	DummyDelay(0xffff);
	PrintInfo_HwStatus(funcPrint);
	DummyDelay(0xffff);
    PrintInfo_ForDebug(funcPrint);
#endif

}


/*!------------------------------------------------
@brief タスクの情報をデバッグコマンドでデバッグ出力する（データフラッシュのバッファのみ）
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfoDataFlashBuffer(void (*funcPrint)(const char*))
{
	PrintInfo_DataStoredInDataFlash(funcPrint);
	DummyDelay(0xffff);
	PrintInfo_SystemErrorStoredInDataFlash(funcPrint);
	DummyDelay(0xffff);
}

#ifdef USE_PRINT_DEBUG_DATAMANAGER

/*!------------------------------------------------
@brief タスクのイニシャル設定情報をデバッグコマンドでデバッグ出力する
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_InitialInfo(void (*funcPrint)(const char*))
{
	funcPrint("[InitialInfo - Mifare1]\r\n");
	const char* pKeyType1
		= _InitialInfo.Mifare1_KeyType == INITIAL_MIFARE_KEYA ? "KeyA"
		: _InitialInfo.Mifare1_KeyType == INITIAL_MIFARE_KEYB ? "KeyB"
		: "Invalid";
	ClsCommon::PrintInfoLine(funcPrint, "KeyType: ", pKeyType1);
	ClsCommon::PrintInfoDump(funcPrint, "KeyData:", _InitialInfo.Mifare1_KeyData, sizeof(_InitialInfo.Mifare1_KeyData));
	ClsCommon::PrintInfoDump(funcPrint, "Block:", &_InitialInfo.Myfare1_Block, 1);
	funcPrint("\r\n");

	funcPrint("[InitialInfo - Mifare2]\r\n");
	const char* pKeyType2
		= (_InitialInfo.Mifare2_KeyType == INITIAL_MIFARE_KEYA) ? "KeyA"
		: (_InitialInfo.Mifare2_KeyType == INITIAL_MIFARE_KEYB) ? "KeyB"
		: "Invalid";
	ClsCommon::PrintInfoLine(funcPrint, "KeyType: ", pKeyType2);
	ClsCommon::PrintInfoDump(funcPrint, "KeyData:", _InitialInfo.Mifare2_KeyData, sizeof(_InitialInfo.Mifare2_KeyData));
	ClsCommon::PrintInfoDump(funcPrint, "Block:", &_InitialInfo.Myfare2_Block, 1);
	funcPrint("\r\n");

	funcPrint("[InitialInfo - FeliCa]\r\n");
	const char* pValidBlock1
		= (_InitialInfo.Ferica_ValidBlock1 == INITIAL_FERICA_VALID) ? "Valid"
		: "Invalid";
	ClsCommon::PrintInfoLine(funcPrint, "ValidBlock1: ", pValidBlock1);
	const char* pValidBlock2
		= (_InitialInfo.Ferica_ValidBlock2 == INITIAL_FERICA_VALID) ? "Valid"
		: "Invalid";
	ClsCommon::PrintInfoLine(funcPrint, "ValidBlock2: ", pValidBlock2);
	ClsCommon::PrintInfoDump(funcPrint, "SystemCd:", _InitialInfo.Ferica_SystemCd, sizeof(_InitialInfo.Ferica_SystemCd));
	ClsCommon::PrintInfoDump(funcPrint, "ServiceCd:", _InitialInfo.Ferica_ServiceCd, sizeof(_InitialInfo.Ferica_ServiceCd));
	ClsCommon::PrintInfoDump(funcPrint, "Block1:", &_InitialInfo.Ferica_Block1, 1);
	ClsCommon::PrintInfoDump(funcPrint, "Block2:", &_InitialInfo.Ferica_Block2, 1);
	funcPrint("\r\n");
}

#endif

/*!------------------------------------------------
@brief タスクのデータフラッシュ書込情報をデバッグコマンドでデバッグ出力する
@note　
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_DataStoredInDataFlash(void (*funcPrint)(const char*))
{
	funcPrint("[DataStoredInDataFlash]\r\n");
	ClsCommon::PrintInfoDump(funcPrint, "MemoryBuf:", _DataStoredInDataFlash.info.MemoryBuf, sizeof(_DataStoredInDataFlash.info.MemoryBuf));
	funcPrint("LED: '");
	ClsCommon::DebugPrintBuf(funcPrint, _DataStoredInDataFlash.info.ArrayLedStatusForInit, sizeof(_DataStoredInDataFlash.info.ArrayLedStatusForInit));
	funcPrint("' - ");
	ClsCommon::DebugPrintDataHex(funcPrint, _DataStoredInDataFlash.info.ArrayLedStatusForInit, sizeof(_DataStoredInDataFlash.info.ArrayLedStatusForInit));
	funcPrint("\r\n");
    funcPrint("\r\n");
}


/*!------------------------------------------------
@brief タスクのデータフラッシュ書込情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_SystemErrorStoredInDataFlash(void (*funcPrint)(const char*))
{
    funcPrint("[SystemErrorStoredInDataFlash]\r\n");
    if(_SystemErrorStoredInDataFlash.info[0].CounterSysErrorOcuurs == 0)
    {
        funcPrint(" (No data)\r\n");
    }
    else
    {
        for(uint32_t i=0; i<3; i++)
        {
            if(_SystemErrorStoredInDataFlash.info[i].CounterSysErrorOcuurs > 0)
            {
                ClsCommon::PrintInfoNum(funcPrint, "Counter: ", _SystemErrorStoredInDataFlash.info[i].CounterSysErrorOcuurs);
                ClsCommon::PrintInfoNum(funcPrint, " -- TimeTick: ", _SystemErrorStoredInDataFlash.info[i].TimeTickWhenSysErrorOcuurs);
                ClsCommon::PrintInfoLine(funcPrint, " -- FileName: ", _SystemErrorStoredInDataFlash.info[i].FileNameWhenSysErrOccurs);
                ClsCommon::PrintInfoNum(funcPrint, " -- Line: ", _SystemErrorStoredInDataFlash.info[i].LineWhenSysErrOccurs);
                ClsCommon::PrintInfoLine(funcPrint, " -- TaskName: ", _SystemErrorStoredInDataFlash.info[i].TaskNameWhenSysErrOccurs);
                ClsCommon::PrintInfoLine(funcPrint, " -- Message: ", _SystemErrorStoredInDataFlash.info[i].MessageWhenSysErrOccurs);
            }
        }
    }

    funcPrint("\r\n");
}

/*!------------------------------------------------
@brief タスクのボード情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_Board(void (*funcPrint)(const char*))
{
	funcPrint("[Board]\r\n");

	// ロードした設定値をデバッグコマンドに出力
	const uint8_t dipsw1 = GetInfoDipSw1();
	const uint8_t dipsw2 = GetInfoDipSw2();
	ClsCommon::PrintInfoDump(DebugCommand::SendPrintSci, "DIPSW1: ", &dipsw1, 1);
	ClsCommon::PrintInfoDump(DebugCommand::SendPrintSci, "DIPSW2: ", &dipsw2, 1);
	ClsCommon::PrintInfoHex(funcPrint, "_ChecksumRunningFw: ", _ChecksumRunningFw);
	funcPrint("\r\n");
}

#ifdef USE_PRINT_DEBUG_DATAMANAGER

/*!------------------------------------------------
@brief タスクのハードウエア情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_HwStatus(void (*funcPrint)(const char*))
{
	funcPrint("[HwStatus]\r\n");

	ClsCommon::PrintInfoNum(funcPrint, "_IsNfcStarted: ", _IsNfcStarted);
	ClsCommon::PrintInfoNum(funcPrint, "_IsQrStarted: ", _IsQrStarted);
	ClsCommon::PrintInfoNum(funcPrint, "_IsDoorNormality: ", _IsDoorNormality);
	ClsCommon::PrintInfoNum(funcPrint, "_IsEepromNormality: ", _IsEepromNormality);
	funcPrint("\r\n");
}

/*!------------------------------------------------
@brief タスクのデバッグ情報をデバッグコマンドでデバッグ出力する
@note
@return なし
@author
-------------------------------------------------*/
void DataManager::PrintInfo_ForDebug(void (*funcPrint)(const char*))
{
    funcPrint("[ForDebug]\r\n");

    funcPrint("DebugCounterDataFlash:\r\n");
    ClsCommon::PrintInfoNum(funcPrint, " -- Interval: ", __debugCounterDataFlashInterval);
    ClsCommon::PrintInfoNum(funcPrint, " -- Checked: ", __debugCounterDataFlashCheckd);
    ClsCommon::PrintInfoNum(funcPrint, " -- Updated: ", __debugCounterDataFlashUpdated);

#ifndef WINDOWS_DEBUG

    fsp_err_t err_t = RM_VEE_FLASH_Open(g_vee0.p_ctrl, g_vee0.p_cfg);
    if(FSP_SUCCESS == err_t)
    {
        for(uint32_t id=1; id<=2; id++)
        {
            uint32_t len = 0;
            uint8_t* pBufOnFlash = 0;

            ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, "VeeRecId = ", id);
            err_t = RM_VEE_FLASH_RecordPtrGet(g_vee0.p_ctrl, id, &pBufOnFlash, &len);
            if (FSP_SUCCESS == err_t)
            {
                ClsCommon::PrintInfoHex(funcPrint, " -- pBufOnFlash = ", (uint32_t)pBufOnFlash);
                ClsCommon::PrintInfoNum(funcPrint, " -- len = ", len);
            }
            else
            {
                funcPrint(" -- Data is not retained in Data Flash\r\n");
            }
        }

        RM_VEE_FLASH_Close(g_vee0.p_ctrl);
    }
    else
    {
        funcPrint("RM_VEE_FLASH_Open() Error\r\n");
    }

#endif

}

#endif

/*!------------------------------------------------
@brief 強制遅延
@note
@param num ループさせる回数
@return なし
@author
-------------------------------------------------*/
void DataManager::DummyDelay(uint32_t num)
{
    volatile uint32_t write_time_out = num;
    for (; write_time_out != 0; write_time_out--)
    {
    }
}
