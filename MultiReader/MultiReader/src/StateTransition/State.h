/*!------------------------------------------------
@brief 状態基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
#ifndef STATE_H
#define STATE_H
#include "BaseTask.h"
#include "BaseHandler.h"
#include "FatalException.h"

class State {
private:
    BaseTask* _pOwnTask;                    //!< 自タスクオブジェクトポインタ
    const uint32_t      _stateId;           //!< 状態ID
    const uint32_t      _maxEvent;          //!< イベント最大数
    BaseEventHandler**  _pEventHandlerList; //!< イベントハンドラクラスポインタリスト 1次元配列
    uint32_t            _eventIndex;        //!< イベントの登録数
    BaseEnterHandler*   _enterHandler;      //!< 状態入口処理クラスポインタ
    BaseExitHandler*    _exitHandler;       //!< 状態出口処理クラスポインタ

public:
    explicit State(BaseTask* pTask, uint32_t StateId, uint32_t MaxEvent);
    ~State() = default;

    const uint32_t& GetId(void);
    void Enter(void);
    void Update(const uint32_t& EventId, const uint8_t* pData, uint32_t DataLen);
    void Exit(void);
    TYPE_ERROR AddEventHandler(uint32_t EventId, PF_DO_EVENT pFunc);
    TYPE_ERROR AddEnterHandler(PF_DO_ENTER pFunc);
    TYPE_ERROR AddExitHandler(PF_DO_EXIT pFunc);
};
#endif


