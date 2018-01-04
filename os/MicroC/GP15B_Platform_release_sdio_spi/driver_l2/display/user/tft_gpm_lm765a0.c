#include "drv_l1_sfr.h"
#include "drv_l1_tft.h"
#include "drv_l1_gpio.h"
#include "drv_l1_timer.h"
#include "drv_l2_display.h"

#if (defined GPM_LM765A0) && (GPM_LM765A0 == 1)

#define USE_160_128		1
#define USE_162_132		2
#define TFT_RESOLUTION	USE_160_128

INT32S tft_gpm_lm765a0_init(void)
{	
	drv_l1_tft_mem_unit_set(8);
	drv_l1_tft_clk_set(TFT_CLK_DIVIDE_8);
	R_TFT_LINE_RGB_ORDER |= 0x0080; //memory mode
                        
#if 0	//hw reset 
	gpio_init_io(IO_C3, GPIO_OUTPUT);	
	gpio_set_port_attribute(IO_C3, ATTRIBUTE_HIGH);
    gpio_write_io(IO_C3, DATA_LOW);
    drv_msec_wait(200);
    drv_msec_wait(200);
    gpio_write_io(IO_C3, DATA_HIGH);
    drv_msec_wait(200);	
#endif

#if 0 //match frame size 
	R_TFT_V_PERIOD = 240-1;	//240  
    R_TFT_H_PERIOD = 320-1;	//320
#else //free size	
	#if TFT_RESOLUTION == USE_162_132    
	R_TFT_V_PERIOD = 132-1;	
    R_TFT_H_PERIOD = 162-1;	
	#elif TFT_RESOLUTION == USE_160_128    
	R_TFT_V_PERIOD = 128-1;	 
    R_TFT_H_PERIOD = 160-1;	
	#endif    
#endif    
    R_TFT_V_START = 1;		//Setup time 
    R_TFT_VS_WIDTH = 20;	//Blanking time
    R_TFT_H_START = 1;		//R/W pulse "high" period = (TFT_H_Start + 1)
    R_TFT_HS_WIDTH = 1;		//R/W pulse "low" period =  (TFT_HS_Width + 1)

#if TFT_RESOLUTION == USE_162_132    
    R_TFT_VS_START = 0;		//0+y
    R_TFT_VS_END = 132;		//132
    R_TFT_HS_START = 0;		//0+x
    R_TFT_HS_END = 	162-1;	//162
#elif TFT_RESOLUTION == USE_160_128
	R_TFT_VS_START = 0;		//0+y
    R_TFT_VS_END = 128;		//128
    R_TFT_HS_START = 0;		//x+0
    R_TFT_HS_END = 	160-1;	//160-1+x
#endif    
	//Start initial sequence 		    
	drv_l1_tft_i80_wr_cmd(0x11);	//00h
	drv_msec_wait(200);

	drv_l1_tft_i80_wr_cmd(0xC0); 	//GVDD
	drv_l1_tft_i80_wr_data(0x02);
	drv_l1_tft_i80_wr_data(0x00);	//VCI 1
	drv_msec_wait(200);
	drv_msec_wait(200);
		
	drv_l1_tft_i80_wr_cmd(0xc1); //AVDD VGL VGL VCL
	drv_l1_tft_i80_wr_data(0x05);

	drv_l1_tft_i80_wr_cmd(0xc5);  //SET VCOMH
	drv_l1_tft_i80_wr_data(0xBA); //08	

	drv_l1_tft_i80_wr_cmd(0xc6);  //VCOMAC
	drv_l1_tft_i80_wr_data(0x0A);	//0D

	drv_l1_tft_i80_wr_cmd(0x3A);
	drv_l1_tft_i80_wr_data(0x05);	//RGB-565
	drv_msec_wait(200);

	drv_l1_tft_i80_wr_cmd(0x36);	//show h or v
	drv_l1_tft_i80_wr_data(0x60);	//by zhangxh 0xC0	

	drv_l1_tft_i80_wr_cmd(0xE0);  //0Eh gamma set
	drv_l1_tft_i80_wr_data(0x00); //V0
	drv_l1_tft_i80_wr_data(0x01); //V1  01
	drv_l1_tft_i80_wr_data(0x0E); //V2  0E
	drv_l1_tft_i80_wr_data(0x35); //V61  35
	drv_l1_tft_i80_wr_data(0x2B); //V62
	drv_l1_tft_i80_wr_data(0x0B); //V63
	drv_l1_tft_i80_wr_data(0x1C); //V13  1F 
	drv_l1_tft_i80_wr_data(0x01); //V50      
	drv_l1_tft_i80_wr_data(0x00); //V4
	drv_l1_tft_i80_wr_data(0x04); //V8   04
	drv_l1_tft_i80_wr_data(0x0C); //V20  0C
	drv_l1_tft_i80_wr_data(0x0F); //V27  0A
	drv_l1_tft_i80_wr_data(0x00); //V36  00
	drv_l1_tft_i80_wr_data(0x08); //V43  08 06
	drv_l1_tft_i80_wr_data(0x03); //V55  02
	drv_l1_tft_i80_wr_data(0x06); //V59  06
	 	
	drv_l1_tft_i80_wr_cmd(0xE1);  //E1h gamma set
	drv_l1_tft_i80_wr_data(0x06); //V0
	drv_l1_tft_i80_wr_data(0x23); //V1
	drv_l1_tft_i80_wr_data(0x20); //V2
	drv_l1_tft_i80_wr_data(0x0E); //V61   OE
	drv_l1_tft_i80_wr_data(0x0A); //V62
	drv_l1_tft_i80_wr_data(0x04); //V63
	drv_l1_tft_i80_wr_data(0x04); //V13
	drv_l1_tft_i80_wr_data(0x19); //V50  1B
	drv_l1_tft_i80_wr_data(0x06); //V4   06
	drv_l1_tft_i80_wr_data(0x04); //V8   03
	drv_l1_tft_i80_wr_data(0x04); //V20  04 02
	drv_l1_tft_i80_wr_data(0x04); //V27  04
	drv_l1_tft_i80_wr_data(0x0A); //V36  05
	drv_l1_tft_i80_wr_data(0x0C); //V43  0C
	drv_l1_tft_i80_wr_data(0x04); //V55  04
	drv_l1_tft_i80_wr_data(0x05); //V59 

	//column address set 
	drv_l1_tft_i80_wr_cmd(0x2A);
	drv_l1_tft_i80_wr_data(0x00);
	drv_l1_tft_i80_wr_data(0x00);
	drv_l1_tft_i80_wr_data(0x00);
#if TFT_RESOLUTION == USE_162_132    
	drv_l1_tft_i80_wr_data(0xa1); 	 //162
#elif TFT_RESOLUTION == USE_160_128    
	drv_l1_tft_i80_wr_data(0x9F);	//160
#endif

	//row address set
	drv_l1_tft_i80_wr_cmd(0x2B);
	drv_l1_tft_i80_wr_data(0x00);
	drv_l1_tft_i80_wr_data(0x00);
	drv_l1_tft_i80_wr_data(0x00);
#if TFT_RESOLUTION == USE_162_132    
	drv_l1_tft_i80_wr_data(0x83);
#elif TFT_RESOLUTION == USE_160_128    
	drv_l1_tft_i80_wr_data(0x7F);
#endif 

	//start show
	drv_l1_tft_i80_wr_cmd(0x29);
	drv_l1_tft_i80_wr_cmd(0x2C);
	
	drv_l1_tft_clk_set(TFT_CLK_DIVIDE_5);
	drv_l1_tft_mode_set(TFT_MODE_MEM_CONTI);
	drv_l1_tft_en_set(TRUE); 
	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param =
{
	160,
	128,
	tft_gpm_lm765a0_init
};
#endif //(defined GPM_LM765A0) && (GPM_LM765A0 == 1)