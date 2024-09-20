/*!------------------------------------------------
@brief NFC読取り処理
@note　NFC Reader Library v07.09.00 for PN5190 including all software examplesにあるサンプルコードの
　　　　　以下のファイルをベースに変更
　　　　　NfcrdlibEx1_DiscoveryLoop.c
　　　　　NfcrdlibEx4_MIFAREClassic.c
@author
-------------------------------------------------*/
/**
* Reader Library Headers
*/

#include <phApp_Init.h>
/* Local headers */
	
/*******************************************************************************
**   Definitions
*******************************************************************************/

phacDiscLoop_Sw_DataParams_t       * pDiscLoop;       /* Discovery loop component */
//add
void *psKeyStore;
void *psalMFC;
void *psalFeliCa;

/*The below variables needs to be initialized according to example requirements by a customer */
uint8_t  sens_res[2]     = {0x04, 0x00};              /* ATQ bytes - needed for anti-collision */
uint8_t  nfc_id1[3]      = {0xA1, 0xA2, 0xA3};        /* user defined bytes of the UID (one is hardcoded) - needed for anti-collision */
uint8_t  sel_res         = 0x40;
uint8_t  nfc_id3         = 0xFA;                      /* NFC3 byte - required for anti-collision */
uint8_t  poll_res[18]    = {0x01, 0xFE, 0xB2, 0xB3, 0xB4, 0xB5,
                                   0xB6, 0xB7, 0xC0, 0xC1, 0xC2, 0xC3,
                                   0xC4, 0xC5, 0xC6, 0xC7, 0x23, 0x45 };

//add ---------------------------------------------
// for Mifare
#define NUMBER_OF_KEYENTRIES        2
#define NUMBER_OF_KEYVERSIONPAIRS   2
#define NUMBER_OF_KUCENTRIES        1

#define DATA_BUFFER_LEN             16*2 /* Buffer length */
#define MFC_BLOCK_DATA_SIZE         16 /* Block Data size - 16 Bytes */
/*PAL variables*/
phKeyStore_Sw_KeyEntry_t           sKeyEntries[NUMBER_OF_KEYENTRIES];                                  /* Sw KeyEntry structure */
phKeyStore_Sw_KUCEntry_t           sKUCEntries[NUMBER_OF_KUCENTRIES];                                  /* Sw Key usage counter structure */
phKeyStore_Sw_KeyVersionPair_t     sKeyVersionPairs[NUMBER_OF_KEYVERSIONPAIRS * NUMBER_OF_KEYENTRIES]; /* Sw KeyVersionPair structure */

uint8_t                            bDataBuffer[DATA_BUFFER_LEN];  /* universal data buffer */

uint8_t                            bSak;                      /* SAK  card type information */
uint16_t                           wAtqa;                     /* ATQA card type information */

//MIFARE Classic用定義
typedef struct {
	uint8_t ReadCount;			//読出しブロック数(1 or 2)
	uint8_t KeyIndex;		    //キー保存位置(常に1)
	uint8_t KeyVer;			    //キーバージョン(常に０)
	uint8_t BlockNum[2];		//読出しブロック(0:Block1, 1:Block2)
	uint8_t KeyType[2];			//認証使用Key (0:Block1, 1:Block2) (PHHAL_HW_MFC_KEYA or PHHAL_HW_MFC_KEYB)
	uint8_t Key[12];            //キーコード(KeyA/KeyB)
}TYPE_MFC;

//Mifare 読出し用定義
TYPE_MFC tMfc = {
     2,         //読出しブロック数
     1,         //キー保存位置(常に1)
     0,         //キーバージョン(常に０)
     {4, 5},   //読出しブロック(0:Block1, 1:Block2)
     {PHHAL_HW_MFC_KEYA, PHHAL_HW_MFC_KEYA}, //認証使用Key (0:Block1, 1:Block2)
     {0x40, 0x7F, 0x32, 0xA9, 0xFE, 0xB2,   //認証Keyデータ(KeyA:0-5 )
     0x40, 0x7F, 0x32, 0xA9, 0xFE, 0xB2}    //認証Keyデータ(KeyB:6-12)
};
TYPE_MFC tTempMfc;                          //更新前データ保持

//FeliCa要定義
#define NFC_MIFARE_FELICA_RFOFF_INTERVAL_US (5000)     //Mifare/FeliCa切り替え時の待ち時間(usec)
#define NFC_FELICA_RETRY_INTERVAL_US         (5000)       //FeliCa読出しリトライ時の待ち時間(usec)
enum{
    FELICA_TYPE_FTSIC = 0,   //FTSic
    FELICA_TYPE_SURPASS,     //SurPass
    FELICA_TYPE_MAX,
};
typedef struct {
    uint8_t ReadCount;          //読出しブロック数(1)
    uint8_t ServiceCode[FELICA_TYPE_MAX][2];  //サービスコードリスト 0:FTSic,1:SurPass
    uint8_t BlockList[4];       //ブロックリスト(読取りブロックを2バイト毎に指定)
}TYPE_FELICA;
TYPE_FELICA tFeliCa = {
    2,                         //読出しブロック数(1 or 2)
    {{0x4B, 0x49},{0x8B, 0x43}},//サービスコードリスト {{FTSic},{SurPass}}
    {0x80, 0x01, 0x80, 0x03},  //ブロックリスト(読取りブロックを2バイト毎に指定)
};
TYPE_FELICA tTempFeliCa;        //更新前データ保持

//FeliCaから読み書き可能なIDMが取得できない問題対応(phacDiscLoop_Sw_Int_F.cファイル内で参照)
uint8_t pFeliCaSystemCode[2] = {0xFF, 0xFF};    //{0xFE, 0x00};
uint8_t pTempFeliCaSystemCode[2];

//Mifare/FeliCa共通
TYPE_NFC_DATA tNfcData;            //読込データ格納用構造体変数

enum{
    PARAM_MIFARE = 0,   //Mifare
    PARAM_FELICA,       //FeliCa
    PARAM_MAX,
};
//パラメータがセットされたか(false:未設定、true:設定済)(配列PARAM_MIFARE:Mifare, 配列PARAM_FELICA:FeliCa)
bool IsSetParameter[PARAM_MAX] = {false,false};
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
//パラメータ更新実行フラグ(false:更新実行しない、true:更新実行)
bool ParameterUpdateFlag = false;
#endif /* NFC_ANTENNA_ADJUSTMENT_COM_ENABLE */
//読出し・認証リトライ回数
#define MIFARE_RETRY (1)
#define FELICA_RETRY (5)

//アンテナ出力
uint8_t OutputPowerVoltage = NFC_OUTPUT_VOLTAGE;
uint16_t OutputPowerCurrent = NFC_OUTPUT_CURRENT;

//FWバージョン
static uint16_t __NfcFwVersion = 0xffff;

//送信カウンタ(イベントバッファ送信停止)
#define SEND_INTERVAL 5
uint8_t SendInterval = SEND_INTERVAL;

//検出対象カード(DIPSW1で指定)
static uint8_t __TargetCard = 0x00;

enum{
    TECH_IDX_MIFARE = 0,   //Mifare
    TECH_IDX_FELICA,       //FeliCa
    TECH_IDX_MAX,
};
//検出対象カード種別定義
static uint16_t Technology[TECH_IDX_MAX] = {
                   PHAC_DISCLOOP_POS_BIT_MASK_A,
                   (PHAC_DISCLOOP_POS_BIT_MASK_F212 | PHAC_DISCLOOP_POS_BIT_MASK_F424)};
static uint8_t TargetTechnology = 0;

ENUM_NFC_ERROR __NFCError = 0;

//add ---------------------------------------------
#ifdef PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION
uint32_t aDiscTaskBuffer[DISC_DEMO_TASK_STACK];
#else /* PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION */
#define aDiscTaskBuffer    NULL
#endif /* PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION */

#ifdef PH_OSAL_FREERTOS
const uint8_t bTaskName[configMAX_TASK_NAME_LEN] = {"DiscLoop"};
#else
const uint8_t bTaskName[] = {"DiscLoop"};
#endif /* PH_OSAL_FREERTOS */

/*******************************************************************************
**   Static Defines
*******************************************************************************/

/* This is used to save restore Poll Config.
 * If in case application has update/change PollCfg to resolve Tech
 * when Multiple Tech was detected in previous poll cycle
 */
static uint16_t bSavePollTechCfg;

/*******************************************************************************
**   Prototypes
*******************************************************************************/

phStatus_t DiscoveryLoop(void  *pDataParams);
uint16_t NFCForumProcess(uint16_t wEntryPoint, phStatus_t DiscLoopStatus);
phStatus_t ReadMifare(void *pMFC, uint8_t *pUid, uint8_t UidSize, uint8_t Sak, TYPE_NFC_DATA *pNfcData);
phStatus_t ReadFeliCa(void *pFeliCa, TYPE_NFC_DATA *pNfcData);
phStatus_t ReadNFC(phacDiscLoop_Sw_DataParams_t * pLoop, void *pFeliCa, void *pMifare, uint8_t TagIndex, TYPE_NFC_DATA *pNfcData);
#ifdef ENABLE_DISC_CONFIG
static phStatus_t LoadProfile(void);
#endif /* ENABLE_DISC_CONFIG */

void NFCSetTargetCard(uint8_t Target)
{
    __TargetCard = Target;
}
uint16_t NFCGetFwVersion()
{
    return __NfcFwVersion;
}

/*!------------------------------------------------
@brief アンテナ出力電圧取得
@note
@param なし
@return　電圧(0x00-0x2A)
@author
-------------------------------------------------*/
uint8_t NFCGetOutputPowerVoltage(void)
{
    return OutputPowerVoltage;
}
/*!------------------------------------------------
@brief アンテナ出力電圧設定
@note
@param Voltage 電圧(0x01-0x2A)
@return　なし
@author
-------------------------------------------------*/
bool NFCSetOutputPowerVoltage(uint8_t Voltage)
{
    if( Voltage > NFC_OUTPUT_VOLTAGE_MAX || Voltage == 0 )
    {
        //範囲外
        return false;
    }
    OutputPowerVoltage = Voltage;
    return true;
}
/*!------------------------------------------------
@brief アンテナ出力電流取得
@note
@param なし
@return　電流(1-350)
@author
-------------------------------------------------*/
uint16_t NFCGetOutputPowerCurrent(void)
{
    return OutputPowerCurrent;
}
/*!------------------------------------------------
@brief アンテナ出力電流設定
@note
@param Current 電流(0-350)
@return　なし
@author
-------------------------------------------------*/
bool NFCSetOutputPowerCurrent(uint16_t Current)
{
    if( Current > NFC_OUTPUT_CURRENT_MAX || Current == 0 )
    {
        //範囲外
        return false;
    }

    OutputPowerCurrent = Current;
    return true;
}
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
/*!------------------------------------------------
@brief パラメータ更新指示
@note  更新フラグをONする
@param なし
@return なし
@author
-------------------------------------------------*/
void NFCParameterUpdate(void)
{
    //更新開始
    ParameterUpdateFlag = true;
}
#endif /* NFC_ANTENNA_ADJUSTMENT_COM_ENABLE */
/*!------------------------------------------------
@brief Mifareパラメータ設定
@note  上位より指示されたデータをセットする
@param Block1　　　:読出しブロック1(最初に読み出すブロック位置 0,1,2,3・・・)
@param KeyType1 :読出しブロック1で使用する認証Keyタイプ('0':KeyA or '1':KeyB)
@param pKeyData1:読出しブロック1認証Keyデータ(6Byte)
@param Block2　　　:読出しブロック2(2番目に読み出すブロック位置 0,1,2,3・・・)
@param KeyType2 :読出しブロック2で使用する認証Keyタイプ('0':KeyA or '1':KeyB)
@param pKeyData2:読出しブロック2認証Keyデータ(6Byte)
@return true:データOK、false:データNG
@author
-------------------------------------------------*/
bool NFCSetParameterMifare(uint8_t Block1,  uint8_t KeyType1, uint8_t *pKeyData1,
        uint8_t Block2,  uint8_t KeyType2, uint8_t *pKeyData2)
{
    memset((uint8_t *)&tTempMfc, 0x00, sizeof(tTempMfc));
    //1ブロック目は必須
    if('0' == KeyType1 || '1' == KeyType1)
    {
        tTempMfc.BlockNum[0] = Block1;
        tTempMfc.KeyType[0] = ('0' == KeyType1) ? PHHAL_HW_MFC_KEYA : PHHAL_HW_MFC_KEYB;
        memcpy(tTempMfc.Key, pKeyData1, 6);
        tTempMfc.ReadCount = 1;
    }
    else
    {
        //最初のデータが無効なら2番目のブロック設定はしない
        return false;
    }

    //2ブロック目は必須でないので、データがなくてもOKとする
    if('0' == KeyType2 || '1' == KeyType2)
    {
        tTempMfc.BlockNum[1] = Block2;
        tTempMfc.KeyType[1] = ('0' == KeyType2) ? PHHAL_HW_MFC_KEYA : PHHAL_HW_MFC_KEYB;
        memcpy(&tTempMfc.Key[6], pKeyData2, 6);
        tTempMfc.ReadCount = 2;
    }

    IsSetParameter[PARAM_MIFARE] = true;  //パラメータ設定フラグON

    return true;
}

/*!------------------------------------------------
@brief FeliCaパラメータ初期化
@note  DIPSW1がイニシャル設定の場合に呼び出す
　　　　　　上位からイニシャルコマンド設定がされるまで読込させないため
@return なし
@author
-------------------------------------------------*/
void NFCClearParameterFeliCa(void)
{
    memset((uint8_t *)&tFeliCa, 0x00, sizeof(tFeliCa));
}
/*!------------------------------------------------
@brief Mifareパラメータ初期化
@note  DIPSW1がイニシャル設定の場合に呼び出す
　　　　　　上位からイニシャルコマンド設定がされるまで読込させないため
@return なし
@author
-------------------------------------------------*/
void NFCClearParameterMifare(void)
{
    tMfc.BlockNum[0] = 0;   //読出しブロック Block1
    tMfc.BlockNum[1] = 0;   //読出しブロック Block2
    tMfc.KeyType[0] = PHHAL_HW_MFC_KEYA;    //認証使用Key Block1
    tMfc.KeyType[1] = PHHAL_HW_MFC_KEYA;    //認証使用Key Block2
    memset(tMfc.Key, 0x00, sizeof(tMfc.Key));
    tMfc.ReadCount = 0;         //読出しブロック数
}


/*!------------------------------------------------
@brief FeliCaパラメータ設定
@note  上位より指示されたデータをセットする
@param ValidBlock1　　　:読出しブロック1正常フラグ('V':正常、それ以外：異常)
@param ValidBlock2　　　:読出しブロック2正常フラグ('V':正常、それ以外：異常)
@param pSystemCode :システムコード(2Byte)
@param pServiceCode:サービスコード(2Byte)
@param Block1 :読出しブロック1(最初に読み出すブロック位置 0,1,2,3・・・)
@param Block2 :読出しブロック2(2番目に読み出すブロック位置 0,1,2,3・・・)
@return true:データOK、false:データNG
@author
-------------------------------------------------*/
bool NFCSetParameterFeliCa(uint8_t ValidBlock1 ,uint8_t ValidBlock2,
        uint8_t *pSystemCode, uint8_t *pServiceCode ,uint8_t Block1 ,uint8_t Block2)
{
    if('V' != ValidBlock1 || NULL == pSystemCode || NULL == pServiceCode)
    {
        return false;
    }

    memset((uint8_t *)&tTempFeliCa, 0x00, sizeof(tTempFeliCa));

    //1ブロック目が有効であるため、読込ブロック数=１
    tTempFeliCa.ReadCount = 1;

    //システムコード
    memcpy(pTempFeliCaSystemCode, pSystemCode, 2);

	//読出しブロック位置1
    tTempFeliCa.BlockList[0] = 0x80;
    tTempFeliCa.BlockList[1] = Block1;
    if('V' == ValidBlock2)
    {
        //読出しブロック位置2
        tTempFeliCa.BlockList[2] = 0x80;
        tTempFeliCa.BlockList[3] = Block2;
        //2ブロック目が有効であるため、読込ブロック数=2
        tTempFeliCa.ReadCount = 2;
    }

    //サービスコード
    //どちらが来るか不明のためどちらにも設定する
    uint8_t SCode[2];
    //サービスコードはリトルエンディアンに変換
    SCode[0] = pServiceCode[1];
    SCode[1] = pServiceCode[0];
    memcpy( tTempFeliCa.ServiceCode[FELICA_TYPE_FTSIC], SCode, 2);
    memcpy( tTempFeliCa.ServiceCode[FELICA_TYPE_SURPASS], SCode, 2);

    IsSetParameter[PARAM_FELICA] = true;  //パラメータ設定フラグON

    return true;
}

/*!------------------------------------------------
@brief Mifareデータ読出し
@note  指定されたブロック、キータイプ、キーデータでデータを読み出す
@param pMFC　　　:Mifare読出しライブラリポインタ
@param pUid　　　:UIDデータ格納先頭ポインタ
@param UidSize :UIDサイズ
@param Sak   :Mifare情報(0x08:Mifare 1K / 0x18:Mifare 4K)
@param pNfcData :読出しデータ格納ブロック先頭ポインタ
@return エラーコード
@author
-------------------------------------------------*/
phStatus_t ReadMifare(void *pMFC, uint8_t *pUid, uint8_t UidSize, uint8_t Sak, TYPE_NFC_DATA *pNfcData)
{
	phStatus_t status = PH_ERR_INTERNAL_ERROR;
	uint8_t retry = 0;

	//Mifareパラメータ更新
	if(true == IsSetParameter[PARAM_MIFARE])
    {
        memcpy((uint8_t*)&tMfc, (uint8_t*)&tTempMfc, sizeof(tMfc));

        //Mifare Classic Key設定
        status = phKeyStore_FormatKeyEntry(psKeyStore, tMfc.KeyIndex, PH_KEYSTORE_KEY_TYPE_MIFARE);
        if(status != PH_ERR_SUCCESS)
        {
            return (status & PH_ERR_MASK);
        }
        //Mifare Classic Key設定
        status = phKeyStore_SetKey(
                psKeyStore,
                tMfc.KeyIndex,                  //キーを保存位置
                tMfc.KeyVer,                    //キーバージョン
                PH_KEYSTORE_KEY_TYPE_MIFARE,    //種別
                tMfc.Key,                       //キーデータ
                tMfc.KeyVer);                   //新キーバージョン
        if(status != PH_ERR_SUCCESS)
        {
            return (status & PH_ERR_MASK);
        }

        IsSetParameter[PARAM_MIFARE] = false;
    }

	//2ブロック分読出し
	for(uint8_t i=0; i<tMfc.ReadCount; i++)
	{
	    //Mifare Classic認証処理
	    for(retry=0; retry < MIFARE_RETRY; retry++)
	    {
            status = phalMfc_Authenticate(
                    pMFC,
                    tMfc.BlockNum[i],   //認証対象ブロック
                    tMfc.KeyType[i],    //認証使用キー
                    tMfc.KeyIndex,      //キーを保存した位置
                    tMfc.KeyVer,        //キーバージョン
                    pUid,               //UID
                    UidSize);
            if ((status & PH_ERR_MASK) != PH_ERR_SUCCESS)
            {
                DEBUG_PRINTF("\nAuthentication Failed!!!");
            }
            else
            {
#ifdef NFC_ERROR_DEBUG_RW_M_AUTH1
                if(0==i){status = PH_ERR_INTERNAL_ERROR;}
#elif defined(NFC_ERROR_DEBUG_RW_M_AUTH2)
                if(1==i){status = PH_ERR_INTERNAL_ERROR;}
#else
                //認証成功
                break;
#endif /* NFC_ERROR_DEBUG_RW_M_AUTH1 / NFC_ERROR_DEBUG_RW_M_AUTH2 */
            }
	    }
	    if ((status & PH_ERR_MASK) != PH_ERR_SUCCESS)
	    {
	        //認証失敗
	        NFCDetectError((0 == i ? NFC_ERROR_RW_M_AUTH1 : NFC_ERROR_RW_M_AUTH2));
	        return (status & PH_ERR_MASK);
	    }
	    for(retry=0; retry < MIFARE_RETRY; retry++)
	    {
	        status = phalMfc_Read(psalMFC, tMfc.BlockNum[i], (pNfcData->BlockData + (i * MFC_BLOCK_DATA_SIZE)));
	        if ((status & PH_ERR_MASK) != PH_ERR_SUCCESS)
	        {
	            DEBUG_PRINTF("\nRead operation failed!!!\n");
	        }
	        else
	        {
#ifdef NFC_ERROR_DEBUG_RW_M_READ1
                if(0==i){status = PH_ERR_INTERNAL_ERROR;}
#elif defined(NFC_ERROR_DEBUG_RW_M_READ2)
                if(1==i){status = PH_ERR_INTERNAL_ERROR;}
#else
	            //読出し成功
                //読出しブロック数カウント
                pNfcData->NumBlocks++;
                break;
#endif /* NFC_ERROR_DEBUG_RW_M_READ1 / NFC_ERROR_DEBUG_RW_M_READ2 */
	        }
	    }
	    if((status & PH_ERR_MASK) != PH_ERR_SUCCESS)
        {
            //読出し失敗
	        NFCDetectError((0 == i ? NFC_ERROR_RW_M_READ1 : NFC_ERROR_RW_M_READ2));
            return (status & PH_ERR_MASK);
        }
	}
	if((status & PH_ERR_MASK) == PH_ERR_SUCCESS)
    {
        //読出し成功
	    //読出しデータ種別
	    pNfcData->Type = (0x08 == Sak) ? DEV_TYPE_MIFARE_1K : ((0x18 == Sak) ? DEV_TYPE_MIFARE_4K : DEV_TYPE_MIFARE);
    }
	return (status & PH_ERR_MASK);
}
/*!------------------------------------------------
@brief FeliCaデータ読出し
@note  指定されたブロックのデータを読み出す
@param pFeliCa     :FeliCa読出しライブラリポインタ
@param pNfcData    :読出しデータ格納ブロック先頭ポインタ
@return エラーコード
@author
-------------------------------------------------*/
phStatus_t ReadFeliCa(void *pFeliCa, TYPE_NFC_DATA *pNfcData)
{
    phStatus_t status = PH_ERR_INTERNAL_ERROR;
    uint8_t retry = 0;

    //FeliCaパラメータ更新
    if(true == IsSetParameter[PARAM_FELICA])
    {
        memcpy((uint8_t*)&tFeliCa, (uint8_t*)&tTempFeliCa, sizeof(tFeliCa));
        IsSetParameter[PARAM_FELICA] = false;
    }

    //読取りサービスコード指定用ID(0:FTSic,1:SurPass,2:その他[IDm])
    uint8_t TargetId = IS_FELICA_SP(__TargetCard) ? 1 :
                        IS_FELICA_FTS(__TargetCard) ? 0 : 2;
    if(TargetId < 2)
    {
        (((phpalFelica_Sw_DataParams_t*)((phalFelica_Sw_DataParams_t *)pFeliCa)->pPalFelicaDataParams)->aIDmPMm[0]) |= 0x10;
        for(retry=0; retry < FELICA_RETRY; retry++)
        {
            status = phalFelica_Read(
                    pFeliCa,
                    1,                              //サービス数
                    tFeliCa.ServiceCode[TargetId],  //サービスコード(2バイト)
                    tFeliCa.ReadCount,              //読込ブロック数
                    tFeliCa.BlockList,              //読込対象ブロックリスト
                    (tFeliCa.ReadCount * 2),        //ブロックリストのサイズ
                    &pNfcData->NumBlocks,           //受信したブロック数格納変数
                    pNfcData->BlockData );          //受信したブロックデータ格納変数
            if (status == PH_ERR_SUCCESS)
            {
#ifdef NFC_ERROR_DEBUG_RW_DATA
                status = PH_ERR_INTERNAL_ERROR;
#else
                //読出し成功
                pNfcData->Type = (IS_FELICA_FTS(__TargetCard) ? DEV_TYPE_FELICA_FTS : DEV_TYPE_FELICA_SP);
#endif /* NFC_ERROR_DEBUG_RW_DATA */
                break;
            }
            //FeliCa読出しリトライ時の待ち時間
            phhalHw_Wait(pHal, PHHAL_HW_TIME_MICROSECONDS, NFC_FELICA_RETRY_INTERVAL_US);
        }
        if(status != PH_ERR_SUCCESS)
        {
            //読出し失敗
            //2ブロック一度に読むのでNFC_ERROR_RW_DATA4はなし
            NFCDetectError(NFC_ERROR_RW_DATA);
        }
    }
    return (status & PH_ERR_MASK);
}
/*!------------------------------------------------
@brief NFCデータ読出し
@note  検出したNFCデータを読み出す
@param pDiscLoop   :検出ループ制御用ポインタ
@param pFeliCa     :FeliCa読出しライブラリポインタ
@param pMifare     :Mifare読出しライブラリポインタ
@param TagIndex   　:読出し対象指定インデックス
@param pNfcData    :読出しデータ格納ブロック先頭ポインタ
@return なし
@author
-------------------------------------------------*/
phStatus_t ReadNFC(phacDiscLoop_Sw_DataParams_t * pLoop, void *pFeliCa, void *pMifare, uint8_t TagIndex, TYPE_NFC_DATA *pNfcData)
{
    phStatus_t    status = PH_ERR_READ_WRITE_ERROR;
    uint16_t      wTechDetected = 0;

    //カード読出し向上のためにLED制御へカード検出通知　（検出カード情報取得前に移動）
    NFCDetectedCard(true);

    //検出カード情報取得
    status = phacDiscLoop_GetConfig(pLoop, PHAC_DISCLOOP_CONFIG_TECH_DETECTED, &wTechDetected);
    if (status != PH_ERR_SUCCESS)
    {
        NFCDetectError(NFC_ERROR_RW_SELECT);
    }
    else
    {
        status = PH_ERR_READ_WRITE_ERROR;

        //カード読出し向上のためにLED制御へカード検出通知
//        NFCDetectedCard(true);

        if (PHAC_DISCLOOP_CHECK_ANDMASK(wTechDetected, PHAC_DISCLOOP_POS_BIT_MASK_A) &&
                (pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].aSak & (uint8_t) ~0xFB) == 0)
        {
            //----------------------------
            //Mifareカード読出し
            //----------------------------
            memset((uint8_t*)pNfcData,0x00,sizeof(TYPE_NFC_DATA));

            if( IS_MIFARE(__TargetCard) )
            {
                uint8_t bTagType = (pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].aSak & 0x60);
                bTagType = bTagType >> 5;
                if(bTagType == PHAC_DISCLOOP_TYPEA_TYPE2_TAG_CONFIG_MASK)
                {
                    //UID
                    memcpy(pNfcData->UidIdm, pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].aUid, pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].bUidSize);
                    pNfcData->UidIdmLength = pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].bUidSize;
                    status = ReadMifare(
                            pMifare,
                            pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].aUid,
                            pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].bUidSize,
                            pLoop->sTypeATargetInfo.aTypeA_I3P3[TagIndex].aSak,
                            pNfcData
                            );
                    if (status != PH_ERR_SUCCESS)
                    {
                        //読込は失敗したがMifareであるためDEV_TYPE_MIFAREを設定
                        pNfcData->Type = DEV_TYPE_MIFARE;
                        //Mifareは1ブロックのみ読出しできている可能性があるため読出しブロック数は0にしない
                        //ブロック読出しの成功失敗にかかわらず通知するため、読出し成功とする
                        status = PH_ERR_SUCCESS;
                    }
                }
            }
        }
        else if( PHAC_DISCLOOP_CHECK_ANDMASK(wTechDetected, PHAC_DISCLOOP_POS_BIT_MASK_F212) ||
                 PHAC_DISCLOOP_CHECK_ANDMASK(wTechDetected, PHAC_DISCLOOP_POS_BIT_MASK_F424))
        {
            //----------------------------
            //FeliCaカード読出し
            //----------------------------
            memset((uint8_t*)pNfcData,0x00,sizeof(TYPE_NFC_DATA));

            if( IS_FELICA(__TargetCard) )
            {
                //IDM
                memcpy(pNfcData->UidIdm, pLoop->sTypeFTargetInfo.aTypeFTag[TagIndex].aIDmPMm, PHAC_DISCLOOP_FELICA_IDM_LENGTH);
                pNfcData->UidIdmLength = PHAC_DISCLOOP_FELICA_IDM_LENGTH;
                status = ReadFeliCa(pFeliCa, pNfcData);
                if (status != PH_ERR_SUCCESS)
                {
                    //読込は失敗したがFeliCaであるためDEV_TYPE_FELICAを設定
                    pNfcData->Type = DEV_TYPE_FELICA;
                    //読出し失敗のため読出しブロック数は0
                    pNfcData->NumBlocks = 0;
                    //ブロック読出しの成功失敗にかかわらず通知するため、読出し成功とする
                    status = PH_ERR_SUCCESS;
                }
            }
        }
    }

    return status;
}
/*!------------------------------------------------
@brief NFC初期化
@note  基本的にNFCで使用するハードの初期化
@param なし
@return true:成功、false:失敗
@author
-------------------------------------------------*/
bool InitNFC(void)
{
    fsp_err_t err = FSP_SUCCESS;
    phStatus_t status = PH_ERR_INTERNAL_ERROR;

    err = R_ICU_ExternalIrqOpen(&g_external_irq1_ctrl, &g_external_irq1_cfg);
    if (FSP_SUCCESS != err)
    {
        return false;
    }
    err = R_ICU_ExternalIrqEnable(&g_external_irq1_ctrl);
    if (FSP_SUCCESS != err)
    {
        return false;
    }

    err = R_GPT_Open(&g_timer10_ctrl,&g_timer10_cfg);
    if (FSP_SUCCESS != err)
    {
        return false;
    }

    status = phOsal_Init();
    if(status != PH_OSAL_SUCCESS)
    {
        return false;
    }

    status = phbalReg_Init(&sBalParams, sizeof(phbalReg_Type_t));
    //CHECK_STATUS(status);
    if(status != PH_ERR_SUCCESS)
    {
        return false;
    }

    status = phApp_Configure_IRQ();
    //CHECK_STATUS(status);
    if(status != PH_ERR_SUCCESS)
    {
        return false;
    }

    return true;
}

/*!------------------------------------------------
@brief NFC処理
@note
@param なし
@return エラーコード
@author
-------------------------------------------------*/
uint16_t DoNFC(void)
{
    phStatus_t status = PH_ERR_INTERNAL_ERROR;
    phNfcLib_Status_t     dwStatus;

    phNfcLib_AppContext_t AppContext = {0};
    __NFCError = 0;

    AppContext.pBalDataparams = &sBalParams;
    dwStatus = phNfcLib_SetContext(&AppContext);
    if(dwStatus != PH_NFCLIB_STATUS_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        status = PH_ERR_INTERNAL_ERROR;
        goto DoNFC_Error;
    }

    /* Initialize library */
    dwStatus = phNfcLib_Init();
    if(dwStatus != PH_NFCLIB_STATUS_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        status = PH_ERR_INTERNAL_ERROR;
        goto DoNFC_Error;
    }

    /* Set the generic pointer */
    pHal = phNfcLib_GetDataParams(PH_COMP_HAL);
    pDiscLoop = phNfcLib_GetDataParams(PH_COMP_AC_DISCLOOP);
    //認証キー用
    psKeyStore = phNfcLib_GetDataParams(PH_COMP_KEYSTORE);
    //Mifare用
    psalMFC = phNfcLib_GetDataParams(PH_COMP_AL_MFC);
    //FeliCa用
    psalFeliCa = phNfcLib_GetDataParams(PH_COMP_AL_FELICA);

    /* Initialize other components that are not initialized by NFCLIB and configure Discovery Loop. */
    status = phApp_Comp_Init(pDiscLoop);
    if(status != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        goto DoNFC_Error;
    }

    //認証キー処理初期化
    status = phKeyStore_Sw_Init(
        psKeyStore,
        sizeof(phKeyStore_Sw_DataParams_t),
        &sKeyEntries[0],
        NUMBER_OF_KEYENTRIES,
        &sKeyVersionPairs[0],
        NUMBER_OF_KEYVERSIONPAIRS,
        &sKUCEntries[0],
        NUMBER_OF_KUCENTRIES
        );
    if(status != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        goto DoNFC_Error;
    }

    while(1)
    {
        DEBUG_PRINTF ("DoNFC\n");

        /* load a Key to the Store */
        /* Note: If You use Key number 0x00, be aware that in SAM
          this Key is the 'Host authentication key' !!! */

        //Mifare Classic Key設定
        status = phKeyStore_FormatKeyEntry(psKeyStore, tMfc.KeyIndex, PH_KEYSTORE_KEY_TYPE_MIFARE);
        if(status != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_INIT;
            break;
        }
        //Mifare Classic Key設定
        status = phKeyStore_SetKey(
                psKeyStore,
                tMfc.KeyIndex,                  //キーを保存位置
                tMfc.KeyVer,                    //キーバージョン
                PH_KEYSTORE_KEY_TYPE_MIFARE,    //種別
                tMfc.Key,                       //キーデータ
                tMfc.KeyVer);                   //新キーバージョン
        if(status != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_INIT;
            break;
        }

        //通常動作時はこの関数を抜けない。
        //エラー発生、またはパラメータ更新時に抜けて再初期化
        status = DiscoveryLoop(pDiscLoop);
        if(status != PH_ERR_SUCCESS)
        {
            //DiscoveryLoop内部で__NFCErrorにエラーをセットする為
            //ここでは__NFCError変数にセットしない。
            break;
        }
    }
DoNFC_Error:
    phNfcLib_DeInit();
    //エラーが発生した場合は各タスクが実行不可のためここで呼び出し
    NFCDoTask();
    NFCDetectError(__NFCError);
    return status;
}


/*!------------------------------------------------
@brief NFC検出処理
@note　通常はこの関数は無限ループなっている。異常検出時やパラメータ更新時にのみループから出る
@param pDataParams DiscoveryLoopパラメータ
@return エラーコード
@author
-------------------------------------------------*/
phStatus_t DiscoveryLoop(void  *pDataParams)
{
    phStatus_t    status, statustmp;
    uint16_t      wEntryPoint;
    uint8_t res = 0;
#ifdef NFC_SWITCHING_TARGET_SYNC_P609
        bsp_io_level_t pindata = 0;
#endif

    //アンテナ出力設定
    res = phApp_SetOutputPowerVoltage(OutputPowerVoltage);
    if (res > 0 )
    {
        if ( 2 == res ){__NFCError = NFC_ERROR_RW_INIT;}
        return PH_ERR_UNKNOWN;
    }
    res = phApp_SetOutputPowerCurrent(OutputPowerCurrent);
    if (res > 0 )
    {
        if ( 2 == res ){__NFCError = NFC_ERROR_RW_INIT;}
        return PH_ERR_UNKNOWN;
    }

    // FW Version 取得
    __NfcFwVersion = phApp_GetPn5910FwVersion();

    //Discovery loopプロファイル設定 PHAC_DISCLOOP_PROFILE_NFCのみ対象
    status = LoadProfile();
    if (status != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        return status;
    }

#ifdef NXPBUILD__PHHAL_HW_TARGET
    /* Initialize the setting for Listen Mode */
    status = phApp_HALConfigAutoColl();
    if (status != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        return status;
    }
#endif /* NXPBUILD__PHHAL_HW_TARGET */

    /* Get Poll Configuration */
    status = phacDiscLoop_GetConfig(pDataParams, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG, &bSavePollTechCfg);
    if (status != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        return status;
    }

    /* Start in poll mode */
    wEntryPoint = PHAC_DISCLOOP_ENTRY_POINT_POLL;
    status = PHAC_DISCLOOP_LPCD_NO_TECH_DETECTED;

    /* Switch off RF field */
    statustmp = phhalHw_FieldOff(pHal);
#ifdef NFC_ERROR_DEBUG_RW_INIT
    statustmp = PH_ERR_INTERNAL_ERROR;
#endif  /* NFC_ERROR_DEBUG_RW_INIT */
    if (statustmp != PH_ERR_SUCCESS)
    {
        __NFCError = NFC_ERROR_RW_INIT;
        return statustmp;
    }

    //NFC初期化完了通知
    NFCInitComlpete();
    SendInterval = SEND_INTERVAL;

    while(1)
    {
        DEBUG_PRINTF ("\nDiscoveryLoop:%d　\n",wEntryPoint);

        /* Before polling set Discovery Poll State to Detection , as later in the code it can be changed to e.g. PHAC_DISCLOOP_POLL_STATE_REMOVAL*/
        statustmp = phacDiscLoop_SetConfig(pDataParams, PHAC_DISCLOOP_CONFIG_NEXT_POLL_STATE, PHAC_DISCLOOP_POLL_STATE_DETECTION);
#ifdef NFC_ERROR_DEBUG_RW_POL
        statustmp = PH_ERR_INTERNAL_ERROR;
#endif /* NFC_ERROR_DEBUG_RW_POL */
        if (statustmp != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_POL;
            return statustmp;
        }

#if !defined(ENABLE_EMVCO_PROF) && defined(PH_EXAMPLE1_LPCD_ENABLE)
        /* Configure LPCD */
        if ((status & PH_ERR_MASK) == PHAC_DISCLOOP_LPCD_NO_TECH_DETECTED)
        {
            status = phApp_ConfigureLPCD();
            if (status != PH_ERR_SUCCESS)
            {
                __NFCError = NFC_ERROR_RW_POL;
                return status;
            }
        }

        /* Bool to enable LPCD feature. */
        status = phacDiscLoop_SetConfig(pDataParams, PHAC_DISCLOOP_CONFIG_ENABLE_LPCD, PH_ON);
#ifdef NFC_ERROR_DEBUG_RW_POL
        status = PH_ERR_INTERNAL_ERROR;
#endif /* NFC_ERROR_DEBUG_RW_POL */
        if (status != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_POL;
            return status;
        }
#endif /* PH_EXAMPLE1_LPCD_ENABLE*/

        /* Start discovery loop */
        status = phacDiscLoop_Run(pDataParams, (uint8_t)wEntryPoint);

        wEntryPoint = NFCForumProcess(wEntryPoint, status);
#if 1
        if(wEntryPoint != PHAC_DISCLOOP_ENTRY_POINT_POLL)
        {
            if( IS_MIFARE(__TargetCard) && IS_FELICA(__TargetCard) )
            {
                //Mifare,FeliCa両方読出しの場合は交互に指定
                if( TargetTechnology < FELICA_TYPE_MAX )
                {
                    bSavePollTechCfg = Technology[TargetTechnology];
#ifdef NFC_SWITCHING_TARGET_SYNC_P609
					//読取対象MifareはHIGH、FelicaはLOWに出力
                    if(TargetTechnology==TECH_IDX_MIFARE)
                    {
                        //Mifare_P609_LOW
                        R_IOPORT_PinWrite(&g_ioport_ctrl,BSP_IO_PORT_06_PIN_09, BSP_IO_LEVEL_LOW);
                    }
                    else
                    {
                        //felica_P609_HIGH
                        R_IOPORT_PinWrite(&g_ioport_ctrl,BSP_IO_PORT_06_PIN_09, BSP_IO_LEVEL_HIGH);
                    }
#endif
                }
                TargetTechnology++;

                if( TargetTechnology >= FELICA_TYPE_MAX )
                {
                    TargetTechnology = TECH_IDX_MIFARE;
                }
            }
            else
            {
                //Mifare or FeliCaの場合は読出し種別固定
                if( IS_MIFARE(__TargetCard) )
                {
                    bSavePollTechCfg = Technology[TECH_IDX_MIFARE];
#ifdef NFC_SWITCHING_TARGET_SYNC_P609
                    //P609を反転する
                    R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_09, &pindata);
                    R_IOPORT_PinWrite(&g_ioport_ctrl,BSP_IO_PORT_06_PIN_09, !pindata);
#endif
                }
                else if( IS_FELICA(__TargetCard) )
                {
                    bSavePollTechCfg = Technology[TECH_IDX_FELICA];
#ifdef NFC_SWITCHING_TARGET_SYNC_P609
                    //P609を反転する
                    R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_09, &pindata);
                    R_IOPORT_PinWrite(&g_ioport_ctrl,BSP_IO_PORT_06_PIN_09, !pindata);
#endif
                }
            }
        }
#endif
        /* Set Poll Configuration */
        statustmp = phacDiscLoop_SetConfig(pDataParams, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG, bSavePollTechCfg);
        if (statustmp != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_POL3;
            return statustmp;
        }

        /* Switch off RF field */
        statustmp = phhalHw_FieldOff(pHal);
        if (statustmp != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_POL3;
            return statustmp;
        }

        /* Wait for field-off time-out */
        statustmp = phhalHw_Wait(pHal, PHHAL_HW_TIME_MICROSECONDS, 5100);
#ifdef NFC_ERROR_DEBUG_RW_POL3
        statustmp = PH_ERR_INTERNAL_ERROR;
#endif /* NFC_ERROR_DEBUG_RW_POL3 */
        if (statustmp != PH_ERR_SUCCESS)
        {
            __NFCError = NFC_ERROR_RW_POL3;
            return statustmp;
        }
#ifdef NFC_ANTENNA_ADJUSTMENT_COM_ENABLE
        //パラメータ更新フラグ検出
        if(true == ParameterUpdateFlag)
        {
            ParameterUpdateFlag = false;
            //再初期化のためループを抜ける
            return PH_ERR_SUCCESS;
        }
#endif /* NFC_ANTENNA_ADJUSTMENT_COM_ENABLE */
        if(wEntryPoint != PHAC_DISCLOOP_ENTRY_POINT_POLL)
        {
            //Mifare、FeliCaが読込対象になっている場合、Mifare、FeliCa切り替え時にウェイトを入れる
            if( IS_MIFARE(__TargetCard) && IS_FELICA(__TargetCard) )
            {
                phhalHw_Wait(pHal, PHHAL_HW_TIME_MICROSECONDS, NFC_MIFARE_FELICA_RFOFF_INTERVAL_US);
            }
        }
    }
}

/*!------------------------------------------------
@brief NFC検出後処理
@note
@param wEntryPoint 検出状態
@param DiscLoopStatus DiscoveryLoop状態
@return エラーコード
@author
-------------------------------------------------*/
uint16_t NFCForumProcess(uint16_t wEntryPoint, phStatus_t DiscLoopStatus)
{
    phStatus_t    status;
    uint16_t      wTechDetected = 0;
    uint16_t      wNumberOfTags = 0;
    uint16_t      wValue;
    uint8_t       bIndex;
    uint16_t      wReturnEntryPoint;

    if(wEntryPoint == PHAC_DISCLOOP_ENTRY_POINT_POLL)
    {
#if 0
        if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_MULTI_TECH_DETECTED)
        {
            /////////////////////////////
            /// 複数種類カード検出 -> 読込対象を1つに限定
            /////////////////////////////
            DEBUG_PRINTF (" \n Multiple technology detected: \n");

            status = phacDiscLoop_GetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_TECH_DETECTED, &wTechDetected);
            if (status != PH_ERR_SUCCESS)
            {
                NFCDetectError(NFC_ERROR_RW_SELECT);
            }
            else
            {
                //複数検出したデバイス種別から読取り対象種別を決定する(Mifare or FeliCa)
                for(uint8_t i=0;i<TECH_IDX_MAX;i++)
                {
                    //検出した種別が対象カード、かつ前回指定した種別と異なる場合に読取り対象とする
                    if( (wTechDetected & Technology[i]) && (TargetTechnology != Technology[i]) )
                    {
                        TargetTechnology = Technology[i];
                        break;
                    }
                }
                //読取り対象種別を指定
                status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG, TargetTechnology);
                if (status != PH_ERR_SUCCESS)
                {
                    NFCDetectError(NFC_ERROR_RW_SELECT);
                }
            }
            //複数衝突解決状態に設定
            status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_NEXT_POLL_STATE, PHAC_DISCLOOP_POLL_STATE_COLLISION_RESOLUTION);
            if (status != PH_ERR_SUCCESS)
            {
                NFCDetectError(NFC_ERROR_RW_SELECT);
            }

            // 衝突解決状態で検出処理
            DiscLoopStatus = phacDiscLoop_Run(pDiscLoop, (uint8_t)wEntryPoint);
        }
        else
        {
            //検出種別が1種類の場合はターゲット変数初期化
            TargetTechnology = 0;
        }
#endif
        if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_MULTI_DEVICES_RESOLVED)
        {
            /////////////////////////////
            /// 同種複数カード検出 -> 複数カードを読出し
            /////////////////////////////

            //検出カード情報取得(複数種類カード検出解決後であるため検出種類は必ず1種類 Mifare or FeliCa)
            status = phacDiscLoop_GetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_TECH_DETECTED, &wTechDetected);
            if (status != PH_ERR_SUCCESS)
            {
                NFCDetectError(NFC_ERROR_RW_SELECT);
            }
            else
            {
                //検出カード種別からアクティブ化に必要なインデックスを取得
                for(bIndex = 0; bIndex < PHAC_DISCLOOP_PASS_POLL_MAX_TECHS_SUPPORTED; bIndex++)
                {
                    if((1 << bIndex) & (uint8_t)wTechDetected) {break;}
                }
                //検出カード数取得(同種類で複数カード検出)
                status = phacDiscLoop_GetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_NR_TAGS_FOUND, &wNumberOfTags);
#ifdef NFC_ERROR_DEBUG_RW_SELECT
                status = PH_ERR_INTERNAL_ERROR;
#endif /* NFC_ERROR_DEBUG_RW_SELECT */
                if (status != PH_ERR_SUCCESS)
                {
                    NFCDetectError(NFC_ERROR_RW_SELECT);
                }
                else
                {
                    //検出した枚数全て読出し
                    for( uint8_t tagIndex=0; tagIndex < wNumberOfTags; tagIndex++ )
                    {
                        //検出したカードをアクティブ化
                        status = phacDiscLoop_ActivateCard(pDiscLoop, bIndex, tagIndex);
                        if(((status & PH_ERR_MASK) == PHAC_DISCLOOP_DEVICE_ACTIVATED) ||
                            ((status & PH_ERR_MASK) == PHAC_DISCLOOP_PASSIVE_TARGET_ACTIVATED))
                        {
                            //アクティブ化したカード情報読出し
                            status = ReadNFC(pDiscLoop, psalFeliCa, psalMFC, tagIndex, &tNfcData);
                            if (status == PH_ERR_SUCCESS)
                            {
                                //読み出した情報はNfcReaderに通知
                                if(SendInterval > 0)
                                {
                                    SendInterval--;
                                }
                                else
                                {
                                    NFCNoticeCardData(&tNfcData);
                                    SendInterval = SEND_INTERVAL;
                                }
                            }
                        }
                    }
                }
            }
            /* Switch to LISTEN mode after POLL mode */
        }
        else if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_DEVICE_ACTIVATED)
        {
            /////////////////////////////
            /// 単独カード検出、アクティブ化成功 -> 単独カードを読出し
            /////////////////////////////

            //情報読出し
            status = ReadNFC(pDiscLoop, psalFeliCa, psalMFC, 0, &tNfcData);
            if (status == PH_ERR_SUCCESS)
            {
                //読み出した情報はNfcReaderに通知
                if(SendInterval > 0)
                {
                    SendInterval--;
                }
                else
                {
                    NFCNoticeCardData(&tNfcData);
                    SendInterval = SEND_INTERVAL;
                }

            }

            /* Switch to LISTEN mode after POLL mode */
        }
        else
        {
            if(SendInterval > 0){SendInterval--;}
#ifndef NFC_ERROR_DEBUG_RW_POL2
            //その他エラー
            if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_FAILURE)
            {
                status = phacDiscLoop_GetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_ADDITIONAL_INFO, &wValue);
                if (status != PH_ERR_SUCCESS)
                {
                    //NFCDetectError(NFC_ERROR_RW_POL2);
                }
                DEBUG_ERROR_PRINT(PrintErrorInfo(wValue));
            }
            else
#endif /* NFC_ERROR_DEBUG_RW_POL2 */
            {
                //NFCDetectError(NFC_ERROR_RW_POL2);
                DEBUG_ERROR_PRINT(PrintErrorInfo(status));
            }
        }

        /* Update the Entry point to LISTEN mode. */
        wReturnEntryPoint = PHAC_DISCLOOP_ENTRY_POINT_LISTEN;

    }
    else
    {
        DEBUG_PRINTF (" \n LISTEN  Mode \n");
#ifdef NFC_ERROR_DEBUG_RW_RELEASE
        DiscLoopStatus = PHAC_DISCLOOP_EXTERNAL_RFOFF;
#endif /* NFC_ERROR_DEBUG_RW_RELEASE */
        if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_EXTERNAL_RFOFF)
        {
            /*
             * Enters here if in the target/card mode and external RF is not available
             * Wait for LISTEN timeout till an external RF is detected.
             * Application may choose to go into standby at this point.
             */
            status = phhalHw_EventConsume(pHal);
#ifdef NFC_ERROR_DEBUG_RW_RELEASE
            status = PH_ERR_INTERNAL_ERROR;
#endif /* NFC_ERROR_DEBUG_RW_RELEASE */
            if (status != PH_ERR_SUCCESS)
            {
                NFCDetectError(NFC_ERROR_RW_RELEASE);
            }

            status = phhalHw_SetConfig(pHal, PHHAL_HW_CONFIG_RFON_INTERRUPT, PH_ON);
            if (status != PH_ERR_SUCCESS)
            {
                NFCDetectError(NFC_ERROR_RW_RELEASE);
            }

            status = phhalHw_EventWait(pHal, LISTEN_PHASE_TIME_MS);
            if((status & PH_ERR_MASK) == PH_ERR_IO_TIMEOUT)
            {
                wReturnEntryPoint = PHAC_DISCLOOP_ENTRY_POINT_POLL;
            }
            else
            {
                wReturnEntryPoint = PHAC_DISCLOOP_ENTRY_POINT_LISTEN;
            }
        }
        else
        {
            if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_ACTIVATED_BY_PEER)
            {
                DEBUG_PRINTF (" \n Device activated in listen mode... \n");
            }
            else if ((DiscLoopStatus & PH_ERR_MASK) == PH_ERR_INVALID_PARAMETER)
            {
                /* In case of Front end used is RC663, then listen mode is not supported.
                 * Switch from listen mode to poll mode. */
            }
            else
            {
                if((DiscLoopStatus & PH_ERR_MASK) == PHAC_DISCLOOP_FAILURE)
                {
                    status = phacDiscLoop_GetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_ADDITIONAL_INFO, &wValue);
                    CHECK_STATUS(status);
                    DEBUG_ERROR_PRINT(PrintErrorInfo(wValue));
                }
                else
                {
                    DEBUG_ERROR_PRINT(PrintErrorInfo(status));
                }
            }

            /* On successful activated by Peer, switch to LISTEN mode */
            wReturnEntryPoint = PHAC_DISCLOOP_ENTRY_POINT_POLL;
        }
    }
    return wReturnEntryPoint;
}

#ifdef ENABLE_DISC_CONFIG
/**
* This function will load/configure Discovery loop with default values based on interested profile
 * Application can read these values from EEPROM area and load/configure Discovery loop via SetConfig
* \param   bProfile      Reader Library Profile
* \note    Values used below are default and is for demonstration purpose.
*/
static phStatus_t LoadProfile(void)
{
    phStatus_t status = PH_ERR_SUCCESS;
    uint16_t   wPasPollConfig = 0;
    uint16_t   wActPollConfig = 0;
    uint16_t   wPasLisConfig = 0;
    uint16_t   wActLisConfig = 0;

#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEA_TAGS
    //wPasPollConfig |= PHAC_DISCLOOP_POS_BIT_MASK_A;
#endif

#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF_TAGS
    //wPasPollConfig |= (PHAC_DISCLOOP_POS_BIT_MASK_F212 | PHAC_DISCLOOP_POS_BIT_MASK_F424);
#endif

#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEA_TARGET_PASSIVE
    wPasLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_A;
#endif
#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF212_TARGET_PASSIVE
    wPasLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_F212;
#endif
#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF424_TARGET_PASSIVE
    wPasLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_F424;
#endif

#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEA_TARGET_ACTIVE
    wActLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_A;
#endif
#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF212_TARGET_ACTIVE
    wActLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_F212;
#endif
#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF424_TARGET_ACTIVE
    wActLisConfig |= PHAC_DISCLOOP_POS_BIT_MASK_F424;
#endif

    /* passive Bailout bitmap config. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_BAIL_OUT, 0x00);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Set Passive poll bitmap config. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG, wPasPollConfig);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Set Active poll bitmap config. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_ACT_POLL_TECH_CFG, wActPollConfig);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Set Passive listen bitmap config. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_PAS_LIS_TECH_CFG, wPasLisConfig);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Set Active listen bitmap config. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_ACT_LIS_TECH_CFG, wActLisConfig);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* reset collision Pending */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_COLLISION_PENDING, PH_OFF);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* whether anti-collision is supported or not. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_ANTI_COLL, PH_ON);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Poll Mode default state*/
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_NEXT_POLL_STATE, PHAC_DISCLOOP_POLL_STATE_DETECTION);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

#ifdef  NXPBUILD__PHAC_DISCLOOP_TYPEA_TAGS
    /* Device limit for Type A */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_TYPEA_DEVICE_LIMIT, 3); //Mifare 最大検出3枚まで
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Passive polling Tx Guard times in micro seconds. */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_GTA_VALUE_US, 5100);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }
#endif

#ifdef NXPBUILD__PHAC_DISCLOOP_TYPEF_TAGS
    /* Device limit for Type F */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_TYPEF_DEVICE_LIMIT, 3); //FeliCa最大検出3枚まで
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Guard time for Type F. This guard time is applied when Type F poll before Type B */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_GTFB_VALUE_US, 20400);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }

    /* Guard time for Type F. This guard time is applied when Type B poll before Type F */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_GTBF_VALUE_US, 15300);
    if (status != PH_ERR_SUCCESS)
    {
        return status;
    }
#endif

    /* Discovery loop Operation mode */
    status = phacDiscLoop_SetConfig(pDiscLoop, PHAC_DISCLOOP_CONFIG_OPE_MODE, RD_LIB_MODE_NFC);
    //CHECK_STATUS(status);
    return status;
}
#endif /* ENABLE_DISC_CONFIG */
