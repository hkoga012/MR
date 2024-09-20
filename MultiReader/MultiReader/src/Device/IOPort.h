/*!------------------------------------------------
@brief IOPortクラスヘッダ
@note　
@author
-------------------------------------------------*/

#ifndef IO_PORT_H
#define IO_PORT_H
/*
enum ENUM_PORT {
	PORT_NONE = -1,
	PORT_SW1_1 = BSP_IO_PORT_00_PIN_00,
	PORT_SW1_2 = BSP_IO_PORT_00_PIN_01,
	PORT_SW1_3 = BSP_IO_PORT_00_PIN_02,
	PORT_SW1_4 = BSP_IO_PORT_00_PIN_03,
	PORT_SW1_5 = BSP_IO_PORT_00_PIN_04,
	PORT_SW1_6 = BSP_IO_PORT_00_PIN_05,
	PORT_SW1_7 = BSP_IO_PORT_00_PIN_06,
	PORT_SW1_8 = BSP_IO_PORT_00_PIN_07,
	PORT_SW2_1 = BSP_IO_PORT_04_PIN_00,
	PORT_SW2_2 = BSP_IO_PORT_04_PIN_01,
	PORT_SW2_3 = BSP_IO_PORT_04_PIN_02,
	PORT_SW2_4 = BSP_IO_PORT_04_PIN_03,
	PORT_SW2_5 = BSP_IO_PORT_04_PIN_04,
	PORT_SW2_6 = BSP_IO_PORT_04_PIN_05,
	PORT_SW2_7 = BSP_IO_PORT_04_PIN_06,
	PORT_SW2_8 = BSP_IO_PORT_04_PIN_07,
	PORT_P011 = BSP_IO_PORT_00_PIN_11,
	PORT_P010 = BSP_IO_PORT_00_PIN_10,
	PORT_P008 = BSP_IO_PORT_00_PIN_08,
	PORT_P012 = BSP_IO_PORT_00_PIN_12,
	PORT_P013 = BSP_IO_PORT_00_PIN_13,
	PORT_P014 = BSP_IO_PORT_00_PIN_14,
	PORT_P015 = BSP_IO_PORT_00_PIN_15,
	PORT_485RE = BSP_IO_PORT_04_PIN_12,
	PORT_485DE = BSP_IO_PORT_04_PIN_13,
	PORT_485PO = BSP_IO_PORT_04_PIN_14,
	PORT_LED_Left_R = BSP_IO_PORT_01_PIN_00,
	PORT_LED_Left_B = BSP_IO_PORT_01_PIN_02,
	PORT_LED_Center_R = BSP_IO_PORT_01_PIN_04,
	PORT_LED_Center_B = BSP_IO_PORT_01_PIN_06,
	PORT_LED_Right_R = BSP_IO_PORT_06_PIN_00,
	PORT_LED_Right_B = BSP_IO_PORT_06_PIN_02,
	PORT_Buzzer = BSP_IO_PORT_01_PIN_15,
	PORT_AutoDoor = BSP_IO_PORT_06_PIN_08,
	PORT_SPI_SS = BSP_IO_PORT_02_PIN_01,
	PORT_SPI_SSI_0 = BSP_IO_PORT_03_PIN_07,
	PORT_SPI_SSI_1 = BSP_IO_PORT_03_PIN_06,
	PORT_IRQ_OUT = BSP_IO_PORT_02_PIN_05,
};
*/
class IOPort {
	uart_ctrl_t  *_pIoPortCtrl;
public:
    /*!------------------------------------------------
    @brief コンストラクタ
    -------------------------------------------------*/
    IOPort(uart_ctrl_t  *);

    /*!------------------------------------------------
    @brief デストラクタ
    -------------------------------------------------*/
    ~IOPort() = default;
    
    /*!------------------------------------------------
	@brief 初期化
	-------------------------------------------------*/
	TYPE_ERROR Init(void);

	/*!------------------------------------------------
	@brief ポート出力
	-------------------------------------------------*/
	TYPE_ERROR Write(bsp_io_port_pin_t , bsp_io_level_t );

	/*!------------------------------------------------
	@brief ポートOFF
	-------------------------------------------------*/
	TYPE_ERROR Read(bsp_io_port_pin_t , bsp_io_level_t *);
	
};
#endif

