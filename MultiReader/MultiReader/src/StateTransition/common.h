/*!------------------------------------------------
@brief 共通処理クラス
@note 各タスクのインナークラスで継承させること
@author
-------------------------------------------------*/
#ifndef COMMON_H
#define COMMON_H
//////////////////////////////////////////////////////////////
//VisualStudio C++(Windows)で動作する場合はWINDOWS_DEBUGを有効にする
//その他環境ではコメントアウトすること
//#define WINDOWS_DEBUG
/////////////////////////////////////////////////////////////

#include <string.h>
#include <stdint.h>
#include "typedef.h"
#include "assert.h"
#ifndef WINDOWS_DEBUG
    #include "hal_data.h"
    #include "../Device/DeviceDriver.h"
#endif


#ifdef WINDOWS_DEBUG
    //!割込み禁止(使用するマイコンにあわせてカスタマイズすること)
    #define DEFINE_INT()
    #define DISABLE_INT()
    #define ENABLE_INT()
#else
    #define DEFINE_INT() FSP_CRITICAL_SECTION_DEFINE;
    //!割込み禁止(使用するマイコンにあわせてカスタマイズすること)
    #define DISABLE_INT() FSP_CRITICAL_SECTION_ENTER;
    //!割込み許可
    #define ENABLE_INT() FSP_CRITICAL_SECTION_EXIT;
#endif


class ClsCommon {
private:
	
public:
	static void I2HexAsciiByte(uint8_t Val, char* pHexH, char* pHexL);
    static bool AsciiHex2Int(const char HexH, char HexL, uint8_t* pBuff);
    static int32_t AsciiHex2Bin(const char* pAscii, uint32_t Len, uint8_t* pBinary, uint32_t Size);
// 削除予定)  static int32_t Bin2AsciiHex(const uint8_t* pBinary, uint32_t Len, char* pAscii, uint32_t Size);
    static uint32_t ITOA(int32_t n, char* pStr, uint32_t Size);
    static TYPE_ERROR ATOI(const char* pStr, uint32_t Len, uint32_t* pNum);
    static TYPE_ERROR AsciiHex2Uint32(const char* pStr, uint32_t Len, uint32_t* pNum);
    static int32_t ZeroSuppress(uint32_t n, uint32_t digit, char* out, uint32_t size);
    static void I2HexAscii(uint32_t n, uint8_t Digit, char* pStrHex);

    static void DebugPrintNum(void (*funcPrint)(const char*), int32_t Num);
    static void DebugPrintHex(void (*funcPrint)(const char*), uint32_t Num);
    static void DebugPrintDataHex(void (*funcPrint)(const char*), const uint8_t* pData, uint32_t Len);
    static void DebugPrintBuf(void (*funcPrint)(const char*), const uint8_t* pData, uint32_t Len);
    static void PrintInfoLine(void (*funcPrint)(const char*), const char* title, const char* strVal);
    static void PrintInfoDump(void (*funcPrint)(const char*), const char* title, const uint8_t* pData, uint32_t Len);
    static void PrintInfoNum(void (*funcPrint)(const char*), const char* title, uint32_t val);
    static void PrintInfoHex(void (*funcPrint)(const char*), const char* title, uint32_t val);

    static void ConvertSendData(uint8_t *pData, uint8_t DataLen);
};

#ifndef GETMIN_FINCDEF
#   define GetMin(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif
#ifndef GETMAX_FINCDEF
#   define GETMAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#endif

#endif

