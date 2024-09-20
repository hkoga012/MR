/*!------------------------------------------------
@brief ハンドラ基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
#ifndef BASE_HANDLER_H
#define BASE_HANDLER_H
#include "common.h"

class BaseTask;
class State;

/*!------------------------------------------------
@brief イベントハンドラ基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
class BaseEventHandler
{
public:
    PF_DO_EVENT doEventFunc;  //!< イベントハンドラポインタ

    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    BaseEventHandler(PF_DO_EVENT pFunc) : doEventFunc(pFunc) {}
    virtual ~BaseEventHandler() = default;
};

/*!------------------------------------------------
@brief 状態入口処理基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
class BaseEnterHandler
{
public:
    PF_DO_ENTER doEnterFunc;  //!< 状態入口処理ハンドラポインタ

    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    BaseEnterHandler(PF_DO_ENTER pFunc) : doEnterFunc(pFunc) {}
    virtual ~BaseEnterHandler() = default;
};

/*!------------------------------------------------
@brief 状態出口処理基底クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
class BaseExitHandler
{
public:
    PF_DO_EXIT doExitFunc;    //!< 状態出口処理ハンドラポインタ

    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    BaseExitHandler(PF_DO_EXIT pFunc) : doExitFunc(pFunc) {}
    virtual ~BaseExitHandler() = default;
};
#endif


