#ifndef NFC_H
#define NFC_H
#ifdef __cplusplus
extern "C" {
#endif

#define NFC_ANTENNA_ADJUSTMENT_COM_ENABLE   // アンテナ出力調整用、アンテナ出力電圧、電流変更コマンド有効化

#if defined (NXPBUILD__PHHAL_HW_PN5180)   || \
    defined (NXPBUILD__PHHAL_HW_PN5190)   || \
    defined (NXPBUILD__PHHAL_HW_RC663)    || \
    defined (NXPBUILD__PHHAL_HW_PN7462AU) || \
    defined (NXPBUILD__PHHAL_HW_PN7642)
        //#define PH_EXAMPLE1_LPCD_ENABLE             /* If LPCD needs to be configured and used over HAL or over DiscLoop */
#endif
/* Listen Phase TIME */
#define LISTEN_PHASE_TIME_MS              300
/* Enables configuring of Discovery loop */
#define ENABLE_DISC_CONFIG

//エラー定義
//DataManager.hのEVENT_COUNT列挙型にあわせること
typedef enum {
     NFC_ERROR_RW_OK = 0            // 0:エラーなし
   , NFC_ERROR_RW_INIT                // 1:R/W通信エラー回数(初期設定中)
   , NFC_ERROR_RW_POL                // 2:R/W通信エラー回数(ポーリング中:カード捕捉)
   , NFC_ERROR_RW_SELECT             // 3:R/W通信エラー回数(カードセレクト中)
   , NFC_ERROR_RW_POL2               // 4:R/W通信エラー回数(ポーリング中:共通領域)
   , NFC_ERROR_RW_DATA               // 5:R/W通信エラー回数(ブロック２リード中)
   , NFC_ERROR_RW_DATA4              // 6:R/W通信エラー回数(ブロック４リード中)
   , NFC_ERROR_RW_RELEASE            // 7:R/W通信エラー回数(カードリリース)
   , NFC_ERROR_RW_POL3               // 8:R/W通信エラー回数(ポーリング中:カードリリース)
   , NFC_ERROR_RW_M_POL              // 9:R/W通信エラー回数(MIFARE ポーリング中:カード捕捉)
   , NFC_ERROR_RW_M_AUTH1            // 10:R/W通信エラー回数(MIFARE 1ブロック目認証)
   , NFC_ERROR_RW_M_AUTH2            // 11:R/W通信エラー回数(MIFARE 2ブロック目認証)
   , NFC_ERROR_RW_M_READ1            // 12:R/W通信エラー回数(MIFARE 1ブロック目リード)
   , NFC_ERROR_RW_M_READ2            // 13:R/W通信エラー回数(MIFARE 2ブロック目リード)
   , NFC_ERROR_RW_MAX
}ENUM_NFC_ERROR;

//読出しデータ格納定義
#define NFC_UIDIDM_LEN     (8)
typedef struct _type_read{
    uint8_t NumBlocks;      //受信ブロック数格納
    uint8_t BlockData[32];  //受信Blockデータ格納(最大2ブロック分 16byte * 2)
    uint8_t UidIdm[NFC_UIDIDM_LEN];      //受信UID/IDMデータ格納
    uint8_t UidIdmLength;   //受信UID/IDMデータ長
    uint8_t Type;           //受信データタイプ([bit7]0:Mifare/1:FeliCa [bit0]0:FTS/1:SurPass )
}TYPE_NFC_DATA;
//読出しデータタイプ定義
#define DEV_TYPE_MIFARE     (0x40)
#define DEV_TYPE_MIFARE_1K  (DEV_TYPE_MIFARE | 0x01)
#define DEV_TYPE_MIFARE_4K  (DEV_TYPE_MIFARE | 0x02)
#define DEV_TYPE_FELICA     (0x80)
#define DEV_TYPE_FELICA_FTS (DEV_TYPE_FELICA | 0x01)
#define DEV_TYPE_FELICA_SP  (DEV_TYPE_FELICA | 0x02)

#define IS_MIFARE(x)        ((x & DEV_TYPE_MIFARE) == DEV_TYPE_MIFARE)
#define IS_MIFARE_1K(x)     ((x & (DEV_TYPE_MIFARE | 0x0F)) == DEV_TYPE_MIFARE_1K)
#define IS_MIFARE_4K(x)     ((x & (DEV_TYPE_MIFARE | 0x0F)) == DEV_TYPE_MIFARE_4K)
#define IS_FELICA(x)        ((x & DEV_TYPE_FELICA) == DEV_TYPE_FELICA)
#define IS_FELICA_FTS(x)    ((x & (DEV_TYPE_FELICA | 0x0F)) == DEV_TYPE_FELICA_FTS)
#define IS_FELICA_SP(x)     ((x & (DEV_TYPE_FELICA | 0x0F)) == DEV_TYPE_FELICA_SP)

// アンテナ出力定義

// 最大値(設定上限値)
#define NFC_OUTPUT_VOLTAGE_MAX 0x2A  // 5.7V
#define NFC_OUTPUT_CURRENT_MAX 350   // 350mA

// 現行機（三和製）と同等出力が出る電圧、電流値 を 計測結果から設定
#define NFC_OUTPUT_VOLTAGE 0x1D // 4.4V
#define NFC_OUTPUT_CURRENT 350  // 350mA MAX値

extern bool InitNFC();
extern uint16_t DoNFC(void);
extern void NFCDoTask(void);
extern void NFCDebugPrint(char* format, ...);
extern void NFCClearParameterFeliCa(void);
extern void NFCClearParameterMifare(void);
extern bool NFCSetParameterFeliCa(uint8_t ValidBlock1 ,uint8_t ValidBlock2,
                                    uint8_t *pSystemCode, uint8_t *pServiceCode ,uint8_t Block1 ,uint8_t Block2);
extern bool NFCSetParameterMifare(uint8_t Block1,  uint8_t KeyType1, uint8_t *pKeyData1,
                                    uint8_t Block2,  uint8_t KeyType2, uint8_t *pKeyData2);
extern void NFCParameterUpdate(void);
extern void NFCDetectedCard(bool IsDetect);
extern void NFCNoticeCardData(TYPE_NFC_DATA *pNfcData);
extern void NFCDetectError(ENUM_NFC_ERROR err);
extern void NFCInitComlpete(void);
extern void NFCSetDetectFeliCaType(uint8_t Type);
extern uint16_t NFCGetFwVersion();
extern uint8_t NFCGetOutputPowerVoltage(void);
extern bool NFCSetOutputPowerVoltage(uint8_t Voltage);
extern uint16_t NFCGetOutputPowerCurrent(void);
extern bool NFCSetOutputPowerCurrent(uint16_t Current);
extern void NFCResetPinWriteError(void);
extern void NFCSetTargetCard(uint8_t Target);
#ifdef __cplusplus
}
#endif

#endif /* NFC_H */

