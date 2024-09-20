/*!------------------------------------------------
@brief コマンド受信タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

CommandReceiver::CommandReceiver()
	:BaseTask(ST_CMDRCV_MAX, EVT_CMDRCV_MAX, TIMER_CMDRCV_MAX, CMDRCV_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_ONESHOT, EVT_CMDRCV_TMOUT_RCV_INST);

	// 停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_CMDRCV_DORMANT);
	st->AddEventHandler(EVT_CMDRCV_START, Dormant_Start);

	// 受信中状態へ各ハンドラ登録
	st = this->GetState(ST_CMDRCV_RECEIVING);
	st->AddEventHandler(EVT_CMDRCV_RD_DATA_SENDED, Xxx_RdDataSended);			// 読取データ送信完
	st->AddEventHandler(EVT_CMDRCV_PACKET_RECEIVED, Xxx_PacketReceived);		// パケットを受信した
	st->AddEventHandler(EVT_CMDRCV_IDLE, Xxx_Idle);			// Idle

	// 指示コマンド待ち状態へ各ハンドラ登録
	st = this->GetState(ST_CMDRCV_WAITING_INST);
	st->AddEventHandler(EVT_CMDRCV_RD_DATA_SENDED, Xxx_RdDataSended);			// 読取データ送信完
	st->AddEventHandler(EVT_CMDRCV_PACKET_RECEIVED, Xxx_PacketReceived);		// パケットを受信した
	st->AddEventHandler(EVT_CMDRCV_TMOUT_RCV_INST, WaitingInst_TmoutRcvInst);	// 指示コマンド受信タイムアウト
	st->AddEventHandler(EVT_CMDRCV_IDLE, Xxx_Idle);			// Idle
	st->AddEnterHandler(WaitingInst_Enter);	// Enter
	st->AddExitHandler(WaitingInst_Exit);	// Exit


	// 再起動実行状態へ各ハンドラ登録
	st = this->GetState(ST_CMDRCV_DO_REBOOT);
	st->AddEventHandler(EVT_CMDRCV_IDLE, DoReboot_Idle);	// Idle

	// 読取データ送信完カウンタ初期化
	_CntRdDataSended = 0;

	// 受信パケットキュー初期化
	memset((uint8_t*)&_RcvDataQue, 0, sizeof(_RcvDataQue));
	_PosRcvDataQue = 0;

	// 受信パケット抽出リセット
	ResetReceivedPacket();

}

/*!------------------------------------------------
@brief 停止状態 － 開始イベント処理
@note　通信可能な状態になった時、外部から通知される
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	CommandReceiver* pMyTask = (CommandReceiver*) pOwnTask;

	//RS485初期化処理
#ifdef WINDOWS_DEBUG
#else
	TYPE_ERROR err = E_OK;
    //FETポートをOFF（トランシーバ電源を有効に）
	err = G_pDrvIOPort->Write(RS485_PO,BSP_IO_LEVEL_LOW);
#ifdef RS485_ERROR_DEBUG_PORT_485_FET
	err = E_SYS;
#endif /* RS485_ERROR_DEBUG_PORT_485_FET */
    if(err != E_OK)
    {
        //RS485：FETポート 出力異常フラグを立てる
        G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_485_OFFSET,ERRORFLAG_PORT_485_FET);
    }
    //RS485_REの初期化処理（常時に受信可能(常時LOW)）

    err = G_pDrvIOPort->Write(RS485_RE,BSP_IO_LEVEL_LOW);
#ifdef RS485_ERROR_DEBUG_PORT_485_RE_A
    err = E_SYS;
#endif /* RS485_ERROR_DEBUG_PORT_485_RE_A */
    if(err != E_OK)
    {
        //RS485：プライマリRE異常フラグを立てる
        G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_485_OFFSET,ERRORFLAG_PORT_485_RE_A);
    }
    //RS485_DEの初期化処理
    err = G_pDrvIOPort->Write(RS485_DE,BSP_IO_LEVEL_LOW);
#ifdef RS485_ERROR_DEBUG_PORT_485_DE_A
    err = E_SYS;
#endif /* RS485_ERROR_DEBUG_PORT_485_DE_A */
    if(err != E_OK)
    {
        //RS485：プライマリDE異常フラグを立てる
        G_pDataManager->SetErrorFlag(ERRORFLAG_PORT_485_OFFSET,ERRORFLAG_PORT_485_DE_A);
    }

    //受信バッファをクリア
    G_pDrvUART_RS485->ClearRxBuffer();
    //Uartのリセット
    pMyTask->UartCheckAndReset();
#endif
	G_pEventSender->SendEvent(EVT_EVTSND_START);
	// 動作中に遷移
	pMyTask->SetNextStateId(ST_CMDRCV_RECEIVING);
}


/*!------------------------------------------------
@brief 通常受信中・指示コマンド待ち状態 共通 － 読取データ送信完 イベント処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::Xxx_RdDataSended(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	// 衝突検知
	pMyTask->CheckCollision();

	(pMyTask->_CntRdDataSended)++;
	if (pMyTask->_CntRdDataSended >= OVER_CNT_NO_RCV_INST) 
	{
		// 指示コマンドを受信せず、指定回数読取データを送信したら、指示コマンド待ちに遷移
		pMyTask->SetNextStateId(ST_CMDRCV_WAITING_INST);
	}
}


/*!------------------------------------------------
@brief 通常受信中・指示コマンド待ち状態 共通 － パケットを受信した イベント処理
@note　受信したデータ部の中身を確認し、それに応じた処理を行う
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : パケットバッファのポインタ（STX、ETX、BCCを除くデータ部のみ格納）
@param DataLen : 受信パケットのデータ部サイズ
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::Xxx_PacketReceived(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	if ((pData == NULL) || (DataLen == 0))
	{	// 受信したデータ部の長さ、ポインタが渡されていない筈はない
		THROW_FATAL_ERROR_PAR(pMyTask->GetTaskName());	// パラメータエラー
	}

	// 衝突検知
	pMyTask->CheckCollision();

	// デバッグ用　コンソール上で受信パケットを確認
	//std::cout << "Received Len=" << DataLen;
	//std::cout << " Data:";
	for (uint32_t i = 0; i < DataLen; i++)
	{
		//std::cout << pData[i];
	}
	//std::cout << "\r\n";

	switch (DataLen) 
	{
		case DT_LEN_INST_CMD:		// 指示コマンド
		    G_pDebugCommand->SetDebugTime(6);
		    //指示コマンドを受信したら動作中に遷移
            pMyTask->SetNextStateId(ST_CMDRCV_RECEIVING);
		    //読取データ送信完カウンタをリセット
		    pMyTask->_CntRdDataSended = 0;
		    //累積回数：指示コマンド受信回数＋１
            G_pDataManager->AddEventCount(EVENT_RSVCONTROL);
			// 指示コマンドに対するイベントをデータ管理と出力ポート制御に通知
			pMyTask->SendInstructEvent_To_DataManager_And_OutputController(pData);
			break;

		case DT_LEN_STATUS_REQ:		// ステータス要求コマンド
		    //累積回数：ステータス要求受信回数＋１
            G_pDataManager->AddEventCount(EVENT_RSVSTATUS);
			// イベント送信にリセットデータ送信要求
			G_pEventSender->SendEvent(EVT_EVTSND_REQ_RESET_DATA_STATUS);
			break;

		case DT_LEN_INITIAL_CMD:	// イニシャルコマンド
            //累積回数：イニシャルコマンド受信回数＋１
            G_pDataManager->AddEventCount(EVENT_RSVINITIAL);
			// データ管理にイニシャル（NFCリーダの初期設定）通知
			G_pDataManager->SendEvent(EVT_DATA_SET_INITIAL, pData, DataLen);
			// イベント送信にリセットデータ送信要求
			G_pEventSender->SendEvent(EVT_EVTSND_REQ_RESET_DATA_INIT);
			break;

		default:				
			if (pData[POS_MENTE_CD] == MENTE_CD) 	// メンテコード
			{
				switch (pData[POS_MENTE_TYPE]) 
				{
					case MENTE_TYPE_MEM_REQ:		// メモリデータ要求
						if (DataLen == DT_LEN_MENTE_MEM_REQ)
						{
						    uint32_t offsetVal = 0;

						    ClsCommon::AsciiHex2Uint32((const char*)&pData[3], 8, &offsetVal);
						    if(offsetVal > 144)
						    {
						        //オフセットの値は0~144(0x00~90)なはず
						        //144より大きい場合、何もしない（イベント送信しない）
						    }
						    else
						    {
						        // イベント送信にメモリデータ送信要求
                                G_pEventSender->SendEvent(EVT_EVTSND_REQ_MEMORY_DATA, pData, DataLen);
						    }
						}
						break;

					case MENTE_TYPE_CLS_CUMULATIVE:	// 累積回数クリア
						if (DataLen == DT_LEN_MENTE_CLS_CUMULATIVE)
						{
							// データ管理に累積回数クリア通知
							G_pDataManager->SendEvent(EVT_DATA_CLS_CUMULATIVE);
						}
						break;

					case MENTE_TYPE_GET_VERSION:	// バージョン・チェックサム要求
						if (DataLen == DT_LEN_MENTE_GET_VERSION)
						{
							// イベント送信にバージョン・チェックサム送信要求
							G_pEventSender->SendEvent(EVT_EVTSND_REQ_VER_DATA);
						}
						break;

					case MENTE_TYPE_DLL_CONT:       // FW更新 制御
						switch (pData[POS_MENTE_SUBTYPE])
						{
							case MENTE_SUBTYPE_DLL_START:        // FW更新 ダウンロード開始
								if (DataLen == DT_LEN_MENTE_DLL_START)
								{
									// FW更新にダウンロード開始を通知
									G_pFwUpdater->SendEvent(EVT_FW_DLL_START);
								}
								break;

							case MENTE_SUBTYPE_DLL_UPDATE:       // FW更新 更新実行
								if (DataLen == DT_LEN_MENTE_DLL_UPDATE)
								{
									// FW更新に更新実行を通知
									G_pFwUpdater->SendEvent(EVT_FW_DLL_UPDATE);
								}
								break;

							default:	// 無視
								break;
						}
						break;

					case MENTE_TYPE_DLL_DATA:		// FW更新 データパケット受信
						if (DataLen == DT_LEN_MENTE_DLL_DATA)
						{
							// FW更新に受信したデータパケットを通知
							G_pFwUpdater->SendEvent(EVT_FW_DLL_DATA, pData, DataLen);
						}
						break;

                    case MENTE_TYPE_FLASH_FORMAT:   //データフラッシュ初期化
                        if (DataLen == DT_LEN_MENTE_FLASH_FORMAT)
                        {
                            // データフラッシュを初期化
                            G_pDataManager->DataFlashFormat();
                        }
                        break;

                    case MENTE_TYPE_REBOOT:   //再起動
                        if (DataLen == DT_LEN_MENTE_REBOOT)
                        {
                            // 再起動実行
                            G_pDataManager->SystemSaveAndReboot();
                        }
                        break;

                    case MENTE_TYPE_GET_VER_INFO:   //バージョン情報取得
                        if (DataLen == DT_LEN_MENTE_GET_VER_INFO)
                        {
                            // イベント送信にバージョン情報取得を通知
                            G_pEventSender->SendEvent(EVT_EVTSND_REQ_VER_INFO_DATA);
                        }
                        break;

					default:	// 無視
						break;
				}
			}
			break;
	}
}


/*!------------------------------------------------
@brief 通常受信中・指示コマンド待ち状態 共通 － Idle処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::Xxx_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	// 衝突検知
	pMyTask->CheckCollision();

	// 受信パケットを抽出し、内部イベントとして通知
	pMyTask->ExtractReceivedPacketAndNoticeInnerEvent();
}

/*!------------------------------------------------
@brief 指示コマンド待ち状態 － 指示コマンド受信タイムアウト イベント処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::WaitingInst_TmoutRcvInst(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	// 再起動に遷移
	pMyTask->SetNextStateId(ST_CMDRCV_DO_REBOOT);
}

/*!------------------------------------------------
@brief 指示コマンド待ち状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::WaitingInst_Enter(BaseTask* pOwnTask)
{
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	pMyTask->StartTimer(EVT_CMDRCV_TMOUT_RCV_INST, MS_TMOUT_RCV_INST);
}

/*!------------------------------------------------
@brief 指示コマンド待ち状態 － Exit処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::WaitingInst_Exit(BaseTask* pOwnTask)
{
	CommandReceiver* pMyTask = (CommandReceiver*)pOwnTask;

	pMyTask->StopTimer(EVT_CMDRCV_TMOUT_RCV_INST);
}


/*!------------------------------------------------
@brief 再起動実行 状態 － Idle
@note　ここに入ったら（デバッグコマンドなど出力出来るよう）一定時間待って、再起動させる
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::DoReboot_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)pOwnTask;(void)EventId;(void)pData;(void)DataLen;    // 未使用引数のワーニング対策

    //累積回数：通信無しリセット回数＋１
    G_pDataManager->AddEventCount(EVENT_ERROR_HOST_NO_COM);
    // ROM書き込み後、リブート
    G_pDataManager->SystemSaveAndReboot();
}


/*!------------------------------------------------
@brief 指示コマンドに対するイベントをデータ管理と出力ポート制御に通知
@note　
@param pData :受信した指示コマンドのデータ部
@author
-------------------------------------------------*/
void CommandReceiver::SendInstructEvent_To_DataManager_And_OutputController(const uint8_t* pData)
{
	// LED点灯指示に有効データ（）があるか確認する
	uint8_t isEnableLed = 0;
	uint8_t isEnableLedWriteRom = 0;
	for (uint16_t index = 0; index < LEN_LED_IN_INST_CMD; index++)
	{
		if(IsEnableLedCommand(pData[POS_LED_IN_INST_CMD + index]))
		{
			isEnableLed = 1; // 有効データあり
		}

		if (IsEnableLedCommandWriteRom(pData[POS_LED_IN_INST_CMD + index]))
		{
			isEnableLedWriteRom = 1; // 有効データあり
		}
	}

	// LED点灯指示をデータ管理タスクと出力ポート制御に通知
	if (isEnableLed)
	{
	    G_pOutputController->SendEvent(EVT_OUTPUT_LED, pData + POS_LED_IN_INST_CMD, LEN_LED_IN_INST_CMD);
	}
	if (isEnableLedWriteRom)
	{
		G_pDataManager->SendEvent(EVT_DATA_LED, pData + POS_LED_IN_INST_CMD, LEN_LED_IN_INST_CMD);
	}

	// ブザー鳴動指示を出力ポート制御に通知
	if (IsEnableBuzzerCommand(pData[POS_BUZZER_IN_INST_CMD]))
	{
		G_pOutputController->SendEvent(EVT_OUTPUT_BUZZER, pData + POS_BUZZER_IN_INST_CMD, LEN_BUZZER_IN_INST_CMD);
	}

	// 自動扉出力指示を出力ポート制御に通知
	if (IsEnableAutodoorCommand(pData[POS_AUTODOOR_IN_INST_CMD]))
	{
		G_pOutputController->SendEvent(EVT_OUTPUT_AUTODOOR, pData + POS_AUTODOOR_IN_INST_CMD, LEN_AUTODOOR_IN_INST_CMD);
	}
}


/*!------------------------------------------------
@brief 衝突検知
@note　受信データがあったら衝突を確認し、コマンド送信タスクに衝突検知を通知
@return 
@author
-------------------------------------------------*/
void CommandReceiver::CheckCollision()
{
#ifndef DEBUG_NO_COLLISION
	TYPE_ERROR err = E_OK;

	// 直近に送信データがあるか、イベント送信タスクから確認する
	uint32_t lenLatestSend = G_pEventSender->GetLatestSendLen();
	if (lenLatestSend > 0) 
	{
        err = ExtractReceivedPacket();
	    if(err == E_EMP)
        {
            //データを受信するまでブロックする
            //timer割り込みが1ms間隔の為、50msを保証する為に _timeOutVal の定義を 51msにしている
            uint32_t startTime = GetTimerTick();
            //タイムアウト処理
            while(IsTimerIsExpired(startTime,_timeOutVal) == 0)
            {
                err = ExtractReceivedPacket();
                if(err != E_EMP)
                {
                    //受信したら抜ける
                    break;
                }
            }
        }
		if (err == E_OK)
		{
			// イベント送信タスクより送信データのポインタを取得
			const uint8_t* pLatestSend = G_pEventSender->GetLatestSendData();

			// 送信したデータと同じデータを受信したか確認する
			if (memcmp(&_RcvDataQue[_PosRcvDataQue][2], pLatestSend, lenLatestSend) != 0)
			{
				// 違っていたら衝突とみなす
				err = E_COLLISION;
			}
		}

#ifdef RS485_ERROR_DEBUG_HOST_COLLISION
	    {   // 起動直後のリセットデータ通知だけはエラーとしない。その後は衝突による再送を繰り返し再起動することになる。
    		static uint8_t isDebugError = 0;
    		if(isDebugError)
    		{
    		    err = E_COLLISION;
    		}
	        else
	        {
        		isDebugError = 1;
	        }
	    }
#endif /* RS485_ERROR_DEBUG_HOST_COLLISION */
#ifdef RS485_ERROR_DEBUG_RECOVERY_FROM_COLLISION
	    {   // 初回エラー、次は成功を繰り返す： 衝突による再送１回をシミュレート
    		static uint8_t debug_counter = 0;
    		if(debug_counter%2 == 0)
    		{
    		    err = E_COLLISION;
    		}
    		debug_counter++;
	    }
#endif /* RS485_ERROR_DEBUG_RECOVERY_FROM_COLLISION */

		if (err != E_OK) 
		{
			// イベント送信タスクに衝突検知をセット
			G_pEventSender->SetCollisionDetected();
			// 受信パケット抽出リセット
			ResetReceivedPacket();
		}
	}
#endif
}

/*!------------------------------------------------
@brief 受信パケットを抽出し、内部イベントとして通知
@note　
@return エラーコード
@author
-------------------------------------------------*/
void CommandReceiver::ExtractReceivedPacketAndNoticeInnerEvent()
{
	// 受信パケットを切り出している間、ループし、正常パケットをイベント通知
	// 自アドレスでない、BCCエラーについては、処理を継続
	for (TYPE_ERROR err = E_OK; (err == E_OK) || (err == E_NOT_MY_ADRS) || (err == E_BCC); ) 
	{
		// １パケット切り出す
		err = ExtractReceivedPacket();
		if (err == E_OK) 
		{
		    if(IsOver(QUE_MAX_VALUE)==true)
		    {
		        //イベントキューにQUE_MAX_VALUE以上格納されていれば、イベントを通知しない
		        DebugCommand::SendPrintSci("\r\n##que　MAX ##\r\n");
		        return;
		    }
		    if(_PosDataRcv>2)
		    {   //データ部が存在する場合のみ、イベントを通知する

                // データを切り出したら、リングバッファポインタ（データ部の先頭を渡す為＋２）を添えてパケット受信イベント通知
                SendEvent(EVT_CMDRCV_PACKET_RECEIVED, &_RcvDataQue[_PosRcvDataQue][2], _PosDataRcv - 2);

                // 受信パケットキューのカレントバッファ位置を１つ移動
                MovePosRcvDataQueByOne();
		    }
		}
	}
}


/*!------------------------------------------------
@brief 受信パケットキューのカレントバッファ位置を１つ移動
@note　
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::MovePosRcvDataQueByOne()
{
	_PosDataRcv = 0;	// 桁をクリア
	_PosRcvDataQue++;	// 行を次に
	if (_PosRcvDataQue >= RECEIVE_PACKET_RING_SIZE)
	{
		_PosRcvDataQue = 0;
	}

	// 受信パケット抽出ロジックで STX を受信した際に初期化する為、
	// ここでは、カレントバッファの初期化は行わない
}

/*!------------------------------------------------
@brief 受信パケット抽出リセット
@note　
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::ResetReceivedPacket()
{
	// カレントバッファの次に受信データをセットする位置（現在セット済データ長）をクリア
	_PosDataRcv = 0;

	// パケット解析ロジックの状態管理 を初期化
	_StPktRcv = ST_PKTRCV_WAIT_STX;

	// 受信パケット抽出ロジックで STX を受信した際に初期化する為、
	// ここでは、カレントバッファの初期化は行わない
}


/*!------------------------------------------------
@brief 受信パケットを抽出する
@note　受信データをデータ部（STX, ETX, BCCを除く）のみ抽出しバッファ _RcvDataQue[_PosRcvDataQue] にセットする
	   セットするバッファの情報
			ENUM_ST_PKTRCV _StPktRcv;   パケット解析ロジックの状態管理
			uint8_t _RcvDataQue[RECEIVE_PACKET_RING_SIZE][RECEIVE_PACKET_BUFFER_SIZE]; パケット受信データキュー
			uint32_t _PosRcvDataQue;    現在使用中のパケット受信データキュー位置（関数内では ReadOnly）
			uint32_t _PosDataRcv;       パケット受信データに次にセットする位置（０オリジン）、言い換えると現在セット済のデータ長

　		STX受信後、正常終了（正常なパケットがバッファにセットされた）の時は E_OK で関数を戻す
		STX受信後、各種エラーで終了したら 対応するエラー（E_BCC、E_NOT_MY_ADRS）で関数を戻す
		STXを受信していない、又は、パケット受信途中 、又は、STXを受信していない場合 E_EMP で関数を戻す

@return E_OK / E_BCC / E_NOT_MY_ADRS / E_EMP
@author
-------------------------------------------------*/
TYPE_ERROR CommandReceiver::ExtractReceivedPacket()
{
	const uint8_t ASCII_STX = 0x02;
	const uint8_t ASCII_ETX = 0x03;
	uint16_t tempRcvBuf = 0;		//受信データを一時的に2byteで保持する
	uint8_t rcvBuf = 0;				//受信データを1byteで保持する

	//受信前にUARTのエラー検出
	UartCheckAndReset();

	while ((tempRcvBuf =GetReceiveByte()) != 0xffff)//シリアルバッファが空になるまでループ、0xffffはデータが空を表す
	{
        if(_StPktRcv!=ST_PKTRCV_WAIT_STX)
        {   // STX受信からパケット受信タイムアウト時間を超えた
            if(IsTimerIsExpired(_RcvStartTime, MS_TMOUT_RCV_PKT))
            {
                _StPktRcv = ST_PKTRCV_WAIT_STX;
            }
        }

	    rcvBuf = (uint8_t)tempRcvBuf;		//上位8bitは必要ない
		//状態（_StPktRcv）に基づくswitch文
		switch (_StPktRcv)
		{
		case ST_PKTRCV_WAIT_STX:		//STX受信待ち状態
			if (rcvBuf == ASCII_STX)	//STXを受信した場合
			{
				//	STX受信待ちからETX受信待ちに状態遷移
				//	受信データリングバッファを初期化する
				_StPktRcv = ST_PKTRCV_WAIT_ETX;
				memset(_RcvDataQue[_PosRcvDataQue], 0, sizeof(RECEIVE_PACKET_BUFFER_SIZE));//パケット受信データキューを0で初期化する
				_PosDataRcv = 0;//パケット受信データ位置を0で初期化する
			    _RcvStartTime = GetTimerTick(); // パケット受信開始時刻を取得する
			}
			break;
		case ST_PKTRCV_WAIT_ETX:		//ETX受信待ち状態
			if (rcvBuf == ASCII_ETX)	//ETXを受信した場合
			{
				//	BCCチェック中に状態遷移
				_StPktRcv = ST_PKTRCV_WAIT_BCC;
				break;				
			}
			//	受信データリングバッファに格納
			//	桁数をインクリメント
			_RcvDataQue[_PosRcvDataQue][_PosDataRcv] = rcvBuf;
			_PosDataRcv++;
			break;
		case ST_PKTRCV_WAIT_BCC:	//BCCチェック中状態
			//BCCチェック開始
			uint8_t bccBuf = 0;
			for (uint16_t index = 0;index < _PosDataRcv;index++)	//index0からデータ最後までXOR演算する
			{
				bccBuf = bccBuf ^ _RcvDataQue[_PosRcvDataQue][index];
			}
			bccBuf = bccBuf ^ ASCII_ETX;	// bccBufとETXをXOR演算する
			if (bccBuf == rcvBuf)		//計算結果と受信したBCC比較する
			{
				//	BCCチェックOK
				uint32_t rcvAdres = 0;
                ClsCommon::ATOI((const char*)&_RcvDataQue[_PosRcvDataQue][0], 2, &rcvAdres);//アドレスをアスキーから数値に変換する
				if (G_pDataManager->GetReaderAddress() == rcvAdres || memcmp(&_RcvDataQue[_PosRcvDataQue][0],"FF",2) == 0)	//自アドレスチェック、FFはブロードキャストのこと
				{
					//	自アドレスチェックOK
					_StPktRcv = ST_PKTRCV_WAIT_STX;        //STX受信待ちに状態遷移
					return E_OK;
				}
				else
				{
					//	自アドレスチェックエラー
					_StPktRcv = ST_PKTRCV_WAIT_STX;        //STX受信待ちに状態遷移
					return E_NOT_MY_ADRS;
				}
			}
			else
			{
				//	BCCチェックエラー
				_StPktRcv = ST_PKTRCV_WAIT_STX;			   //STX受信待ちに状態遷移
				return E_BCC;
			}
		}
	}
	return E_EMP;
}



/*!------------------------------------------------
@brief シリアルポートから１文字受信データを読み込む
@note　Windows、RAマイコン実行時で処理を変更すること
@return 正常データ 0x00～0xff、受信データ無し 0xffff
@author
-------------------------------------------------*/
uint16_t CommandReceiver::GetReceiveByte()
{
#ifdef WINDOWS_DEBUG
	//Windows動作時
	return WinSerialPort::ReadByte(COM_PORT0_RS485);
#else
	//マイコン動作時
	// ★ 上位システム対応用
	uint8_t data = 0;  //読取データ用ポインタ
	uint32_t result = G_pDrvUART_RS485->Read(&data,1);
	if(result == 0)
	{
	    //データが無い場合は0xffff
	    return 0xffff;
	}
	else
	{
	    //データを値で返す
	    return data;
	}
#endif
}

/*!------------------------------------------------
@brief Uartエラー検出、累積回数の更新
@note  エラーを検出したらリセット(Close -> Openする)
@return なし
@author
-------------------------------------------------*/
void CommandReceiver::UartCheckAndReset()
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
#else
    UART *pUART = G_pDrvUART_RS485;
    EVENT_COUNT evt = (EVENT_COUNT)0;
    ERRORFLAG flg = (ERRORFLAG)0;

#ifdef RS485_ERROR_DEBUG_HOST_OVER
    switch(UART_ERR_OVERFLOW)
#elif defined(RS485_ERROR_DEBUG_HOST_FERR)
    switch(UART_ERR_FRAMING)
#else
    switch(pUART->GetError())
#endif /* RS485_ERROR_DEBUG_HOST_OVER / RS485_ERROR_DEBUG_HOST_FERR */
    {
        case UART_ERR_NONE:
            return; //エラーなし、これ以上処理しない
            /* returnのためbreakなし */
        case UART_ERR_FRAMING:
            //フレーミングエラー
            flg = ERRORFLAG_HOST_FERR;
            evt = EVENT_ERROR_HOST_PRI_FER;
            break;
        case UART_ERR_OVERFLOW:
            //オーバーフロー
            flg = ERRORFLAG_HOST_OVER;
            evt = EVENT_ERROR_HOST_PRI_OVER;
            break;
        case UART_ERR_PARITY:
        case UART_BREAK_DETECT:
        default:
            /*　記録なし */
            break;
    }

    //記録対象の場合
    if( flg )
    {
        //エラー記録
        G_pDataManager->SetErrorFlag(ERRORFLAG_HOST_PRI_OFFSET, flg);
        //回数記録
        G_pDataManager->AddEventCount(evt);
    }

    //UARTリセット
    if(E_OK != pUART->Reset())
    {
        //復旧不可
        THROW_FATAL_ERROR("CommandReceiver", "Uart Reset Error");
    }
#endif
}
