/*!------------------------------------------------
@brief デバッグコマンドタスク
@note　コマンドを追加する際はファイルの一番下に追加すること
@author
-------------------------------------------------*/
#include "Task.h"
#include "..\version.h"

#ifdef WINDOWS_DEBUG
using namespace System;
using namespace System::Runtime::InteropServices;
#endif

/*----------------------------
//コンストラクタ
//BaseTaskインスタンス作成時に状態インスタンスを作成
-----------------------------*/
DebugCommand::DebugCommand()
	:BaseTask(ST_DEBUGCMD_MAX, EVT_DEBUGCMD_MAX, TIMER_DEBUGCMD_MAX, DEBUGCMD_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	State* st = NULL;

	_received_length = 0;
	_command_Index = 0;
	_isPreviousCommandSetted = OFF;

	//バッファ初期化
	memset(_commandStr, 0, DEBUG_COMMAND_BUFFER_SIZE);
	memset(_debug_command_buffer, 0, DEBUG_COMMAND_BUFFER_SIZE);

	_pTaskCmdParam = NULL;

	//初期化中状態　各ハンドラ登録
	st = this->GetState(ST_DEBUGCMD_INIT);
	st->AddEventHandler(EVT_DEBUGCMD_INIT_DISP, Init_InitDisp);	// 初期表示指示
	st->AddEnterHandler(Init_Enter);	// Enter

	//停止中状態　各ハンドラ登録
	st = this->GetState(ST_DEBUGCMD_STOP);
	st->AddEventHandler(EVT_DEBUGCMD_START, Stop_Start);	// 開始指示

	//動作中状態　各ハンドラ登録
	st = this->GetState(ST_DEBUGCMD_RUNNING);
	st->AddEventHandler(EVT_DEBUGCMD_STOP, Running_Stop);	// 停止指示
	st->AddEventHandler(EVT_DEBUGCMD_IDLE, Running_Idle);	// イベント無し
}

/*!------------------------------------------------
@brief UARTエラー検出リセット
@note　エラーを検出したらリセット(Close -> Openする)
        UARTのRead関数呼び出し前に呼び出すこと
@param なし
@author
-------------------------------------------------*/
void DebugCommand::UartCheckAndReset(void)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
#else
    UART *pUART = G_pDrvUART_BLE;

    if( UART_ERR_NONE == pUART->GetError() )
    {
        //エラーなし、これ以上処理しない
        return;
    }
    else
    {
        //UARTリセット
        if(E_OK != pUART->Reset())
        {
            //復旧不可
            THROW_FATAL_ERROR("DebugCommand", "Uart Reset Error");
        }
    }
#endif
}

/*!------------------------------------------------
@brief シリアルポート出力
@note　Windows、RAマイコン実行時で処理を変更すること
@param format 出力文字列
@return エラーコード
@author
-------------------------------------------------*/
void DebugCommand::SendPrintSci(const char* buf)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
	WinSerialPort::Write(COM_PORT1_BLE, buf);

#else
	//マイコン動作時
	G_pDrvUART_BLE->Write((uint8_t*)buf, (uint32_t)strlen(buf));
#endif
}


/*!------------------------------------------------
@brief シリアルポート出力 printfフォーマット対応機能追加
@note RAマイコン実行時のみ出力可能
      一度に出力できるサイズは255バイト以下とする
@param format 出力文字列
@param ... 可変引数
@return エラーコード
@author
-------------------------------------------------*/
//マイコン動作時のみ実装　Windows環境では、ヘッダにて空関数でコンパイル
#ifndef WINDOWS_DEBUG
void DebugCommand::SendPrintSciEx(const char* format, ...)
{

    #define TMP_BUFFER_SIZE 256
    char tmpBuffer[TMP_BUFFER_SIZE];

	va_list argptr;
    va_start(argptr, format);
    int32_t cnt = vsnprintf(tmpBuffer, TMP_BUFFER_SIZE, format, argptr);
    va_end(argptr);

	G_pDrvUART_BLE->Write((uint8_t*)tmpBuffer, (uint32_t)cnt);
}
#endif


/*!------------------------------------------------
@brief シリアルポート出力 "\t:"つき
@note
@param str 出力文字列
@param tabNum タイトル後のタブ数
@return エラーコード
@author
-------------------------------------------------*/
void DebugCommand::SendPrintSciTitle(const char* str, uint8_t tabNum)
{
	SendPrintSci(" ");
	SendPrintSci(str);
	for (uint8_t num = tabNum - (uint8_t)((strlen(str) + 1) / 8); num > 0; num--)
	{
		SendPrintSci("\t");
	}
	SendPrintSci(": ");
}

/*!------------------------------------------------
@brief シリアルポート受信データ読込
@note　Windows、RAマイコン実行時で処理を変更すること
@param str 受信データ格納バッファ先頭ポインタ
@param DataLen 受信データ読込可能データ長
@return エラーコード
@author
-------------------------------------------------*/
uint32_t DebugCommand::GetReceiveData(char* str, uint32_t DataLen)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
	return WinSerialPort::Read(COM_PORT1_BLE,str, DataLen);
#else
	//マイコン動作時
	return G_pDrvUART_BLE->Read((uint8_t*)str, DataLen);
#endif
}

/*!------------------------------------------------
@brief バックスペースの前のデータを削除
@note　
@param str 受信データ格納バッファ先頭ポインタ
@return なし
@author
-------------------------------------------------*/
void DebugCommand::DeleteBackspaceChar(char* str)
{
	size_t len = strlen((const char*)str);
	char* pOutStr = str;
	char* pStartStr = str;

	for (uint32_t i = 0;i < (uint32_t)len;i++)
	{
		*pOutStr = *str;
		if (ASCII_BACKSPACE == *str)
		{
			//バックスペースが見つかった場合はセットした文字を次文字で上書き
			if (pOutStr > pStartStr)
			{
				pOutStr--;
			}
		}
		else
		{
			pOutStr++;
		}
		str++;
	}
	*pOutStr = '\0';	//最後は安全のためNULLをセット
}

/*!------------------------------------------------
@brief パラメータ文字列を分解、パラメータ数とパラメータ文字列配列を返す
@note　
@param str コマンド文字列格納バッファ先頭ポインタ
@param argc コマンド引数の数を格納する変数ポインタ
@param argv コマンドパラメータを格納する配列先頭ポインタ
@return なし
@author
-------------------------------------------------*/
void DebugCommand::SplitArgument(char* str, int32_t* argc, const char* argv[])
{
	//コマンドの引数を作成
	for (*argc = 0; *argc < ARG_MAX; (*argc)++)
	{
		while (*str == ASCII_SPACE)
		{
			str++;		//スペース読み飛ばし
		}
		if (*str == '\0')
		{
			break;		//コマンドの最後
		}
		argv[*argc] = str;
		str = (char*)strchr((const char*)str, ASCII_SPACE);
		if (str == NULL)
		{
			break;
		}
		*str++ = '\0';
	}
	(*argc)++;
}

/*!------------------------------------------------
@brief コマンド文字列に一致するコマンドを処理する
@note　
@param pCmdBuffer コマンド文字列 (NULL は 再実行）
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::SeekAndExecuteCommand(char* pCmdBuffer)
{
	// 再実行出来るのは、実行済のコマンド文字列が変更されていないことが前提
	static int32_t argc = 0;
	static const char* argv[ARG_MAX + 1];	// コマンド文字列上の (const char *) ポインタが格納される配列

	if (pCmdBuffer != NULL)
	{
		size_t strLen = strlen((const char*)pCmdBuffer);

		if (strLen > DEBUG_COMMAND_BUFFER_SIZE) {
			return E_PAR;
		}
		
		// 初回なら、行末のスペース、\r,\nを削除
		char* p = pCmdBuffer + strLen - 1;
		while ((*p == ASCII_SPACE || *p == ASCII_CR || *p == ASCII_LF))
		{
			*p = '\0';
			p--;
			if (strLen > 0)
			{
				strLen--;
			}
			else
			{
				break;
			}
		}

		if (0 == strLen)
		{
			return E_PAR;
		}

		// 初回なら コマンド解析（バッファのスペースを\0に置き換え）
		SplitArgument(pCmdBuffer, &argc, argv);
	}

	for (uint32_t i = 0; _pCommandList[i].pStrCommand != NULL; i++) 
	{
		if (!strcmp((const char*)argv[0], (const char*)_pCommandList[i].pStrCommand))
		{
			//コマンド処理(this->が必要な理由はインスタンス生成後の変数の参照が必要なため)
			return (this->*_pCommandList[i].pFunc)(argc, argv);
		}
	}
	return E_ELSE;
}

/*!------------------------------------------------
@brief コマンド処理
@note　
@param　なし
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::DoCommand(void)
{
	uint32_t prev_recv_length;
	TYPE_ERROR err=E_OK;

	if (_received_length >= DEBUG_COMMAND_BUFFER_SIZE)
	{
		SendPrintSci("\r\nToo long command\r\nNG\r\n");		/* バッファ満	*/
		_received_length = 0;
		//コマンドバッファ破棄
		_command_Index = 0;
		memset(_commandStr,0, DEBUG_COMMAND_BUFFER_SIZE);
	}
	UartCheckAndReset();
	prev_recv_length = _received_length;
	_received_length += GetReceiveData((char*)&(_debug_command_buffer[_received_length]), (uint32_t)(DEBUG_COMMAND_BUFFER_SIZE - _received_length));

	if (_received_length > 0 && _received_length > prev_recv_length)
	{

		//エコーバック処理
		_debug_command_buffer[_received_length] = '\0';
		SendPrintSci((char*)&_debug_command_buffer[prev_recv_length]);
			
		//コマンドバッファに格納
		for (uint32_t i = prev_recv_length; i < _received_length; i++)
		{
			if (DEBUG_COMMAND_BUFFER_SIZE > _command_Index)
			{
				if ((_isPreviousCommandSetted == ON) && (_debug_command_buffer[i] == ASCII_TAB))
				{	// 'TAB'キー押下で再実行
					SendPrintSci(pCrlf);
					err = SeekAndExecuteCommand(NULL);
					SendPrintSci((E_OK == err) ? pCmdResOK : pUnknownCmd);
				    _isPreviousCommandSetted = ((E_OK == err) ?  ON : OFF);
					if (_isPreviousCommandSetted == OFF) 
					{	// エラーならバッファクリア
						memset(_commandStr, 0, DEBUG_COMMAND_BUFFER_SIZE);
					}
				}
				else if(IsValidKey(_debug_command_buffer[i]))
				{
					if (_isPreviousCommandSetted == ON)
					{	// 次のコマンドが１文字でも入力されたら、直前に実行されたコマンドはクリア
						memset(_commandStr, 0, DEBUG_COMMAND_BUFFER_SIZE);
					}
					_isPreviousCommandSetted = OFF;

					// 入力文字列追加
					_commandStr[_command_Index] = _debug_command_buffer[i];
					_command_Index++;

					//コマンド終端検出
					if (_commandStr[_command_Index - 1] == ASCII_LF)
					{
						//バックスペース対応
						DeleteBackspaceChar(_commandStr);

						//入力文字列に該当するコマンドがあれば実行
						err = SeekAndExecuteCommand(_commandStr);
	                    SendPrintSci((E_OK == err) ? pCmdResOK : pUnknownCmd);
					    _isPreviousCommandSetted = ((E_OK == err) ?  ON : OFF);
						if (_isPreviousCommandSetted == OFF) 
						{	// エラーならバッファクリア
							memset(_commandStr, 0, DEBUG_COMMAND_BUFFER_SIZE);
						}

						//コマンド格納位置クリア
						_command_Index = 0;
					}
				}

			}
			else 
			{
				//コマンドバッファ満
				SendPrintSci("\r\nToo long command\r\nNG\r\n");
				_command_Index = 0;
				memset(_commandStr, 0, DEBUG_COMMAND_BUFFER_SIZE);
			}
		}
		//受信分を全て処理、または_commandStrに格納したので初期化
		_received_length = 0;
	}
	return err;
}

/*!------------------------------------------------
@brief 初期処理中状態 － Enter処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void DebugCommand::Init_Enter(BaseTask* pOwnTask)
{
	DebugCommand* myTask = (DebugCommand *) pOwnTask;
	myTask->SendEvent(EVT_DEBUGCMD_INIT_DISP);
}

/*!------------------------------------------------
@brief 初期処理中状態 － 初期表示指示イベント処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DebugCommand::Init_InitDisp(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DebugCommand* myTask = (DebugCommand*)pOwnTask;

	myTask->SendPrintSci("\r\n");
	myTask->SendPrintSci(" ------------------------------------------------------\r\n");
	myTask->SendPrintSci(" MultiReader   ");
	myTask->SendPrintSci("Version:");
	myTask->SendPrintSci(myTask->VERSION_INFO);////////////////////
	myTask->SendPrintSci("\r\n");
	//myTask->SendPrintSci(" Copyright 2023 XXXXX rights reserved.\r\n");
	myTask->SendPrintSci(" ------------------------------------------------------\r\n");

	// 停止中状態に遷移し、動作開始イベント送信
	myTask->SendEvent(EVT_DEBUGCMD_START);
	myTask->SetNextStateId(ST_DEBUGCMD_STOP);
}

/*!------------------------------------------------
@brief 停止中状態 － 開始イベント処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DebugCommand::Stop_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DebugCommand* myTask = (DebugCommand*)pOwnTask;

	// 停止中状態に遷移し、動作開始イベント送信
	myTask->SetNextStateId(ST_DEBUGCMD_RUNNING);

	//タスククラスポインタテーブル作成
	myTask->_pTaskCmdParam = new TaskCmd[9];
	myTask->_pTaskCmdParam[0] = {"DebugCommand",     G_pDebugCommand};
	myTask->_pTaskCmdParam[1] = {"OutputController", G_pOutputController};
	myTask->_pTaskCmdParam[2] = {"NfcReader",        G_pNfcReader};
	myTask->_pTaskCmdParam[3] = {"QrReader",         G_pQrReader};
	myTask->_pTaskCmdParam[4] = {"DataManager",      G_pDataManager};
	myTask->_pTaskCmdParam[5] = {"FwUpdater",        G_pFwUpdater};
	myTask->_pTaskCmdParam[6] = {"CommandReceiver",  G_pCommandReceiver};
	myTask->_pTaskCmdParam[7] = {"EventSender",      G_pEventSender};
	myTask->_pTaskCmdParam[8] = {NULL,               NULL};    //終端(削除禁止)
}

/*!------------------------------------------------
@brief 動作中状態 － 停止イベント処理
@note　
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void DebugCommand::Running_Stop(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DebugCommand* myTask = (DebugCommand*)pOwnTask;

	myTask->SetNextStateId(ST_DEBUGCMD_STOP);
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
void DebugCommand::Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
    (void)EventId; (void)pData; (void)DataLen;  // 未使用引数のワーニング対策
	DebugCommand* myTask = (DebugCommand*)pOwnTask;

	//デバッグコマンド実行
	myTask->DoCommand();
}

//******************************************************************************
// 
// 以下に登録するコマンドを定義する
// 
//******************************************************************************

/*!------------------------------------------------
@brief ファームウェアバージョン表示
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdVersion(int32_t argc, const char* argv[])
{
    (void)argc; (void)argv; // 未使用引数のワーニング対策

	SendPrintSci("@Version:");
	SendPrintSci(VERSION_INFO);
	SendPrintSci("\r\n");
	return E_OK;
};

/*!------------------------------------------------
@brief タスク文字列から該当するタスククラスポインタ取得
@note 
@param pStrTask タスク文字列
@return タスククラスポインタ(該当なしはNULLを返す)
@author
-------------------------------------------------*/
BaseTask* DebugCommand::GetTaskPointer(const char *pStrTask)
{
	for (uint32_t i = 0; _pTaskCmdParam[i].pCmdString != NULL; i++)
	{
		if (0 == strcmp(_pTaskCmdParam[i].pCmdString, pStrTask))
		{
			return _pTaskCmdParam[i].pTask;
		}
	}
	return NULL;
};

/*!------------------------------------------------
@brief 指定タスクの状態を表示
@note 
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdPrintStatus(int32_t argc, const char* argv[])
{
	const uint32_t DUMP_LEN_MAX = 16;
	const uint32_t LOCAL_BUF_LEN = 20;

	char Data[LOCAL_BUF_LEN];
	memset(Data, 0, sizeof(Data));

	if (argc == 2)
	{	
		BaseTask* p = NULL;
		const char* pStrParamTaskName = argv[1]; //パラメータ文字列（タスク名）取得

		//指定パラメータと一致するタスクを検索
		p = GetTaskPointer(pStrParamTaskName);
		if (NULL != p)
		{
            SendPrintSci(pStrParamTaskName);
		    SendPrintSci(" Now Status = ");
            SendPrintSci(p->GetStateString(p->GetState()->GetId()));
			{
				uint32_t tick = GetTimerTick();
				uint32_t day = tick / (24 * 60 * 60 * 1000);
				uint32_t hour = ((tick % (24 * 60 * 60 * 1000)) / (60 * 60 * 1000));
				uint32_t min = ((tick % (60 * 60 * 1000)) / (60 * 1000));
				uint32_t sec = ((tick % (60 * 1000)) / 1000);

				SendPrintSci(", Now Time = ");
				DebugPrintNum(tick);
				SendPrintSci(" ( ");
				DebugPrintNum(day);
				SendPrintSci(" ");
				DebugPrintNum(hour);
				SendPrintSci(":");
				DebugPrintNum(min);
				SendPrintSci(":");
				DebugPrintNum(sec);
				SendPrintSci(" ) ");
			}
			SendPrintSci("\r\n");

			//指定されたタスクの状態、イベント履歴表示
			uint32_t cnt = p->GetTask()->GetLogCount();
			SendPrintSci("Time(msec), State, PrevState, Event, DataLen, pData\r\n");
            for (uint32_t i = cnt; i > 0; i--) // 最新の履歴から表示(cnt ... 1を表示、0をデータ無しと判断）
			{

                // #### 【注意】 大元のGet関数で渡されているのは 関数内のローカルバッファ。
                // #### ”function returns an aggregate” のメッセージが出ているが、安易にポインタに変更せず、現行ロジックのオブジェクトのコピーを渡すか、大元のGet関数を変えるべき
#ifndef WINDOWS_DEBUG
#pragma GCC diagnostic ignored "-Waggregate-return"
#endif
				Log log = p->GetTask()->GetLog(i-1); // 0オリジンなので -1
#ifndef WINDOWS_DEBUG
#pragma GCC diagnostic warning "-Waggregate-return"
#endif

				//時間を10桁のゼロサプレス文字に変換
				ClsCommon::ZeroSuppress(log.time, 10, Data, sizeof(Data));
				SendPrintSci(Data);

				SendPrintSci(", ");
				SendPrintSci(p->GetStateString(log.StateId));
				SendPrintSci(", ");
				SendPrintSci(p->GetStateString(log.PrevStateId));
				SendPrintSci(", ");
				SendPrintSci(p->GetEventString(log.EventId));
				SendPrintSci(", ");
				DebugPrintNum((int32_t)log.DataLen);
				SendPrintSci(", ");
				uint32_t len = ((uint32_t)(log.DataLen) < DUMP_LEN_MAX) ? (uint32_t)(log.DataLen) : DUMP_LEN_MAX;
				DebugPrintDataHex(log.pData, len);
				SendPrintSci("\r\n");
			}
		}
		else
		{
			//コマンド文字列不正
			SendPrintSci("palameter error.\r\n");
			return E_PAR;
		}
	}
	else 
	{
		//引数なしの場合はコマンド説明表示
		SendPrintSci("status [TaskClassName]\r\n");
		SendPrintSci("TaskClassName List\r\n");
		for (uint32_t i = 0;_pTaskCmdParam[i].pCmdString != NULL;i++)
		{
			SendPrintSci(" ");
			SendPrintSci(_pTaskCmdParam[i].pCmdString);
			SendPrintSci("\r\n");
		}
	}
	return E_OK;
};

/*!------------------------------------------------
@brief 指定タスクの情報を表示
@note 
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdPrintInfo(int32_t argc, const char* argv[])
{
	if (argc == 2)
	{
		BaseTask* p = NULL;
		const char* pStrParam = argv[argc - 1];	//パラメータ文字列取得

		//指定パラメータと一致するタスクを検索
		p = GetTaskPointer(pStrParam);
		if (NULL != p)
		{
			//指定されたタスクの情報を出力
			p->PrintInfo(SendPrintSci);
			SendPrintSci("\r\n");
		}
		else
		{
			//コマンド文字列不正
			SendPrintSci("palameter error.\r\n");
			return E_PAR;
		}
	}
	else
	{
		//引数なしの場合はコマンド説明表示
		SendPrintSci("info [TaskClassName]\r\n");
		SendPrintSci("TaskClassName List\r\n");
		for (uint32_t i = 0;_pTaskCmdParam[i].pCmdString != NULL;i++)
		{
			SendPrintSci(" ");
			SendPrintSci(_pTaskCmdParam[i].pCmdString);
			SendPrintSci("\r\n");
		}
	}
	return E_OK;
};

/*!------------------------------------------------
@brief 指定タスクへイベント送信
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdSendEvent(int32_t argc, const char* argv[])
{
	const uint32_t LOCAL_BUF_LEN = 20;

	TYPE_ERROR err = E_OK;

	if (argc >= 3)
	{
		BaseTask* p = NULL;
		const char* pParam1 = argv[1];	//タスク文字列
		const char* pParam2 = argv[2];	//イベントID
		const char* pParam3 = argc > 3 ? argv[3] : NULL;	//送信データ(指定されている場合のみ)
		static uint8_t Data[LOCAL_BUF_LEN];	// 変換後の送信データバッファ SendEvent関数でポインタ渡すのでstatic変数にする
		uint32_t EventId = 0;
		int32_t DataLen = 0;

		memset(Data,0,sizeof(Data));

		//イベントID取得
		ClsCommon::ATOI(pParam2, (uint32_t)strlen(pParam2), &EventId);
		
		if (NULL != pParam3)
		{
			//イベントパラメータ取得
			DataLen = ClsCommon::AsciiHex2Bin(pParam3, (uint32_t)strlen(pParam3), &Data[0], sizeof(Data));
			if (DataLen <= 0)
			{	// バッファ領域を超えてセットしても、ここに来る。
				SendPrintSci("Data error.\r\n");
				return E_PAR;
			}
		}
		//指定パラメータと一致するタスクを検索
		p = GetTaskPointer(pParam1);
		if (NULL != p)
		{
			if (EventId >= p->GetEventMax() || 0 == EventId)
			{
				SendPrintSci("EventId error.\r\n");
				return E_PAR;
			}

			//イベント送信
			p->SendEvent(EventId, &Data[0], DataLen);
		}
		else
		{
			//コマンド文字列不正
			SendPrintSci("TaskClassName error.\r\n");
			return E_PAR;
		}
	}
	else if (argc == 2)
	{
		//引数が2つでタスク名を指定された場合はイベントID一覧を表示
		BaseTask* p = NULL;
		const char* pParam1 = argv[1];	//タスク文字列
		//指定パラメータと一致するタスクを検索
		p = GetTaskPointer(pParam1);
		if (NULL != p)
		{
			SendPrintSci("Event List\r\n");
			SendPrintSci("EventId\t: EventName\r\n");
			for (uint32_t i = 1; i < p->GetEventMax();i++)	//i=1はイベントID=0を表示させないため
			{
				DebugPrintNum((int32_t)i);
				SendPrintSci("\t: ");
				SendPrintSci(p->GetEventString(i));
				SendPrintSci("\r\n");
			}
		}
		else
		{
			//コマンド文字列不正
			SendPrintSci("TaskClassName error.\r\n");
			return E_PAR;
		}
	}
	else
	{
		//引数なしの場合はコマンド説明表示
		SendPrintSci("event [TaskClassName] [EventId] [Data]\r\n");
		SendPrintSci("TaskClassName List\r\n");
		for (uint32_t i = 0;_pTaskCmdParam[i].pCmdString != NULL;i++)
		{
			SendPrintSci(" ");
			SendPrintSci(_pTaskCmdParam[i].pCmdString);
			SendPrintSci("\r\n");
		}
		SendPrintSci("EventId\r\n");
		SendPrintSci(" Event Id(decimal)\r\n");
		SendPrintSci("Data\r\n");
		SendPrintSci(" transmission data(2digit Hex)\r\n");
		SendPrintSci(" 0A3F\r\n");
	}
	return err;
}

/*!------------------------------------------------
@brief 使用可能なコマンドと説明を全表示
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdPrintHelp(int32_t argc, const char* argv[])
{
    (void)argc; (void)argv; // 未使用引数のワーニング対策

	SendPrintSci("Command\t\t: Description\r\n");
	for (uint32_t i = 0; i < DEBUGCMD_MAX; i++)
	{
		if (_pCommandList[i].pStrCommand != NULL)
		{
			SendPrintSciTitle(_pCommandList[i].pStrCommand, 2);
			SendPrintSci(_pCommandList[i].pStrHelp);
			SendPrintSci("\r\n");
		}
	}
	SendPrintSci("Press TAB to re-execute the previous command.\r\n");
	return E_OK;
}

/*!------------------------------------------------
@brief 各タスクの累積実行時間を一覧表示する
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdPrintTaskLoad(int32_t argc, const char* argv[])
{
	(void)argc; (void)argv; // 未使用引数のワーニング対策

	SendPrintSci("TaskClassName\t\t: ExecutionTime\r\n");
	for (uint32_t i = 0;_pTaskCmdParam[i].pCmdString != NULL;i++)
	{
		SendPrintSciTitle(_pTaskCmdParam[i].pCmdString, 3);
		const uint32_t timeSec = _pTaskCmdParam[i].pTask->GetExecuteTimeSec();

		if (timeSec > 1000000000)
		{
			SendPrintSci("(over)");
		}
		else
		{
			//時間を10桁のゼロサプレス文字に変換
			char buf[11];
			ClsCommon::ZeroSuppress(timeSec, 10, buf, sizeof(buf));
			SendPrintSci(buf);
		}

		SendPrintSci("\r\n");
	}
	return E_OK;
}

TYPE_ERROR DebugCommand::CmdDataFlashFunc(int32_t argc, const char* argv[])
{
    TYPE_ERROR err = E_PAR;

    if (argc >= 2)
    {
        const char* pFuncName = argv[1];

        if(strcmp(pFuncName, "format")==0)
        {   // データフラッシュ全体を強制的にフォーマットし、１レコードダミーを書く
            SendPrintSci("Format Data Flash and Add Dummy Data Start\r\n");
            G_pDataManager->DataFlashFormat();
            SendPrintSci("Format Data Flash and Add Dummy Data End\r\n");
            err = E_OK;
        }
        else if (strcmp(pFuncName, "erase") == 0)
        {   // データフラッシュ全体を評価用に消去したままの状態にする（基板部品実装直後の状態）
            SendPrintSci("Erase Data Flash Only Start\r\n");
            G_pDataManager->DataFlashErase();
            SendPrintSci("Erase Data Flash Only End\r\n");
            err = E_OK;
        }
        else if (strcmp(pFuncName, "read") == 0)
        {
            G_pDataManager->PrintInfoDataFlashBuffer(DebugCommand::SendPrintSci);
            err = E_OK;
        }
    }
    else
    {   //コマンド説明表示
        SendPrintSci("dflash [FuncName]\r\n");
        SendPrintSci("FuncName List\r\n");
        SendPrintSci(" format ... add 1Record\r\n");
        SendPrintSci(" erase  ... no format erase only\r\n");
		SendPrintSci(" read\r\n");
        err = E_OK;
	}

    return err;
}

TYPE_ERROR DebugCommand::CmdReboot(int32_t argc, const char* argv[])
{
    (void)argc; (void)argv; // 未使用引数のワーニング対策

	TYPE_ERROR err = E_OK;

	// データFlashに保存後、リブートさせる。
	G_pDataManager->SystemSaveAndReboot();

	return err;
}

/*** 要求事項に無いため、出荷時に装置には書かない

TYPE_ERROR DebugCommand::CmdSerialNoFunc(int32_t argc, const char* argv[])
{
    TYPE_ERROR err = E_OK;

    if (argc >= 2)
    {
        const char* pFuncName = argv[1];

        if(strcmp(pFuncName, "get")==0)
        {
            const char *pSNo = G_pDataManager->GetSerialNo();
            ClsCommon::PrintInfoLine(SendPrintSci, "Serial No: ", pSNo);
        }
        else if(strcmp(pFuncName, "set")==0)
        {
            if(argc >= 3)
            {
                const char* pSNo = argv[2];
                ClsCommon::PrintInfoLine(SendPrintSci, "Set Serial No: ", pSNo);
                uint8_t isOk = G_pDataManager->SetSerialNo(pSNo);
                err = isOk ? E_OK : E_ELSE;
            }
            else
            {
                SendPrintSci("Serial number not set\r\n");
                err = E_PAR;
            }
        }else{
            err = E_PAR;
        }
    }
    else
    {   //コマンド説明表示
        SendPrintSci("serno [FuncName]\r\n");
        SendPrintSci("FuncName List\r\n");
        SendPrintSci(" get\r\n");
        SendPrintSci(" set SerialNo(ex:P23120001)\r\n");
    }

    return err;
};
***/

#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
/*!------------------------------------------------
@brief アンテナ出力設定パラメータコマンド
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdSetAntParam(int32_t argc, const char* argv[])
{
    TYPE_ERROR err = E_OK;
    uint32_t Value = 0;

    if (argc == 3)
    {
        const char* pParam1 = argv[1];  //設定種別('v':電圧、'c':電流)
        const char* pParam2 = argv[2];  //設定値

        //設定値
        ClsCommon::ATOI(pParam2, (uint32_t)strlen(pParam2), &Value);

        switch(*pParam1)
        {
            case 'v':
            case 'V':
                if( Value > NFC_OUTPUT_VOLTAGE_MAX || Value == 0 )
                {
                    //コマンド文字列不正
                    SendPrintSciEx("Value error.[1-%d]\r\n",NFC_OUTPUT_VOLTAGE_MAX);
                    return E_PAR;
                }
                NFCSetOutputPowerVoltage( (uint8_t)Value );
                NFCParameterUpdate();   //パラメータ更新実行
                break;
            case 'c':
            case 'C':
                if( Value > NFC_OUTPUT_CURRENT_MAX || Value == 0 )
                {
                    //コマンド文字列不正
                    SendPrintSciEx("Value error.[1-%d]\r\n",NFC_OUTPUT_CURRENT_MAX);
                    return E_PAR;
                }
                NFCSetOutputPowerCurrent( (uint16_t)Value );
                NFCParameterUpdate();   //パラメータ更新実行
                break;
            default:
                //コマンド文字列不正
                SendPrintSci("Data type error.['v' or 'c']\r\n");
                return E_PAR;
        }
    }
    else if (argc == 2)
    {
        const char* pParam1 = argv[1];  //設定種別('v':電圧、'c':電流)
        switch(*pParam1)
        {
            case 'v':
            case 'V':
                Value = NFCGetOutputPowerVoltage();
                SendPrintSciEx("%d\r\n",Value);
                break;
            case 'c':
            case 'C':
                Value = NFCGetOutputPowerCurrent();
                SendPrintSciEx("%d\r\n",Value);
                break;
            default:
                //コマンド文字列不正
                SendPrintSci("Data type error.['v' or 'c']\r\n");
                return E_PAR;
        }
    }
    else
    {
        //引数なしの場合はコマンド説明表示
        SendPrintSci("ant [Type] [Value]\r\n");
        SendPrintSci("Type\r\n");
        SendPrintSci(" 'v':Voltage, 'c':Current\r\n");
        SendPrintSci("Value\r\n");
        SendPrintSciEx(" If Type='v' then input value range 1-%d\r\n",NFC_OUTPUT_VOLTAGE_MAX);
        SendPrintSci(" Voltage = 1.5V + (input value)/10\r\n");
        SendPrintSciEx(" If Type='c' then input value range 1-%d\r\n",NFC_OUTPUT_CURRENT_MAX);
        SendPrintSci(" Current = (input value)\r\n");
    }
    return err;
}
#endif

/*!------------------------------------------------
@brief デバッグ用の汎用コマンド
@note
@param argc コマンド引数の数
@param argv[] コマンド引数文字列の配列(引数は最大8)
@return エラーコード
@author
-------------------------------------------------*/
TYPE_ERROR DebugCommand::CmdDoDebug(int32_t argc, const char* argv[])
{
	(void)argc; (void)argv; // 未使用引数のワーニング対策

/*** デバッグ用コマンドの処理は無効に（コードFlash領域を空ける為）

    if (argc >= 2)
    {
        const char* pParam1 = argv[1];  //TestNo
        uint32_t testNo = 0;
        ClsCommon::ATOI(pParam1, (uint32_t)strlen(pParam1), &testNo);
        ClsCommon::PrintInfoNum(SendPrintSci, "CmdDoDebug() START TestNo=", testNo);

        switch(testNo)
        {
            case 1:
            SendPrintSci("ReadData Send Test\r\n");
            {   // 読み取りデータサンプル テスト
                static uint8_t buf[33];
                buf[0] = 0x30;
                for (uint32_t i = 0; i < 32; i++)
                {
                    buf[1 + i] = 'A' + (uint8_t)i;
                }

                G_pEventSender->SendEvent(EVT_EVTSND_REQ_READ_DATA, buf, sizeof(buf));
            }
            break;

            case 2:
            SendPrintSci("Call Fatal Error Func Test\r\n");
            {   // Fatal Error 関数を呼ぶ
                uint32_t line = GetTimerTick();
                G_pDataManager->SetSystemErrorInfoAndReboot("FileName\\forDebug\\Sample.cpp", line, "SampleTask", "Error Message Sample for Debug");
            }
            break;

            case 3:
            SendPrintSci("Fatal Error Occerred Test\r\n");
            {   //  システムエラーを強制的に発生させる評価 (DataManager にテストコード)
                THROW_FATAL_ERROR(this->GetTaskName(), "Test that forces an error.");
            }
            break;

            case 4:
            SendPrintSci("Many Event Send Test\r\n");
            {   // 大量のイベントを投げる
                for(uint32_t i=0; i<20; i++)
                {
                    G_pEventSender->SendEvent(EVT_EVTSND_REQ_RESET_DATA_INIT);
                }
            }
            break;

            case 5:
            {   // 〇〇テスト
            }
            break;

            case 6:
            {   // 〇〇テスト
            }
            break;

            case 7:
            {   // 〇〇テスト
            }
            break;
        }
    }

	SendPrintSci("CmdDoDebug() END\r\n");

***/

	return E_OK;
}

/*!------------------------------------------------
@brief  バージョン情報を返す
@note   FWのVersionとSvnのRevisionを文字列で返す。
        Revisionはソースコミット時に、Svnが自動でインクリメントする。
        バージョン情報の文字列(長さはNull有り：15,Null無し：14)
@return 文字列の先頭アドレス
@author
-------------------------------------------------*/
char* DebugCommand::GetVerInfo()
{
    const uint8_t RevDigit1StrLen = 11;//Revが１桁の時、文字の長さ(Nullを含まない)
    const uint8_t RevDigit4StrLen = 14;//Revが4桁の時、文字の長さ(Nullを含まない)
    uint8_t currentStrLen = 0;//現在の文字の長さ
    static char verInfoBuf[15];//バージョン情報バッファ

    //Versionを代入

    ClsCommon::I2HexAsciiByte(MAJOR_VERSION, (char*)&verInfoBuf[0], (char*)&verInfoBuf[1]);//上位２桁
    verInfoBuf[2]='.';//Versionの間に'.'を追加
    ClsCommon::I2HexAsciiByte(MINER_VERSION, (char*)&verInfoBuf[3], (char*)&verInfoBuf[4]);//下位２桁
    verInfoBuf[5]=' ';//VersionとRevisionの間にスペース

    //Revisionと結合
    strcat(verInfoBuf,"Rev:");
    if((uint8_t)strlen(SVN_REVISION)>4)
    {
        //Revisionの長さが異常の場合(Revは４桁まで対応)
        strcat(verInfoBuf,"@Err");
    }
    else
    {
        //Revisionの値が正常
        strcat(verInfoBuf,SVN_REVISION);
    }

    //現在の文字の長さ
    currentStrLen = (uint8_t)strlen(verInfoBuf);//Nullは長さに含めない

    //固定長にする為に、余ったバッファはスペースで埋める
    for(uint8_t lenBuf = RevDigit1StrLen;lenBuf < RevDigit4StrLen;lenBuf++)
    {
        if(currentStrLen == lenBuf)
        {
            for(;currentStrLen < RevDigit4StrLen;currentStrLen++)
            {
                verInfoBuf[currentStrLen]=' ';
            }
        }
    }
    verInfoBuf[currentStrLen] = '\0';//末尾にNull付加
    return verInfoBuf;
}


//==== 時間計測用　デバッグ関数 ====
void DebugCommand::SetDebugTime(int32_t pos)
{
#ifndef USE_PRINT_DEBUG_TIME
    (void)pos;
#else
    uint32_t tick = this->GetTimerTick();

    if(pos == 0)
    {
        for(uint32_t i=0; i<10; i++)
        {
            _debugTime[i] = tick;
        }
        _debugTimeNextNum = 1;  // 次の情報に
    }
    else if((pos < 10))
    {
        _debugTime[pos] = tick;
        _debugTimeNextNum = pos + 1;  // 次の情報に
    }
#endif
}
void DebugCommand::PrintDebugTime()
{
#ifdef USE_PRINT_DEBUG_TIME
    SendPrintSci("=== PrintDebug (ms) ===\r\n");
    SendPrintSci("start=");
    DebugPrintNum(_debugTime[0]);
    SendPrintSci("\r\n");

    for(int32_t i=1; i<_debugTimeNextNum; i++)
    {
        int32_t data = (_debugTime[i] - _debugTime[0]);
        if(data>=0 || i==0)
        {
            SendPrintSci("[");
            DebugPrintNum(i);
            SendPrintSci("] ");
            DebugPrintNum(data);
            SendPrintSci("\r\n");
        }
    }
    SendPrintSci("==========================\r\n");

    _debugTimeNextNum = 0;
#endif
}
