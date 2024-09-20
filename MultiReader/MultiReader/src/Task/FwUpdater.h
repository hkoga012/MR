/*!------------------------------------------------
@brief ファームウエア更新タスクヘッダ
@note  
@author
-------------------------------------------------*/
#ifndef FW_UPDATER_H
#define FW_UPDATER_H
#include "../StateTransition/BaseTask.h"

//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_FW_DORMANT = 0,		        //!< 停止（必須）
    ST_FW_DOWN_LOADING,		        //!< ダウンロード中
    ST_FW_NORMAL_REBOOT,            //!< 正常再起動
    ST_FW_ABNORMAL_REBOOT,          //!< 異常再起動
    ST_FW_MAX,				        //!< イベント種別の最大数(削除禁止)
}ENUM_STATE_FW;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
//*****************************************
//!< イベント定義
constexpr uint32_t FW_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_FW_IDLE = 0,        //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント

//《タスク外から通知されるイベント》
    EVT_FW_DLL_START,       //!< FW更新 ダウンロード開始
    EVT_FW_DLL_DATA,        //!< FW更新 データパケット受信
    EVT_FW_DLL_UPDATE,      //!< FW更新 更新実行　※未使用

    //《タスク内部イベント》
    EVT_FW_RECEIVE_TMOUT,   //!< 受信タイムアウト通知

    EVT_FW_MAX,             //!< テストタスクイベント種別最大数(削除禁止)
}ENUM_EVT_FW;


constexpr uint32_t TIMER_FW_MAX = 1;	//!< タイマー登録数

/*!------------------------------------------------
@brief ファームウエア更新タスククラス
@note
@author
-------------------------------------------------*/
class FwUpdater : public BaseTask
{
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_FW_MAX] = {
        "DORMANT",	            //!< 停止
        "DOWN_LOADING",         //!< ダウンロード中
        "NORMAL_REBOOT",	    //!< 正常再起動
        "ABNORMAL_REBOOT",	    //!< 異常再起動
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_FW_MAX] = {
        "IDLE",			        //!< イベント無し(必須)
        "DLL_START",            //!< ダウンロード開始
        "DLL_DATA",             //!< データパケット受信
        "DLL_UPDATE",	        //!< 更新実行　※未使用
        "RECEIVE_TMOUT",	    //!< 受信タイムアウト
    };
public:

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
    static void Dormant_DllStart(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen); // ダウンロード開始

    // ダウンロード中 状態
    static void DownLoading_Enter(BaseTask* pOwnTask); // Enter
    static void DownLoading_DllData(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // データパケット受信
    static void DownLoading_ReceiveTmout(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);  // 受信タイムアウト
    static void DownLoading_Exit(BaseTask* pOwnTask); // Exit

    // 正常再起動 状態
    static void NormalReboot_Enter(BaseTask* pOwnTask); // Enter

    // 異常再起動 状態
    static void AbnormalReboot_Enter(BaseTask* pOwnTask); // Enter
    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    FwUpdater();
    virtual ~FwUpdater() {}

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "FwUpdater"; }

private:
    /*----------------------------
    //内部関数
    -----------------------------*/
    TYPE_ERROR WriteCodeFlash(const uint8_t* pBuf, const uint32_t len, uint32_t headAddress);//CodeFlash書込み関数
    void Reboot();//再起動実行関数
    TYPE_ERROR InitCodeFlash();
    TYPE_ERROR EraseCodeFlash();
    /*----------------------------
    //内部変数
    -----------------------------*/
    //・コードフラッシュ関連
    const uint32_t _adrs_Region2_Begin = 0x00021800;//領域2先頭アドレス
    const uint32_t _adrs_Region2_End = 0x0003F7FF;//領域2後尾アドレス
    const uint32_t _code_Flash_Block_Size = 2048;//1Block:2KB
    const uint32_t _code_Flash_Block_Total = 60;//領域2：60Block(120KB)
    const uint16_t maxBlockNum = 1024;//ブロックナンバーの最大値
    const uint16_t getmaxBlockNum = 960;//データ取得するブロックナンバーの最大値(後半8KBは捨てる為、960)
    //・ロジック関連
    const uint16_t _tmoutValue1s = 1000;//受信タイムアウト：１秒
    const uint16_t _tmoutValue3s = 3000;//受信タイムアウト：１秒
    uint32_t _previousBlockNum = 0;//前回のブロックナンバー
};

#endif
