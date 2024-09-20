/*!------------------------------------------------
@brief タスクポインタ定義ヘッダ
@note
@author
-------------------------------------------------*/
#ifndef TASK_H
#define TASK_H
#include "../StateTransition/FatalException.h"
#include "DebugCommand.h"
#include "OutputController.h"
#include "DataManager.h"
#include "NfcReader.h"
#include "QrReader.h"
#include "FwUpdater.h"
#include "CommandReceiver.h"
#include "EventSender.h"
#include "../DebugOption.h"

//※タスク追加時はDebugCommand.cppのCmdPrintStatus関数にも追加すること

/////////////////////////////////////////////////////
//ココに登録するタスクのポインタ変数を定義
/////////////////////////////////////////////////////
constexpr uint32_t TASK_NUMBER = 8;	//!< 登録するタスク数

extern DebugCommand* G_pDebugCommand;
extern OutputController* G_pOutputController;
extern NfcReader* G_pNfcReader;
extern QrReader* G_pQrReader;
extern DataManager* G_pDataManager;
extern FwUpdater* G_pFwUpdater;
extern CommandReceiver* G_pCommandReceiver;
extern EventSender* G_pEventSender;


#endif
