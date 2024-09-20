/*!------------------------------------------------
@brief コマンド受信タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef COMMAND_RECEIVER_H
#define COMMAND_RECEIVER_H
#include "../StateTransition/BaseTask.h"
//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_CMDRCV_DORMANT = 0,		//!< 停止（必須）
    ST_CMDRCV_RECEIVING,		//!< 通常受信中
    ST_CMDRCV_WAITING_INST,	    //!< 指示コマンド待ち
    ST_CMDRCV_DO_REBOOT,	    //!< 再起動実行
    ST_CMDRCV_MAX,				//!< イベント種別の最大数(削除禁止)
}ENUM_STATE_CMDRCV;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
// *****************************************
//!< イベント定義
constexpr uint32_t CMDRCV_EVENT_QUE_SIZE = 20; //!< イベントバッファサイズ
typedef enum {
    EVT_CMDRCV_IDLE = 0,        //!< Idle(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント

//《タスク外から通知されるイベント》
    EVT_CMDRCV_START,			//!< 開始
    EVT_CMDRCV_RD_DATA_SENDED,  //!< 読取データ送信完

//《タスク内部イベント》
    EVT_CMDRCV_PACKET_RECEIVED, //!< パケットを受信した
    EVT_CMDRCV_TMOUT_RCV_INST,	//!< 指示コマンド受信タイムアウト

    EVT_CMDRCV_MAX,             //!< テストタスクイベント種別最大数(削除禁止)
}ENUM_EVT_CMDRCV;

constexpr uint32_t TIMER_CMDRCV_MAX = 1;	//!< タイマー登録数

typedef enum {
    ST_PKTRCV_WAIT_STX = 0,	//!< STX待ち
    ST_PKTRCV_WAIT_ETX,	    //!< ETX待ち（アドレスを含むデータ部受信中）
    ST_PKTRCV_WAIT_BCC,		//!< BCC待ち
}ENUM_ST_PKTRCV;

constexpr uint32_t RECEIVE_PACKET_BUFFER_SIZE = 300;    //!< 受信パケットバッファサイズ
constexpr uint32_t RECEIVE_PACKET_RING_SIZE = 10;       //!< 受信パケットリングバッファ段数

constexpr uint32_t OVER_CNT_NO_RCV_INST = 5;        //!< 読取データ送信後、指示コマンドが受信出来ない状態が何回発生したらリブートするかを指定する
constexpr uint32_t MS_TMOUT_RCV_INST = 1000;        //!< 指示コマンド受信待ちタイムアウト時間(ms)
constexpr uint32_t MS_TMOUT_RCV_PKT = 500;          //!< STX受信後、パケット受信完了までのタイムアウト時間(ms)

constexpr uint32_t _timeOutVal = 51;                 //!< 衝突検知、受信ブロックタイムアウト時間(ms)

constexpr uint32_t QUE_MAX_VALUE = 9;                 //!< イベントキュー、MAX格納値

// 受信パケット データ部の長さ 定義
constexpr uint32_t DT_LEN_INST_CMD = 8;                 //!< 指示コマンド データ部長さ
constexpr uint32_t DT_LEN_STATUS_REQ = 1;               //!< ステータス要求 データ部長さ
constexpr uint32_t DT_LEN_INITIAL_CMD = 42;             //!< イニシャルコマンド データ部長さ
constexpr uint32_t DT_LEN_MENTE_MEM_REQ = 13;           //!< メモリデータ要求 データ部長さ
constexpr uint32_t DT_LEN_MENTE_CLS_CUMULATIVE = 4;     //!< 累積回数クリア データ部長さ
constexpr uint32_t DT_LEN_MENTE_GET_VERSION = 4;        //!< バージョン・チェックサム要求 データ部長さ
constexpr uint32_t DT_LEN_MENTE_DLL_START = 9;          //!< FW更新 ダウンロード開始 データ部長さ
constexpr uint32_t DT_LEN_MENTE_DLL_DATA = 263;         //!< FW更新 データパケット送信 データ部長さ
constexpr uint32_t DT_LEN_MENTE_DLL_UPDATE = 9;         //!< FW更新 更新実行 データ部長さ
constexpr uint32_t DT_LEN_MENTE_FLASH_FORMAT = 4;       //!< データフラッシュ初期化 データ部長さ
constexpr uint32_t DT_LEN_MENTE_REBOOT = 4;             //!< 再起動 データ部長さ
constexpr uint32_t DT_LEN_MENTE_GET_VER_INFO = 4;       //!< バージョン情報取得 データ部長さ

// 受信パケット 種別判定 定義
constexpr uint8_t POS_MENTE_CD = 1;                     //!< メンテコード 位置(0オリジン)
constexpr uint8_t POS_MENTE_TYPE = 2;                   //!< 種別判定 位置(0オリジン)
constexpr uint8_t POS_MENTE_SUBTYPE = 3;                //!< 種別判定サブ 位置(0オリジン)
constexpr uint8_t MENTE_CD = 0xfe;                      //!< メンテコード
constexpr uint8_t MENTE_TYPE_MEM_REQ = 'P';             //!< メモリデータ要求 種別判定文字列
constexpr uint8_t MENTE_TYPE_CLS_CUMULATIVE = '/';      //!< 累積回数クリア 種別判定文字列
constexpr uint8_t MENTE_TYPE_GET_VERSION = ' ';         //!< バージョン・チェックサム要求 種別判定文字列
constexpr uint8_t MENTE_TYPE_DLL_CONT = 'p';            //!< FW更新 制御 種別判定文字列
constexpr uint8_t MENTE_SUBTYPE_DLL_START = '0';        //!< FW更新 ダウンロード開始 種別判定文字列
constexpr uint8_t MENTE_SUBTYPE_DLL_UPDATE = '1';       //!< FW更新 更新実行 種別判定文字列
constexpr uint8_t MENTE_TYPE_DLL_DATA = 'q';            //!< FW更新 データパケット送信 種別判定文字列
constexpr uint8_t MENTE_TYPE_FLASH_FORMAT = 'A';        //!< データフラッシュ初期化 種別判定文字列
constexpr uint8_t MENTE_TYPE_REBOOT = 'B';              //!< 再起動 種別判定文字列
constexpr uint8_t MENTE_TYPE_GET_VER_INFO = 'C';        //!< バージョン情報取得 種別判定文字列

// 指示コマンド 項目位置
constexpr uint32_t POS_LED_IN_INST_CMD = 1;             //!< LED点灯指示 指示コマンド内のデータ位置
constexpr uint32_t LEN_LED_IN_INST_CMD = 4;             //!< LED点灯指示 データ長
constexpr uint32_t POS_BUZZER_IN_INST_CMD = 6;          //!< ブザー鳴動指示 指示コマンド内のデータ位置
constexpr uint32_t LEN_BUZZER_IN_INST_CMD = 1;          //!< ブザー鳴動指示 データ長
constexpr uint32_t POS_AUTODOOR_IN_INST_CMD = 5;        //!< 自動扉出力指示 指示コマンド内のデータ位置
constexpr uint32_t LEN_AUTODOOR_IN_INST_CMD = 1;        //!< 自動扉出力指示 データ長

// ブザー音量調整ボリュームのAD値有効範囲
constexpr uint16_t MIN_VOL_AD_RANGE = 0x0000;        //!< 有効範囲最小値
constexpr uint16_t MAX_VOL_AD_RANGE = 0x0fff;        //!< 有効範囲最大値


/*!------------------------------------------------
@brief コマンド受信タスク
@note
@author
-------------------------------------------------*/
class CommandReceiver : public BaseTask
{
private:
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_CMDRCV_MAX] = {
        "DORMANT",	        //!< 停止（必須）
        "RECEIVING",	    //!< 通常受信中
        "WAITING_INST",	    //!< 指示コマンド待ち
        "DO_REBOOT",	    //!< 再起動実行
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_CMDRCV_MAX] = {
        "IDLE",			    //!< イベント無し(必須)
        "START",			//!< 開始
        "RD_DATA_SENDED",   //!< 読取データ送信完
        "PACKET_RECEIVED",  //!< 受信パケットを受信した
        "TMOUT_RCV_INST",	//!< 指示コマンド受信タイムアウト
    };

public:
    // 各種カウンタ
    uint32_t _CntRdDataSended;  // 読取データ送信完 カウンタ（指示コマンド受信でカウンタはリセット）

private:
    // パケット解析（データ部切り出し）用変数
    ENUM_ST_PKTRCV _StPktRcv;   //!< パケット解析ロジックの状態管理
    uint8_t _RcvDataQue[RECEIVE_PACKET_RING_SIZE][RECEIVE_PACKET_BUFFER_SIZE]; //!< パケット受信データキュー
    uint32_t _PosRcvDataQue;    //!< 現在使用中のパケット受信データキュー位置
    uint32_t _PosDataRcv;       //!< パケット受信データに次にセットする位置（０オリジン）、言い換えると現在セット済のデータ長
    uint32_t _RcvStartTime;     //!< パケット受信を始めた時刻 GetTimerTickで取得

    /*----------------------------
    // 状態遷移ハンドラ
    // ハンドラ名は、基本的に以下で定義する。
    // 　Eventハンドラ：「状態_イベント」
    // 　Enterハンドラ：「状態_Enter」
    // 　Exitハンドラ：「状態_Exit」
    // 複数の状態やイベントで同じハンドラを使う場合は、それが判るような名前を付ける。
    // イベントが発生しても処理しない場合は、登録しない。
    -----------------------------*/
public:
    // 停止 状態
    static void Dormant_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);       // 開始
    
    // 通常受信中・指示コマンド待ち状態 共通
    static void Xxx_RdDataSended(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);    // 読取データ送信完
    static void Xxx_PacketReceived(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // パケットを受信した
    static void Xxx_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // Idle

    // 指示コマンド待ち 状態
    static void WaitingInst_TmoutRcvInst(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // 指示コマンド受信タイムアウト
    static void WaitingInst_Enter(BaseTask* pOwnTask); // Enter
    static void WaitingInst_Exit(BaseTask* pOwnTask); // Exit    

    // 再起動実行 状態
    static void DoReboot_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // Idle

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    CommandReceiver();

    virtual ~CommandReceiver(){}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "CommandReceiver"; }

    /*----------------------------
    // 内部ロジック
    -----------------------------*/
    void SendInstructEvent_To_DataManager_And_OutputController(const uint8_t* pData);    //指示コマンドに対するイベントをデータ管理と出力ポート制御に通知
    void CheckCollision();   // 衝突検知
    void ExtractReceivedPacketAndNoticeInnerEvent(); // 衝突検知・内部イベント通知
    void MovePosRcvDataQueByOne(); // リングバッファ上の受信パケット用バッファ位置を１つ移動
    void ResetReceivedPacket();    // 受信パケット抽出リセット
    void UartCheckAndReset();      //UARTエラー検出/累積回数の更新
    TYPE_ERROR ExtractReceivedPacket(); // パケット解析（受信データからデータ部を切り出し）
    uint16_t GetReceiveByte(); // シリアルポートから１文字受信データを読み込む

 
    /*!------------------------------------------------
    @brief 有効なLEDコマンド
    @note Z:何もしない は無効コマンドとして判断する
    @param cmd 評価するコマンド
    @return 1:有効 0:無効
    @author
    -------------------------------------------------*/
    uint8_t IsEnableLedCommand(uint8_t cmd) { return (('0' <= cmd && cmd <= '9') || ('A' <= cmd && cmd <= 'X')); }

    /*!------------------------------------------------
    @brief 有効なLEDコマンドで再起動時の初期表示用としてROMに保存されるコマンド
    @note Z:何もしない は無効コマンドとして判断する
    @param cmd 評価するコマンド
    @return 1:有効 0:無効
    @author
    -------------------------------------------------*/
    uint8_t IsEnableLedCommandWriteRom(uint8_t cmd) { return (('0' == cmd ) || ('T' <= cmd && cmd <= 'X')); }

    /*!------------------------------------------------
    @brief 有効なブザーコマンド
    @note Z:何もしない は無効コマンドとして判断する
    @param cmd 評価するコマンド
    @return 1:有効 0:無効
    @author
    -------------------------------------------------*/
    uint8_t IsEnableBuzzerCommand(uint8_t cmd) { return ('0' <= cmd && cmd <= '9'); }

    /*!------------------------------------------------
    @brief 有効な自動扉コマンド
    @note Z:何もしない は無効コマンドとして判断する
    @param cmd 評価するコマンド
    @return 1:有効 0:無効
    @author
    -------------------------------------------------*/
    uint8_t IsEnableAutodoorCommand(uint8_t cmd) { return ('0' <= cmd && cmd <= '9'); }

};
#endif
