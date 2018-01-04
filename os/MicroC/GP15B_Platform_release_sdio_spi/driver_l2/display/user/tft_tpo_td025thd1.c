#include "drv_l1_sfr.h"
#include "drv_l1_tft.h"
#include "drv_l1_gpio.h"
#include "drv_l2_display.h"

#if (defined TPO_TD025THD1) && (TPO_TD025THD1 == 1)
static INT8U CSN_n;
static INT8U SCL_n;
static INT8U SDA_n;

static void cmd_delay(INT32U i) 
{
	INT32U j, cnt;
	
	cnt = i*2;
	for (j=0;j<cnt;j++);
}

static void serial_cmd_1(INT32U cmd) 
{
	INT32S i;
	
	gpio_write_io(CSN_n, 1);//CS=1
	gpio_write_io(SCL_n, 0);//SCL=0
	gpio_write_io(SDA_n, 0);//SDA
	
	// set csn low
	gpio_write_io(CSN_n, 0);//CS=0
	cmd_delay(1);
	for (i=0;i<16;i++) {
		// shift data
		gpio_write_io(SDA_n, ((cmd&0x8000)>>15)); /* SDA */
		cmd = (cmd<<1);
		cmd_delay(1);
		// toggle clock
		gpio_write_io(SCL_n, 1);//SCL=0
		cmd_delay(1);
		gpio_write_io(SCL_n, 0);//SCL=0		
		cmd_delay(1);
	}
	
	// set csn high
	gpio_write_io(CSN_n,1);//CS=1
	cmd_delay(1);
}

static INT32S tft_tpo_td025thd1_init(void)
{

#if (BOARD_TYPE == BOARD_EMU_GPB51PG)
	/* 320*240 */
    CSN_n = IO_A10;
	SDA_n = IO_A9;
    SCL_n = IO_A8;
#elif (BOARD_TYPE == BOARD_DVP_GPB51PG)
    CSN_n = IO_D6;
	SDA_n = IO_D8;
    SCL_n = IO_D7;
#endif  
	//board  
#if 0
 	CSN_n = IO_D6;
	SDA_n = IO_D8;
    SCL_n = IO_A13;
#endif
    gpio_init_io (CSN_n, GPIO_OUTPUT);
	gpio_init_io (SCL_n, GPIO_OUTPUT);
	gpio_init_io (SDA_n, GPIO_OUTPUT);
	
	gpio_set_port_attribute(CSN_n, 1);
	gpio_set_port_attribute(SCL_n, 1);
	gpio_set_port_attribute(SDA_n, 1);
	
	serial_cmd_1((0x02 << (2+8)) | 0x00);	//enable    
    //serial_cmd_1((0x05 << (2+8)) | 0x10); //rotate

	R_TFT_HS_WIDTH			= 0;				//	1		=HPW
	R_TFT_H_START			= 1+239;			//	240		=HPW+HBP
	R_TFT_H_END				= 1+239+1280;		//	1520	=HPW+HBP+HDE
	R_TFT_H_PERIOD			= 1+239+1280+40;	//	1560	=HPW+HBP+HDE+HFP
	R_TFT_VS_WIDTH			= 0;				//	1		=VPW				(DCLK)
	R_TFT_V_START			= 21;				//	21		=VPW+VBP			(LINE)
	R_TFT_V_END				= 21+240;			//	261		=VPW+VBP+VDE		(LINE)
	R_TFT_V_PERIOD			= 21+240+1;			//	262		=VPW+VBP+VDE+VFP	(LINE)
	R_TFT_LINE_RGB_ORDER    = 0x00;
	
	drv_l1_tft_signal_inv_set(TFT_VSYNC_INV|TFT_HSYNC_INV, (TFT_ENABLE & TFT_VSYNC_INV)|(TFT_ENABLE & TFT_HSYNC_INV));
	drv_l1_tft_mode_set(TFT_MODE_UPS052);
	drv_l1_tft_data_mode_set(TFT_DATA_MODE_8);
	drv_l1_tft_clk_set(TFT_CLK_DIVIDE_5); /* FS=66 */
	drv_l1_tft_en_set(TRUE);
	return STATUS_OK;
}

// tft table
const DispCtrl_t TFT_Param =
{
	320,
	240,
	tft_tpo_td025thd1_init
};
#endif //(defined TPO_TD025THD1) && (TPO_TD025THD1 == 1)