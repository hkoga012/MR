/*!------------------------------------------------
@brief イベント・応答・送信タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef EVENT_SENDER_H
#define EVENT_SENDER_H
#include "../StateTransition/BaseTask.h"
//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_EVTSND_DORMANT = 0,		//!< 停止（必須）
    ST_EVTSND_WAIT_ORDER,       //!< 送信指示待ち
    ST_EVTSND_SENDING,			//!< 送信中（正常送信、衝突検知の判定待ち）
    ST_EVTSND_WAIT_RESEND,  	//!< 再送待ち（リーダアドレスに応じて待ち時間が変わる）
    ST_EVTSND_MAX,				//!< イベント種別の最大数(削除禁止)
}ENUM_STATE_EVTSND;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t EVTSND_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_EVTSND_IDLE = 0,        //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント
    EVT_EVTSND_START,           //!< 開始（起動完了を受けて、リセットデータ送信）

//《タスク外から通知されるイベント》
    EVT_EVTSND_REQ_READ_DATA,       //!< 読み取りデータ送信指示 ： DataLen=33, pDataの中身 上位通知する読取データの 識別用コード1byte 読取データ16byte×2
    EVT_EVTSND_REQ_RESET_DATA_INIT,  //!< リセットデータ送信指示
    EVT_EVTSND_REQ_RESET_DATA_STATUS,  //!< リセットデータ送信指示
    EVT_EVTSND_REQ_MEMORY_DATA,         //!< メモリデータ送信指示
    EVT_EVTSND_REQ_VER_DATA,            //!< バージョン・チェックサムデータ送信指示
    EVT_EVTSND_REQ_VER_INFO_DATA,    //!< バージョン情報送信指示

//《タスク内部イベント》
    EVT_EVTSND_REQ_RESET_DATA_POWER,  //!< リセットデータ送信指示
    EVT_EVTSND_TMOUT_RESEND_INTERVAL,    //!< 再送インターバル経過通知
    EVT_EVTSND_REQ_RESET_DATA_POWER_TIMER,    //!< 電源投入時のリセットデータ送信遅延タイマー
    EVT_EVTSND_MAX,             //!< イベント送信・応答タスク イベント種別最大数(削除禁止)
}ENUM_EVT_EVTSND;

constexpr uint32_t TIMER_EVTSND_MAX = 2;	//!< タイマー登録数
constexpr uint8_t SENDED_PACKET_BUFFER_SIZE = 255;
//《送信するパケットの長さ》
constexpr uint32_t DT_LEN_REQ_READ = 37;                //!< 読取データ データ部長さ
constexpr uint32_t DT_LEN_REQ_RESET = 15;               //!< リセットデータ データ部長さ
constexpr uint32_t DT_LEN_REQ_MEMORY = 36;              //!< メモリデータ データ部長さ
constexpr uint32_t DT_LEN_REQ_VER = 12;                 //!< バージョン・チェックサム データ部長さ
constexpr uint32_t DT_LEN_REQ_VER_INFO = 18;            //!< バージョン情報 データ部長さ
/*!------------------------------------------------
@brief イベント・応答・送信タスククラス
@note
@author
-------------------------------------------------*/
class EventSender : public BaseTask
{
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_EVTSND_MAX] = {
        "DORMANT",	        //!< 停止
        "WAIT_ORDER",	    //!< 送信指示待ち
        "SENDING",	        //!< 送信中（正常送信、衝突検知の判定待ち）
        "WAIT_RESEND",	    //!< 再送待ち（リーダアドレスに応じて待ち時間が変わる）
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_EVTSND_MAX] = {
        "IDLE",			            //!< イベント無し(必須)
        "START",			        //!< 開始
        "REQ_READ_DATA",            //!< 読み取りデータ送信指示
        "REQ_RESET_DATA_INIT",      //!< リセットデータ送信指示
        "REQ_RESET_DATA_STATUS",    //!< リセットデータ送信指示
        "REQ_MEMORY_DATA",          //!< メモリデータ送信指示
        "REQ_VER_DATA",             //!< バージョン・チェックサムデータ送信指示
        "REQ_VER_INFO_DATA",        //!< バージョン情報送信指示
        "REQ_RESET_DATA_POWER",     //!< リセットデータ送信指示
        "TMOUT_RESEND_INTERVAL",	//!< 再送インターバル経過通知
        "REQ_RESET_DATA_POWER_TIMER",     //!< 電源投入時のリセットデータ送信遅延タイマー
    };
public:
    //
    /*----------------------------
    // 状態遷移ハンドラ
    // ハンドラ名は、基本的に以下で定義する。
    // 　Eventハンドラ：「状態_イベント」
    // 　Enterハンドラ：「状態_Enter」
    // 　Exitハンドラ：「状態_Exit」
    // 複数の状態やイベントで同じハンドラを使う場合は、それが判るような名前を付ける。
    // イベントが発生しても処理しない場合は、登録しない。
    -----------------------------*/

    // 停止 状態
    static void Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);             //!< 開始


    // 送信指示待ち 状態
    static void WaitOrder_ReqReadData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);     //!< 読み取りデータ送信指示
    static void WaitOrder_ReqResetData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);    //!< リセットデータ送信指示
    static void WaitOrder_ReqMemoryData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);   //!< メモリデータ送信指示
    static void WaitOrder_ReqVerData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);      //!< バージョン・チェックサムデータ送信指示
    static void WaitOrder_ReqVerInfoData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);      //!< バージョン情報送信指示
    static void WaitOrder_ReqResetDataPowerTimer(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  //!< 電源投入時のリセットデータ送信

    // 送信中 状態
    static void Sending_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);          //!< Idle
    static void Sending_ReqReadData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);   //!< 読み取りデータ送信指示
    static void Sending_ReqResetData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  //!< リセットデータ送信指示
    static void Sending_ReqMemoryData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); //!< メモリデータ送信指示
    static void Sending_ReqVerData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);    //!< バージョン・チェックサムデータ送信指示
    static void Sending_ReqVerInfoData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);    //!< バージョン情報送信指示

    // 再送待ち 状態
    static void WaitResend_Enter(BaseTask* pOwnTask);    // Enter
    static void WaitResend_TmoutResendInterval(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  //!< タイマー通知
    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    EventSender();
    virtual ~EventSender() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "EventSender"; }

    /*----------------------------
    //外部公開関数（他タスクよりアクセス）
    -----------------------------*/
	void SetCollisionDetected() { _isCollisionDetected = 1; }
    uint32_t GetLatestSendLen();
    const uint8_t* GetLatestSendData() { return _SendedDataBuf; }  //送信済みデータバッファのポインタ


private:
    /*変数*/
    uint8_t _isCollisionDetected = 0;                       //!<衝突検知　衝突＝1　未衝突＝0
    uint8_t _isInitCompleted = 0;                            //!<初期化　完了＝1　未完了＝0
    uint8_t _SendCounter;                             //!<衝突による再送信のカウント変数
    uint8_t _SendedDataBuf[SENDED_PACKET_BUFFER_SIZE];  //!< 送信済みデータバッファ、データ部のみ格納する
    uint8_t _SendedDataBufLen = 0;                     //!< 送信済みデータバッファの長さ
    const uint32_t _timeOutVal = 51;                    //!< 送信タイムアウト時間(ms)
    /*関数*/
    void ClearCollisionDetected() { _isCollisionDetected = 0; }
    void SendReqReadData(const uint8_t* pData);       //!< 読取データ送信関数
    void SendReqResetData(const uint8_t Factor);      //!< リセットデータ送信関数
    void SendReqMemoryData(const uint8_t* pData);     //!< メモリデータ送信関数
    void SendReqVerData();                      //!< ver+チェックサム送信関数
    void SendReqVerInfoData();                      //!< バージョン情報送信関数
    void SendData(uint8_t DataLen);                            //!< STX,ETX付与,BCCを計算
    void SendPrintSci(const uint8_t* pData, uint8_t Len);             //!< シリアルポート出力関数
    void ResetSendedDataBuf(){ memset(_SendedDataBuf, 0, SENDED_PACKET_BUFFER_SIZE); }//送信済みデータバッファの初期化
    void SendedDataCheck();    //送信済みデータをチェックし、累積回数の更新/イベントを送信する
};

#endif
