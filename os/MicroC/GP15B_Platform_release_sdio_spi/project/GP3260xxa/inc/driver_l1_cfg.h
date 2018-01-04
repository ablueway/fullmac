#ifndef __DRIVER_L1_CFG_H__
#define __DRIVER_L1_CFG_H__

	#define INIT_MCLK                   144000000//48000000
	#define INIT_MHZ                    (INIT_MCLK/1000000)

	#define SDRAM_START_ADDR            0x00000000 
	#define SDRAM_END_ADDR              0x7FFFFF//0x01FFFFFF 

	#define ISRAM_START_ADDR            0xF8000000
	#define ISRAM_END_ADDR              0xF8037FFF//224KB  

	// MCU configuration
	#define _DRV_L1_ADC                 1
	#define _DRV_L1_CACHE               1
	#define _DRV_L1_DAC                 1
	#define _DRV_L1_DMA                 1
	#define _DRV_L1_EXT_INT             1
	#define _DRV_L1_GPIO                1
	#define _DRV_L1_GTE                 1
	#define _DRV_L1_INTERRUPT           1
	#define _DRV_L1_JPEG                1
	#define _DRV_L1_MEMC                1
	#define _DRV_L1_RTC                 1
	#define _DRV_L1_SCALER              1
	#define _DRV_L1_SDC                 1
	#define _DRV_L1_SENSOR              1
	#define _DRV_L1_SPI                 1
	#define _DRV_L1_SPIFC               1
	#define _DRV_L1_SW_TIMER            1
	#if _DRV_L1_SW_TIMER==1
		#define DRV_L1_SW_TIMER_SOURCE  	TIMER_RTC
	    #define DRV_L1_SW_TIMER_Hz      	1024
	#endif
	#define _DRV_L1_SYSTEM              1
	#define _DRV_L1_TFT                 1
	#define _DRV_L1_TIMER               1
	#define _DRV_L1_UART0               1
	#define _DRV_L1_UART1				1
	#define _DRV_L1_USBH_UVC            1
	#define _DRV_L1_USBH                1
	#define _DRV_L1_USBD                1
	#define _DRV_L1_WATCHDOG            1
	#define _DRV_L1_XDC                 0

	#define _DRV_L1_SPU                 1
	#define _DRV_L1_PPU                 1
	#define _DRV_L1_DEFLICKER			1
	#define _DRV_L1_NOR                 0
	#define _DRV_L1_NAND                1
	#define _DRV_L1_CFC                 1
	#define _DRV_L1_MSC                 1
	#define _DRV_L1_KEYSCAN             1
	#define _DRV_L1_TV                  1
	#define _DRV_L1_I2S                 1
	#define _DRV_L1_MIPI				1
	#define _DRV_L1_CDSP				1
	#define	_DRV_L1_HDMI				1
	#define _DRV_L1_CONV420TO422		1
	#define _DRV_L1_CONV422TO420		1
	#define _DRV_L1_WRAP				1

	// UART interface config
	#define UART0_BAUD_RATE             115200

	#define SD_DUAL_SUPPORT				1
#endif      // __DRIVER_L1_CFG_H__
