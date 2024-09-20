/*!------------------------------------------------
@brief ファームウエア更新タスク
@note
@author
-------------------------------------------------*/
#include "Task.h"

FwUpdater::FwUpdater()
	:BaseTask(ST_FW_MAX, EVT_FW_MAX, TIMER_FW_MAX, FW_EVENT_QUE_SIZE, _pStateString, _pEventString)
{
	//タイマー登録
	this->RegisterTimer(TIMER_ONESHOT, EVT_FW_RECEIVE_TMOUT);
	
	//停止状態へ各ハンドラ登録
	State* st = this->GetState(ST_FW_DORMANT);
	st->AddEventHandler(EVT_FW_DLL_START, Dormant_DllStart);	//!< ダウンロード開始

	//ダウロード中状態へ各ハンドラ登録
	st = this->GetState(ST_FW_DOWN_LOADING);
	st->AddEnterHandler(DownLoading_Enter);									//!< Enter
	st->AddEventHandler(EVT_FW_DLL_DATA, DownLoading_DllData);				//!< データパケット受信
	st->AddEventHandler(EVT_FW_RECEIVE_TMOUT, DownLoading_ReceiveTmout);	//!< 受信タイムアウト
	st->AddExitHandler(DownLoading_Exit);                                 //!< Exit

	//正常再起動状態へ各ハンドラ登録
    st = this->GetState(ST_FW_NORMAL_REBOOT);
	st->AddEnterHandler(NormalReboot_Enter);	//!< Enter

	//異常再起動状態へ各ハンドラ登録
	st = this->GetState(ST_FW_ABNORMAL_REBOOT);
	st->AddEnterHandler(AbnormalReboot_Enter);	//!< Enter
}

/*!------------------------------------------------
@brief 停止状態 － 開始イベント処理
@note　 ダウロード中に遷移
       累積回数のダウンロード開始回数を＋１
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData :
@param DataLen : 
@return なし
@author
-------------------------------------------------*/
void FwUpdater::Dormant_DllStart(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
	(void)EventId;(void)pData;(void)DataLen; // 未使用引数のワーニング対策

	//累積回数：ダウンロード開始回数を＋１
	G_pDataManager->AddEventCount(EVENT_DOWNLOAD_ERASE);

	//強制的にデータFlashを更新する
	G_pDataManager->DataFlashUpdateForce();

	//ダウンロード中に遷移
	pMyTask->SetNextStateId(ST_FW_DOWN_LOADING);
}

/*!------------------------------------------------
@brief ダウンロード中状態 － Enter処理
@note　他タスクを停止する
      Flash Lpをオープンし、領域2をblankにする
      QrReader、NfcReader、Datamanagerを停止
      OutputControllerはLEDをFW更新時表示に変更
	  受信タイムアウトタイマースタート
	  変数の初期化を行う
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void FwUpdater::DownLoading_Enter(BaseTask* pOwnTask)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
	TYPE_ERROR err =E_OK;
	//コードフラッシュの初期化
	err = pMyTask->InitCodeFlash();
    if(err != E_OK)
    {
        //初期化失敗
        pMyTask->SetNextStateId(ST_FW_ABNORMAL_REBOOT);
        return;
    }
    //コードフラッシュを空にする
    err = pMyTask->EraseCodeFlash();
    if(err != E_OK)
    {
        //削除失敗
        pMyTask->SetNextStateId(ST_FW_ABNORMAL_REBOOT);
        return;
    }
	//QrRedaerタスク停止処理
    G_pQrReader->SetForcedStop();
		
	//NfcRedaerタスク停止処理
    G_pNfcReader->SetForcedStop();
		
	//OutputControllerFW更新時のLED表示に変更指示
    G_pOutputController->SendEvent(EVT_OUTPUT_FW_UPDATING);

	//DataManagerタスク停止処理
    G_pDataManager->SetForcedStop();

	//前回のブロックナンバーを初期化
	pMyTask->_previousBlockNum = 0;
	//受信タイムアウトタイマースタート(1s)
	pMyTask->StartTimer(EVT_FW_RECEIVE_TMOUT, pMyTask->_tmoutValue3s);
}

/*!------------------------------------------------
@brief ダウンロード中状態 － データパケット受信
@note  データからブロックナンバーを取り出し、それでデータの漏れを検出する
       データに問題が無ければ、領域２の先頭から順に、書き込んでいく
       書込み先アドレスはブロックナンバー×128でオフセットしていく
       正常に全データ(120KB)書き込めたら、正常再起動に遷移する
       何か異常があれば、異常再起動に遷移する
       タイムアウトは1秒
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 受信パケットのデータ部
@param DataLen : データ部の長さ
@return なし
@author
-------------------------------------------------*/
void FwUpdater::DownLoading_DllData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
	(void)EventId;(void)pData;(void)DataLen; // 未使用引数のワーニング対策
	TYPE_ERROR err = E_OK;
	const uint8_t* pdata = pData;//データのポインタ
	uint32_t adrsBuf = pMyTask->_adrs_Region2_Begin;  //アドレスバッファ(領域2の先頭アドレスを入れておく)
    uint8_t dataBuf[128]={0};//値に変換したデータを格納するバッファ
	uint32_t blockNumBuf = 0;//ブロックナンバー用バッファ
	//受信タイムアウトタイマーストップ
	pMyTask->StopTimer(EVT_FW_RECEIVE_TMOUT);
	//受信パケットのブロックナンバーまでアドレスを進める
	pdata += 3;
	//ブロックナンバーを数値に変換し、blockNumBufに代入
	ClsCommon::AsciiHex2Uint32((const char*)pdata, 4, (uint32_t*)&blockNumBuf);
	if (pMyTask->_previousBlockNum != blockNumBuf)//ブロックナンバーが異常なら、異常再起動に遷移
	{
        //異常：ブロック飛ばし
        pMyTask->SetNextStateId(ST_FW_ABNORMAL_REBOOT);
        return;
	}

    if (pMyTask->_previousBlockNum < pMyTask->getmaxBlockNum)
    {
        //ブロックナンバーが960未満の時
        //受信パケットのデータまでアドレスを進める
        pdata += 4;
        //Hex文字列をバイナリに変換
        ClsCommon::AsciiHex2Bin((const char*)pdata,256,(uint8_t*)&dataBuf,128);
        //CodeFlashのアドレス（書込み先）をオフセット
        adrsBuf+=blockNumBuf*128;

        //CodeFlashに書き込む
        err = pMyTask->WriteCodeFlash(dataBuf, 128, adrsBuf);
        if(err != E_OK)
        {
            //書き込みエラー
            pMyTask->SetNextStateId(ST_FW_ABNORMAL_REBOOT);
            return;
        }

        //前回のブロックナンバー変数をインクリメント
        pMyTask->_previousBlockNum++;
        //受信タイムアウトタイマースタート(1s)、次のデータを待つ
        pMyTask->StartTimer(EVT_FW_RECEIVE_TMOUT, pMyTask->_tmoutValue1s);
    }
    else
    {
        //ブロックナンバーが960以上の時
        //前回のブロックナンバー変数をインクリメント
        pMyTask->_previousBlockNum++;
        //最後のブロックナンバ(1024個目)だった場合は、正常再起動
        if(pMyTask->_previousBlockNum == pMyTask->maxBlockNum)
        {
            //正常再起動に遷移
            pMyTask->SetNextStateId(ST_FW_NORMAL_REBOOT);
        }
    }
}


/*!------------------------------------------------
@brief ダウロード中状態 － 受信タイムアウト処理
@note　 異常再起動に遷移する
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void FwUpdater::DownLoading_ReceiveTmout(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask; // 未使用引数のワーニング対策
	(void)EventId;(void)pData;(void)DataLen;
	//受信タイムアウト発生
	//異常再起動に遷移
	pMyTask->SetNextStateId(ST_FW_ABNORMAL_REBOOT);
}

/*!------------------------------------------------
@brief ダウンロード中状態 － Exit処理
@note　CodeFlashを閉じる
      データマネージャーを再開する
@param pOwnTask 自タスクオブジェクトのポインタ
@return なし
@author
-------------------------------------------------*/
void FwUpdater::DownLoading_Exit(BaseTask* pOwnTask)
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
#else
    FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
    (void)pMyTask; // 未使用引数のワーニング対策

    //flash lpを閉じる
    fsp_err_t err = R_FLASH_LP_Close(g_flash0.p_ctrl);
    if(err != FSP_SUCCESS)
    {
        //失敗
        //どちらにせよ再起動するので、何もしない
    }
    //debug:リブート前に最終ブロックナンバー送信
    //ClsCommon::PrintInfoNum(DebugCommand::SendPrintSci, " - reboot: ", pMyTask->_previousBlockNum);
#endif
}

/*!------------------------------------------------
@brief 正常再起動状態 － Enter処理
@note　　ダウンロード更新回数＋１
       再起動する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void FwUpdater::NormalReboot_Enter(BaseTask* pOwnTask)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
	//累積回数：ダウンロード更新回数を＋１
	G_pDataManager->AddEventCount(EVENT_DOWNLOAD_UPDATE);
	//再起動実行
	pMyTask->Reboot();
}

/*!------------------------------------------------
@brief 異常再起動状態 － Enter処理
@note　　他タスクを停止する
　　　　　　再起動する
@param pOwnTask 自タスクオブジェクトのポインタ
@param EventId イベントID
@param pData : 使用しない
@param DataLen : 使用しない
@return なし
@author
-------------------------------------------------*/
void FwUpdater::AbnormalReboot_Enter(BaseTask* pOwnTask)
{
	FwUpdater* pMyTask = (FwUpdater*)pOwnTask;
	//再起動実行
	pMyTask->Reboot();
}

/*!------------------------------------------------
@brief CodeFlash書込み関数
@note
@param pBuf : 書き込むデータバッファのポインタ
@param len : データの長さ
@param headAddress : 書込み先の先頭アドレス
@return なし
@author
-------------------------------------------------*/
TYPE_ERROR FwUpdater::WriteCodeFlash(const uint8_t* pBuf, const uint32_t len, uint32_t headAddress)
{
#ifdef WINDOWS_DEBUG
	//Windows動作時
	return E_OK;
#else
	const uint32_t writeBuf = (uint32_t)pBuf;

	//マイコン動作時
    fsp_err_t err = FSP_SUCCESS;
    //割り込み禁止
    DEFINE_INT();
    DISABLE_INT();
	//書込み
    err = R_FLASH_LP_Write(g_flash0.p_ctrl,writeBuf, headAddress, len);
    if (FSP_SUCCESS != err)
    {
        //割り込み許可
        ENABLE_INT();
        return E_ELSE;
    }
    //割り込み許可
    ENABLE_INT();
    return E_OK;
#endif
}

/*!------------------------------------------------
@brief 再起動実行関数
@note  再起動を実行する
@return なし
@author
-------------------------------------------------*/
void FwUpdater::Reboot()
{
#ifdef WINDOWS_DEBUG
	//Windows動作時

#else
	//マイコン動作時
    //再起動実行
    G_pDataManager->SystemSaveAndReboot();

#endif
}

/*!------------------------------------------------
@brief コードフラッシュの初期化
@note  Flash Lpをオープンする
@return なし
@author
-------------------------------------------------*/
TYPE_ERROR FwUpdater::InitCodeFlash()
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
    return E_OK;
#else

    fsp_err_t err = R_FLASH_LP_Open(g_flash0.p_ctrl,g_flash0.p_cfg);
    if(err!=FSP_SUCCESS)
    {
        //オープンできなければ
        return E_ELSE;
    }
    return E_OK;
#endif
}

/*!------------------------------------------------
@brief コードフラッシュを削除
@note  1ブロック(2KB)単位でブランクチェックを行い、ブランクでなければ
       削除する。合計60ブロック(120KB)行う
       先頭ブロックは0x00021800
@return なし
@author
-------------------------------------------------*/
TYPE_ERROR FwUpdater::EraseCodeFlash()
{
#ifdef WINDOWS_DEBUG
    //Windows動作時
    return E_OK;
#else
    flash_result_t blankCheck = FLASH_RESULT_BLANK;//ブランクチェックの結果
    fsp_err_t err = FSP_SUCCESS;
    uint32_t adrsBuf = 0;//アドレスバッファー
    for(uint8_t blankCheckCnt = 0;blankCheckCnt < _code_Flash_Block_Total;blankCheckCnt++)//120KBのブランクチェックはWDTが間に合わない為、１ブロックずつ
    {
        adrsBuf = _adrs_Region2_Begin + (_code_Flash_Block_Size * blankCheckCnt);//ブラックチェックをする先頭アドレス
        //WDTをクリア
        G_pDrvWDT->Refresh();
        //割り込み禁止
        DEFINE_INT();
        DISABLE_INT();
        //ブランクチェック開始
        err = R_FLASH_LP_BlankCheck(g_flash0.p_ctrl,adrsBuf,_code_Flash_Block_Size,&blankCheck);
        //割り込み許可
        ENABLE_INT();
        if (FSP_SUCCESS != err)
        {
            //ブランクチェック失敗
            return E_ELSE;
        }
        else
        {
            if (FLASH_RESULT_BLANK == blankCheck)
            {
                //ブランクである
                //何もしない
            }
            else if (FLASH_RESULT_NOT_BLANK == blankCheck)
            {
                //ブランクでないなら、Erase処理
                //割り込み禁止
                DEFINE_INT();
                DISABLE_INT();
                err = R_FLASH_LP_Erase(g_flash0.p_ctrl,adrsBuf,1);//１ブロックを削除
                //割り込み許可
                ENABLE_INT();
                if (FSP_SUCCESS != err)
                {
                    //削除失敗
                    return E_ELSE;
                }
                else
                {
                    //削除成功
                    //何もしない
                }
            }
            else
            {
                //何もしない(BGO関連)
            }
        }
    }
    return E_OK;
#endif
}
