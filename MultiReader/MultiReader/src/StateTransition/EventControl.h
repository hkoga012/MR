/*!------------------------------------------------
@brief イベント管理クラスヘッダ
@note　各タスクよりイベントを受付け、キューにためる
       各タスクより要求があればキューから取り出して渡す
@author
-------------------------------------------------*/

#ifndef EVENT_CONTROL_H
#define EVENT_CONTROL_H
#include "Queue.h"

class EventControl {
private:
    BaseTask* _pOwnTask;        //!< 自タスクオブジェクトポインタ

    //!< イベント管理データ
    class   CtrlData {
    public:
        uint32_t EventId;		//!< イベントID
        const uint8_t* pData;		    //!< イベントデータ(データ格納先頭ポインタをセットしても良い)
        uint32_t DataLen;
    };

    Queue<CtrlData> *_pQueue;  //!< イベントキュー ポインタの1次元配列
public:
    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    EventControl(BaseTask* pTask, const uint32_t MaxEvent);

    /*!------------------------------------------------
    @brief デストラクタ
    -------------------------------------------------*/
    ~EventControl() = default;
    
    /*!------------------------------------------------
    @brief イベントセット
    -------------------------------------------------*/
    void SetEvent(const uint32_t EventId, const uint8_t* pData, uint32_t DataLen);
    
    /*!------------------------------------------------
     @brief イベント取得
    -------------------------------------------------*/
    TYPE_ERROR GetEvent(uint32_t* pEventId, const uint8_t** pData, uint32_t* pDataLen);
	
    /*!------------------------------------------------
     @brief イベントキューのオーバー検知
    -------------------------------------------------*/
    bool IsOver(uint32_t limit);

};
#endif

