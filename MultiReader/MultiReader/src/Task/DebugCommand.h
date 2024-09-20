/*!------------------------------------------------
@brief デバッグコマンドタスクヘッダ
@note
@author
-------------------------------------------------*/
#ifndef DEBUGCMD_H
#define DEBUGCMD_H
#include "../StateTransition/BaseTask.h"
#include "../version.h"
#include "../DebugOption.h"
#ifdef WINDOWS_DEBUG
#include "../../../WinMultiReader/Device/SerialPort.h"
#else
#include "./nfc/intfs/nfc_ctrl.h"
#endif
#include "stdarg.h"
#include <stdio.h>

//******************************************
// 状態は最低一つ登録する。起動直後は先頭に登録された状態(値=0)に遷移する
//*****************************************
//!< 状態定義
typedef enum {
    ST_DEBUGCMD_INIT = 0,	    //!< 初期処理中（先頭必須）
    ST_DEBUGCMD_STOP,			//!< 停止中
    ST_DEBUGCMD_RUNNING,		//!< 動作中
    ST_DEBUGCMD_MAX,            //!< イベント種別の最大数(削除禁止)
}ENUM_STATE_DEBUGCMD;

//******************************************
// イベントを定義する際は必ずIdle(値=0)を定義すること
// *****************************************
//!< イベント定義
constexpr uint32_t DEBUGCMD_EVENT_QUE_SIZE = 10; //!< イベントバッファサイズ
typedef enum {
    EVT_DEBUGCMD_IDLE = 0,      //!< イベント無し(必須) ― イベントが無い（Idle状態の）時に実行するハンドラを登録する為に設けたダミーイベント

    //《タスク外から通知されるイベント》

    //《タスク内部イベント》
    EVT_DEBUGCMD_INIT_DISP,		//!< 初期表示指示
    EVT_DEBUGCMD_STOP,			//!< 停止指示
    EVT_DEBUGCMD_START,			//!< 動作指示

    EVT_DEBUGCMD_MAX,           //!< テストタスクイベント種別最大数(削除禁止)
}ENUM_EVT_DEBUGCMD;

constexpr uint32_t TIMER_DEBUGCMD_MAX = 0;	//!< タイマー登録数
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
constexpr uint8_t DEBUGCMD_MAX = 10 + 1;                 //!<コマンド数
#else
constexpr uint8_t DEBUGCMD_MAX = 9 + 1;                 //!<コマンド数
#endif
constexpr uint8_t ARG_MAX = 8;		                    //コマンドの引数最大値
constexpr uint32_t DEBUG_COMMAND_BUFFER_SIZE = 100;

/*!------------------------------------------------
@brief デバッグコマンドタスククラス
@note　
@author
-------------------------------------------------*/
class DebugCommand : public BaseTask
{
public:
    static void SendPrintSci(const char* buf);  // 他タスクも呼べるように public に
#ifndef WINDOWS_DEBUG
    static void SendPrintSciEx(const char* format, ...); // Format対応版 RAマイコンのみ出力
#else
    static void SendPrintSciEx(const char* format, ...) {} // Windows上では空関数
#endif

private:
    //デバッグ用状態文字定義
    //文字列配列_pStateString[]の文字列は必ずenumの状態定義順に合わせること
    const char* _pStateString[ST_DEBUGCMD_MAX] = {
        "INIT",	        //!< 初期処理中
        "STOP",	        //!< 停止
        "RUNNING",	    //!< 動作中
    };

    //デバッグ用イベント文字定義
    //文字列配列_pEventString[]の文字列は必ずenumのイベント定義順に合わせること
    const char* _pEventString[EVT_DEBUGCMD_MAX] = {
        "IDLE",			//!< イベント無し(Idle)
        "INIT_DISP",	//!< 初期表示指示 
        "STOP",			//!< 停止指示
        "START",	    //!< 動作指示
    };

    const char* pCmdResOK = "OK\r\n\r\n";
    const char* pCmdResNG = "NG\r\n\r\n";
    const char* pUnknownCmd = "unknown command\r\n\r\n";
    const char* pCrlf = "\r\n";

    const char ASCII_BACKSPACE = 0x08;	// バックスペースASCIIコード
    const char ASCII_SPACE = 0x20;		// スペースASCIIコード
    const char ASCII_CR = '\r';		    // CR
    const char ASCII_LF = '\n';		    // LF
    const char ASCII_TAB = '\t';		// タブASCIIコード

    //デバッグコマンド文字列格納用バッファ
    char _debug_command_buffer[DEBUG_COMMAND_BUFFER_SIZE];
    char _commandStr[DEBUG_COMMAND_BUFFER_SIZE];
    uint32_t _received_length;
    uint32_t _command_Index;
    uint8_t _isPreviousCommandSetted;   // 直前に実行したコマンドがセットされているか

    //!<コマンド定義構造体
    struct CommandData {
    public:
        const char* pStrCommand;      //!<コマンド文字列
        TYPE_ERROR (DebugCommand::* pFunc)(int32_t argc, const char* argv[]);  //!<コマンド実行クラスポインタ
        const char* pStrHelp;      //!<コマンド説明文字列
    };

    //コマンド処理関数宣言
    void DeleteBackspaceChar(char*);
    TYPE_ERROR SeekAndExecuteCommand(char* pCmdBuffer);
    void SplitArgument(char* str, int32_t* argc, const char* argv[]);
    void SendPrintSciTitle(const char* str, uint8_t tabNum);
    uint32_t GetReceiveData(char*, uint32_t );

    void DebugPrintNum(int32_t Num) { ClsCommon::DebugPrintNum(SendPrintSci, Num); }
    void DebugPrintHex(uint32_t Num) { ClsCommon::DebugPrintHex(SendPrintSci, Num); }
    void DebugPrintDataHex(const uint8_t* pData, uint32_t Len) { ClsCommon::DebugPrintDataHex(SendPrintSci, pData, Len); }


    //タスク文字列からタスクオブジェクトポンタ取得
    BaseTask* GetTaskPointer(const char* pStrTask);

    //************************************************
    // ココに登録するコマンドのプロトタイプ宣言を定義する
    //************************************************
    //コマンド処理プロトタイプ宣言
    TYPE_ERROR CmdVersion(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdPrintStatus(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdPrintInfo(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdSendEvent(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdPrintHelp(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdPrintTaskLoad(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdDataFlashFunc(int32_t argc, const char* argv[]);
    TYPE_ERROR CmdReboot(int32_t argc, const char* argv[]);
    // TYPE_ERROR CmdSerialNoFunc(int32_t argc, const char* argv[]);
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
    TYPE_ERROR CmdSetAntParam(int32_t argc, const char* argv[]);
#endif
    TYPE_ERROR CmdDoDebug(int32_t argc, const char* argv[]);

    //************************************************
    // ココでコマンドを登録する
    // コマンドを追加したらDEBUGCMD_MAXの定義を+1すること
    //************************************************
    //デバッグコマンド定義
    CommandData _pCommandList[DEBUGCMD_MAX] = {
        /*          コマンド文字列  コマンド処理関数                    コマンド説明              */
        CommandData{ "ver",         &DebugCommand::CmdVersion,          "print firmware version.", },
        CommandData{ "status",      &DebugCommand::CmdPrintStatus,      "print Task Status.",       },
        CommandData{ "info",        &DebugCommand::CmdPrintInfo,        "print Task Info.",   },
        CommandData{ "event",       &DebugCommand::CmdSendEvent,        "send an event to task.",   },
        CommandData{ "load",        &DebugCommand::CmdPrintTaskLoad,    "print Task Load.",   },
        CommandData{ "dflash",      &DebugCommand::CmdDataFlashFunc,    "data flash functions.",   },
        CommandData{ "reboot",      &DebugCommand::CmdReboot,           "reboot the multi reader.",   },
        // CommandData{ "serno",       &DebugCommand::CmdSerialNoFunc,     "serial no get/put",   },
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
        CommandData{ "ant",         &DebugCommand::CmdSetAntParam,      "Antenna parameter settings.",   },
#endif
        CommandData{ "debug",       &DebugCommand::CmdDoDebug,          "temporarily used for debugging.", },
        CommandData{ "?",           &DebugCommand::CmdPrintHelp,        "print all command.",      },
                   { NULL,           NULL,                               NULL                      }, //終端(削除禁止)
    };

public:
    const char* VERSION_INFO = GetVerInfo();
    //デバッグ用コマンドクラス
    class TaskCmd {
    public:
        const char *pCmdString;
        BaseTask   *pTask;
    };
    TaskCmd *_pTaskCmdParam;
    //************************************************
    // 以下変更禁止
    //************************************************

    /*----------------------------
    // 状態遷移ハンドラ
    // ハンドラ名は、基本的に以下で定義する。
    // 　Eventハンドラ：「状態_イベント」
    // 　Enterハンドラ：「状態_Enter」
    // 　Exitハンドラ：「状態_Exit」
    // 複数の状態やイベントで同じハンドラを使う場合は、それが判るような名前を付ける。
    // イベントが発生しても処理しない場合は、登録しない。
    -----------------------------*/
    // 初期化中状態
    static void Init_InitDisp(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);   // 初期表示指示
    static void Init_Enter(BaseTask* pOwnTask);    // Enter

    // 停止中状態
    static void Stop_Start(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);      // 開始指示

    // 動作中状態
    static void Running_Stop(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);	// 停止指示
    static void Running_Idle(BaseTask* pOwnTask, uint32_t EventId, const uint8_t* pData, uint32_t DataLen);	// イベント無し

    /*----------------------------
    //コマンド解析・実行
    -----------------------------*/
    TYPE_ERROR DoCommand(void);

    uint8_t IsValidKey(char key)
    {
        uint8_t isValid
            = (((0x20 <= key) && (key <= 0x7e))
                        || (key == ASCII_BACKSPACE)
                        || (key == ASCII_CR)
                        || (key == ASCII_LF))
                ? 1
                : 0;

        return isValid;
    }

    /*----------------------------
    //コンストラクタ
    -----------------------------*/
    DebugCommand();
    virtual ~DebugCommand() {}

    void UartCheckAndReset(void);
    char* GetVerInfo(void);

    /*!------------------------------------------------
    @brief タスク名
    -------------------------------------------------*/
    const char* GetTaskName() { return "DebugCommand"; }


	//==== 時間計測用　デバッグ関数 ====
public:
    void SetDebugTime(int32_t pos);
    void PrintDebugTime();
private:
    uint32_t _debugTime[10];    // デバッグ用なので、tickのオーバフローは考慮しない
    int32_t _debugTimeNextNum = 0;
};

#endif
