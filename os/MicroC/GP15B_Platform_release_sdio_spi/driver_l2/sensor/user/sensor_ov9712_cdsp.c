/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "driver_l1.h"
#include "drv_l1_power.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_i2c.h"
#include "drv_l1_csi.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_cdsp_config.h"
#include "drv_l2_sensor.h"
#include "drv_l2_sccb.h"

#if (defined _SENSOR_OV9712_CDSP) && (_SENSOR_OV9712_CDSP == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define OV9712_ID		0x60

#define COLOR_BAR_EN	0

#define OV9712_MAX_EXPOSURE_TIME			(250*3)//50hz, (208*3)//60hz
#define OV9712_MIN_EXPOSURE_TIME			0x08
#define OV9712_MAX_ANALOG_GAIN				(4*256)//(MAX_ANALOG_GAIN*256) //(3*256)	//4x
#define OV9712_MIN_ANALOG_GAIN				(1*256)
#define OV9712_MAX_DIGITAL_GAIN				(1.5*256)//0x3=4x
#define OV9712_MIN_DIGITAL_GAIN 			(1*256)//0:1x

#define OV9712_50HZ_EXP_TIME_TOTAL			212//116
#define	OV9712_50HZ_INIT_EV_IDX				140//61
#define OV9712_50HZ_NIGHT_EV_IDX			153//81
#define OV9712_50HZ_DAY_EV_IDX				100//51
#define OV9712_50HZ_MAX_EXP_IDX				(OV9712_50HZ_EXP_TIME_TOTAL-22)  // <= OV9712_50HZ_EXP_TIME_TOTAL

#define OV9712_60HZ_EXP_TIME_TOTAL			217
#define	OV9712_60HZ_INIT_EV_IDX				140
#define OV9712_60HZ_NIGHT_EV_IDX			153
#define OV9712_60HZ_DAY_EV_IDX				51
#define OV9712_60HZ_MAX_EXP_IDX				(OV9712_60HZ_EXP_TIME_TOTAL-25)  // <= OV9712_60HZ_EXP_TIME_TOTAL

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regval8_s 
{
	INT8U reg_num;
	INT8U value;
} regval8_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if SCCB_MODE == SCCB_GPIO
	static void *ov9712_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t ov9712_handle; 
#endif 

static sensor_exposure_t ov9712_seInfo; 
static INT32S *p_expTime_table;
 
static const regval8_t OV9712_SW_Reset[] =
{
	{0x12, 0x80},
	{0xFF, 0xFF},
}; 
 
static const regval8_t OV9712_1280_720_F30[]=
{
	//{0x12,0x80},	//Reset
	{0x09,0x10},
	
	//---------------------------------------------------------
	//Core Settings
	//---------------------------------------------------------
	
	{0x1e,0x07},
	{0x5f,0x18},
	{0x69,0x04},
	{0x65,0x2a},
	{0x68,0x0a},
	{0x39,0x28},
	{0x4d,0x90},
	{0xc1,0x80},
	{0x0c,0x30},
	{0x6d,0x02},
	{0x60,0x9d},
	
	
	/*HYSNC Mode Enable*/
	{0xcb, 0x32},    //Hsync mode enable
	{0xc3, 0x21},    //0x29;Starts to calculate the parameters for HYSNC mode
	{0xc2, 0x88},//b1},//84},    //pclk gate disable
		
#if 1
	{0x15, 0x00},//40},	//HREF swap to HYSNC
	{0x30, 0x08},    //HYSNC Start Point
	{0x31, 0x20},    //HYSNC Stop Point
	{0x0c, 0x30},    //??[0]:single_frame_output                 
	{0x6d, 0x02},    //reserved                                  
#endif
	
	//---------------------------------------------------------
	//DSP
	//---------------------------------------------------------
	{0x96,0x00},	//0xf1: DSP options enable
	{0xbc,0x68},	// [7]   reserved
	                // [6:5] bd_sel
	                // [4]   th_opt
	                // [3:0] thresh_hold
	//---------------------------------------------------------
	//Resolution and Format
	//---------------------------------------------------------
	
	{0x12,0x00},
	{0x3b,0x00},	// DSP Downsample
	{0x97,0x80},	// [7]   smph_mean
	                // [6]   reserved
	                // [5:4] smph_drop
	
	//---- Place generated settings here ----//
	/*HightxWeight*/
	{0x17,0x25},	//HSTART
	{0x18,0xD0}, //0xCA},//{0x18,0xA2},	//HSize[10:3]	/*0xA2x8=0x510=1296*/0xca
	{0x19,0x01},	//VSTART_MSBs 
	{0x1a,0xCF},//{0x1a,0xCA},	//VSize_MSB[9:2] /*0xca0>>2=808*/
	{0x03,0x0A},	//[3:2]VSize_LSBs,[1:0]VSTART_LSBs 
	{0x32,0x07},	//[5:3]HSize_LSBs[2:0],[2:0]HSTART_LSBs (0x07)
	
	{0x98,0x00},
	{0x99,0x28},
	{0x9a,0x00},
	//DSP Output Size
	{0x57, 0x00},    	//DSP[4:2] output_Hszie_LSBs,[1:0]output_Vsize_LSBs           
	{0x58, 0xB6},   	//DSP output_Vsize_MSBs	/*0xc80>>2=800; 0xb40>>2=720*/
	{0x59, 0xA2},   	//DSP output_Hsize_MSBs	/*0xa00>>1=1280*/
	
	{0x4c,0x13},
	{0x4b,0x36},
	{0x3d,0x3c},
	{0x3e,0x03},
	{0xbd,0xA0},
	{0xbe,0xb4},
	
	//---------------------------------------------------------
	//Flip
	//---------------------------------------------------------
#if 0 //SENSOR_FLIP
	{0x04, 0xC0}, //Mirror & Flip
#elif 0
	{0x04, 0x80}, //Mirror
#elif 0
	{0x04, 0x40}, //Flip		//VHREF must be adjusted.
#else
	{0x04, 0x00}, //Normal
#endif	
	
	//---------------------------------------------------------
	//Lens Correction
	//---------------------------------------------------------
	
	
	//---- Place lens correction settins here ----//
	// Lens model  	:
	// Module type	:
	
	
	
	//---------------------------------------------------------
	//YAVG
	//---------------------------------------------------------
	
	//---- Place generated "WIndows Weight" settings here ----//
	
	{0x4e,0x55},	//AVERAGE 
	{0x4f,0x55},	//		
	{0x50,0x55},	//
	{0x51,0x55},	//
	
	
	{0x24,0x55},	//Exposure windows
	{0x25,0x40},
	{0x26,0xa1},
	
	//---------------------------------------------------------
	//Clock
	//---------------------------------------------------------
	{0x5c,0x52},
	{0x5d,0x00},
	{0x11,0x01},
	{0x2a,0x98},	
	{0x2b,0x06},
	{0x2d,0x00},
	{0x2e,0x00},
	
	
	//---------------------------------------------------------
	//General
	//1x~16x is analog gain. 16x~ 64x is digital gain 
	//0x00= 00 ~0F 1x ~2x gain 0x00= 10 ~1F 2x ~4x gain 
	//0x00= 30 ~3F 4x ~8x gain 0x00= 70 ~7F 8x ~16x gain 0x00= f0 ~FF 16x ~32x gain
	//---------------------------------------------------------

	{0x13,0xA5},
	{0x14,0x40},	//0x40:Gain Ceiling 8X
	
	//---------------------------------------------------------
	//Banding
	//---------------------------------------------------------
	
	{0x4a,0x00},
	{0x49,0xce},
	{0x22,0x03},
	
	
	//off 3A
	{0x13,0x80}, // manual AEC/AGC
	{0x38,0x00}, // 0x10:manual AWB, 0x0:ISP control WB
	{0xb6,0x08}, 
	{0x96,0xe1}, //0xe1},
	// 
	{0x01,0x40}, // Unity WB R/G/B Gain 
	{0x02,0x40},
	{0x05,0x40},
	{0x06,0x00},
	{0x07,0x00},
	//
	
	{0x10, 0x50},		//Exposure_Time_LSB	//0xF9:2700K, 0xD0:6500K
	{0x16, 0x01},		//Exposure_Time_MSB
	{0x00, 0x00},		//AGC Gain CTRL for manual
	
	
	{0x09, 0x00},
	{0xFF, 0xFF}
};

static const regval8_t OV9712_1280_800_F30[] =	//1280*800,24MHz,30fps
{	
	// CMOS Sensor Initialization Start... 
	//{0x12, 0x80},	//reset,[7]set to factory default values for all register										                      

	/*Core Setting*/
	{0x1e, 0x07},   	//reserved                                             
	{0x5f, 0x18},    //reserved                                            
	{0x69, 0x04},    //reserved                                            
	{0x65, 0x2a},    //reserved                                  
	{0x68, 0x0a},    //reserved                                  
	{0x39, 0x28},    //reserved                                  
	{0x4d, 0x90},    //reserved                                  
	{0xc1, 0x80},	//added 
					
	/*HYSNC Mode Enable*/
	{0xcb, 0x32},    //Hsync mode enable
	{0xc3, 0x21},    //0x29;Starts to calculate the parameters for HYSNC mode
	{0xc2, 0x88},//b1},//84},    //pclk gate disable
		
#if 1
	{0x15, 0x00},//40},	//HREF swap to HYSNC
	{0x30, 0x05},    //HYSNC Start Point
	{0x31, 0x20},    //HYSNC Stop Point
	{0x0c, 0x30},    //??[0]:single_frame_output                 
	{0x6d, 0x02},    //reserved                                  
#endif
	
	/*DSP*/
	{0x96, 0x00}, 	//DSP options enable_0xf1, disable 0  //en_black&white_pixel,isp 0xC1
	{0xbc, 0x68},    //WB_CTRL                                   
	/* PLL (Pixel CLK)*/				//CLK1=XCLK(MCLK)/r5c[6:5]
#if 1
	#if 0	//pclk=2*mclk
	{0x5c, 0x17},      
	{0x5d, 0x84},    	
	{0x11, 0x00},		
	#elif 1	//pclk=//27MHz->10MHz /*for FPGA*/
	{0x11, 0x01},			//PCLK=sysCLK=CLK3/((r11[5:0]+1)*2)=>36/((1+1)*2)=9
	{0x5c, 0x5D},    		//CLK2 = ((mclk/2) *6) => 12/2*6=36
	{0x5d, 0x01},  			//CLK3 = CLK2/(r5d[3:2]+1)=>42/(0+1)=36
	#elif 0		//pclk=mclk/4  (27MHz->7MHz)	//未確認是否可行						
	{0x11, 0x01},			//PCLK=sysCLK=CLK3/((r11[5:0]+1)*2)=>PCLK=54/((1+1)*2)
	{0x5c, 0x5D},    		//CLK2=CLK1*(32-r5c[4:0]) ,CLK1=(MCLK/r5c[6:5])=>CLK2=((27/2)*(32-28))=54MHz
	{0x5d, 0x01},  			//CLK3=CLK2/(r5d[3:2]+1)=>CLK3=54/(0+1)=54
	#else 		//pclk=mclk/2  //未確認是否可行		
	{0x11, 0x01},			//PCLK=sysCLK=CLK3/((r11[5:0]+1)*2)=>PCLK=54/((1+1)*2)
	{0x5c, 0x5E},    		//CLK2=CLK1*(32-r5c[4:0]) ,CLK1=(MCLK/r5c[6:5])=>CLK2=((27/2)*(32-28))=54MHz
	{0x5d, 0xC0},
	#endif
#endif	
	
	/*Mirror & Vertical Flip*/
	#if 1
	{0x04, 0x00}, //Normal
	#elif 0
	{0x04, 0x80}, //Mirror
	#elif 0
	{0x04, 0x40}, //Flip		//VHREF must be adjusted.
	#else
	{0x04, 0xC0}, //Mirror & Flip
	#endif	
	
	/*Resolution and format*/
	{0x3b, 0x00},    //reserved                                  
#if COLOR_BAR_EN == 1	
	{0x96, 0xf1},
	{0x12, 0x02},    //tune resolution reference vertical or Horizontal timming
	{0x3b, 0x00},    //reserved   
	{0x97, 0x08},    //smph_mean+colorbar
#else
	//{ 0x12, 0x00},
	{0x97, 0x80},    //smph_mean, test pattern                               
#endif
	
	/*HightxWeight*/
	{0x17, 0x25},    //HSTART,start 0x25 valid frame
	{0x18, 0xa2},    //HSize_MSB /*0xa20>>1=1296*/
	{0x19, 0x02},    //0x0//VSTART_MSBs                               
	{0x1a, 0xca},    //VSize_MSB /*0xca0>>2=808*/                             
	{0x03, 0x02},    //[3:2]VSize_LSBs,[1:0]VSTART_LSBs                   
	{0x32, 0x00},//7},    //[5:3]HSize_LSBs,[2:0]HSTART_LSBs (0x07)      
		            
	{0x98, 0x00},   	//pre_out_hoff[7:0]		/*sccb_write-(SLAVE_ID,0x98, 0x00)*/         
	{0x99, 0x00},   	//pre_out_voff[7:0]		/*sccb_write-(SLAVE_ID,0x99, 0x00)*/         
	{0x9a, 0x00},   	//pre_out_voff[9:8],pre_out_hoff[10:8]/*sccb_write-(SLAVE_ID,0x9a, 0x00)*/   
	{0x57, 0x00},    //DSP[4:2] output_Hszie_LSBs,[1:0]output_Vsize_LSBs           
	{0x58, 0xc8},   	//DSP output_Vsize_MSBs	/*0xc80>>2=800; 0xb40>>2=720*/
	{0x59, 0xa0},   	//DSP output_Hsize_MSBs	/*0xa00>>1=1280*/
	{0x4c, 0x13},    //reserved                                            
	{0x4b, 0x36},    //reserved                                            
	{0x3d, 0x3c},    //Red_counter_End_Point_LSBs                          
	{0x3e, 0x03},    //Red_counter_End_Point_MSBs                                                                       
	
#if 1		//Manual Exposure time & Gain
	/*AEC/AGC/Banding function*/
	{0x13, 0x0},	//OFF Fast AEC adj,Banding ,Auto AGC, Auto Exposure
	
	{0x10, 0xf0},		//Exposure_Time_LSB
	{0x16, 0x01},		//Exposure_Time_MSB
	{0x00, 0x15},		//AGC Gain CTRL for manual
	
	{0x4a, 0x00},	//Banding step MSBs[1:0]
	{0x49, 0xce},	//Banding step LSBs[1:0]
	{0x22, 0x03},	//Max smooth banding steps
	//{0x09, 0x00},
	{0x0d, 0x01},	//AEC_ctrl,default:0x01,
	{0x0e, 0x40},	//AEC/AGC_based mode,default:0x40,
	{0x14, 0x50},//40},	//AGC-Gainx8(0x40)
	{0x1f, 0x0},		//LAEC[7:0]:less_lan_1_row_of_exposure_time
	{0x3c, 0x0},		//LAEC[15:8]
	
	/*AEC/AGC operation*/				//Maximum_Exposue_Lines:[(0x1A+0x03[3:2])-2Lines]
	{0x24, 0x70},    //Luminance Signal High Range for AEC/AGC                       
	{0x25, 0x40},    //Luminance Signal Low  Range for AEC/AGC                       
	{0x26, 0xa1},    //Fast mode Thresholds,([3:0]*16)>YAVG(0x2f)>([7:4]*16)
#endif
	
#if 0	//Auto Exporsure & Gain
	/*AEC/AGC/Banding function*/
	{0x13, 0xa5},	//Fast AEC adj,Banding ON,Auto AGC, Auto Exposure
	{0x1f, 0x0},		//LAEC[7:0]:less_lan_1_row_of_exposure_time
	{0x3c, 0x0},		//LAEC[15:8]
	{0x00, 0x80},		//AGC Gain CTRL    	
	{0x14, 0x40},	//AGC-Gainx8(0x40)
	
	{0x4a, 0x00},	//Banding step MSBs[1:0]
	{0x49, 0xce},	//Banding step LSBs[1:0]
	{0x22, 0x03},	//Max smooth banding steps
	//{0x09, 0x00},
	
	{0x10, 0x60},		//Exposure_Time_LSB
	{0x16, 0x02},		//Exposure_Time_MSB
	{0x0d, 0x01},	//AEC_ctrl,default:0x01,
	{0x0e, 0x40},	//AEC/AGC_based mode,default:0x40,
	
	/*AEC/AGC operation*/				//Maximum_Exposue_Lines:[(0x1A+0x03[3:2])-2Lines]
	{0x24, 0x70},    //Luminance Signal High Range for AEC/AGC                       
	{0x25, 0x40},    //Luminance Signal Low  Range for AEC/AGC                       
	{0x26, 0xa1},    //Fast mode Thresholds,([3:0]*16)>YAVG(0x2f)>([7:4]*16)
	
	/*YAVG_CTRL*/
	{0xc1, 0x80},    //YAVG_CTRL:yacg_win_man en                                 
	{0xbd, 0xa0},   	//yavg_winh	/*sccb_write-(SLAVE_ID,0xbd, 0xa0},*/
	{0xbe, 0xc8},   	//yavg_winv	/*sccb_write-(SLAVE_ID,0xbe, 0xc8},*/
	/*16-Zone_Y_Avarage_Select*/
	{0x4e, 0x55},    //Y Avg Select:zone1~4 = weight x 1                 
	{0x4f, 0x55},	//Y Avg Select:zone5~8 = weight x 1            
	{0x50, 0x55},	//Y Avg Select:zone9~12 = weight x 1                  
	{0x51, 0x55},	//Y Avg Select:zone13~16 = weight x 1
	{0x2c, 0x60},	//Reserved
	{0x23, 0x10},    //Reserved       	

	/*Histogram-based*/
	{0x72, 0x60},    //
	{0x73, 0x49},    //
	{0x74, 0xe0},    //
	{0x75, 0xe0},    //
	{0x76, 0xc0},    //
	{0x77, 0xc0},    //
	{0x78, 0xff},    //
	{0x79, 0xff},    //
#endif
	
	{0x2f, 0x00},//55},    //Luminance_Average_Value(YAVG)
	
	{0x09, 0x08},   	//[4]:Chip sleep mode, [3]:Reset sensor timing when mode changes  
	{0x55, 0xff},    //0xfc:disable Y0,Y1; 0xff:enable Y0,Y1
	{0x56, 0x1f},    //Enable HREF & enable HSync b[1:0]=Y9,Y8
	{0xca, 0x20}, 	//test pattern bit b[2]0:10bit, 1:8bit
	{0xFF, 0xFF}
};	

static const regval8_t OV9712_640_480_F30[] =	//640*480,24MHz, 30fps	//copy from 1280p
{
	//{0x12, 0x80},	//reset,[7]set to factory default values for all register										                      	
	{0x09, 0x10},   //[4]:Chip sleep mode, [3]:Reset sensor timing when mode changes
	{0x1e, 0x07},
	{0x5f, 0x18},
	{0x69, 0x04},
	{0x65, 0x2a},
	{0x68, 0x0a},
	{0x39, 0x28},
	{0x4d, 0x90},
	{0xc1, 0x80},
	{0x0c, 0x30},
	{0x6d, 0x02},
	{0x96, 0xf1},
	{0xbc, 0x68},
	{0x12, 0x00},
	{0x3b, 0x00},
	{0x97, 0x80},
	{0x17, 0x25},
	{0x18, 0xA0}, /* 0xA2 */
	{0x19, 0x00}, /* 0x01 */
	{0x1a, 0x7a}, /* 0xCA */
	{0x03, 0x00}, /* 0x0A */
	{0x32, 0x07},
	{0x98, 0x00}, /* 0x40 */
	{0x99, 0x00}, /* 0xA0 */
	{0x9a, 0x00}, /* 0x01 */
	{0x57, 0x00},
	{0x58, 0x7a}, /* 0x78 */
	{0x59, 0xA0}, /* 0x50 */
	{0x4c, 0x13},
	{0x4b, 0x36},
	{0x3d, 0x3c},
	{0x3e, 0x03},
	{0xbd, 0x50},
	{0xbe, 0x78},
	{0x4e, 0x55},
	{0x4f, 0x55},
	{0x50, 0x55},
	{0x51, 0x55},
	{0x26, 0x92},
	{0x5c, 0x59},
	{0x5d, 0x00},
	{0x11, 0x00},
	{0x2a, 0x98},
	{0x2b, 0x06},
	{0x2d, 0x00},
	{0x2e, 0x00},
	{0x14, 0x40},
	
	//light band
	{0x4a, 0x00},
	{0x49, 0xce},
	{0x22, 0x03},	
		
	//off 3A
	{0x13,0x80}, // manual AEC/AGC
	{0x38,0x00}, // 0x10:manual AWB, 0x0:ISP control WB
	{0xb6,0x08}, 
	{0x96,0xe1}, //0xe1},
	// 
	{0x01,0x40}, // Unity WB R/G/B Gain 
	{0x02,0x40},
	{0x05,0x40},
	{0x06,0x00},
	{0x07,0x00},
	//
	
	{0x10, 0x50},		//Exposure_Time_LSB	//0xF9:2700K, 0xD0:6500K
	{0x16, 0x01},		//Exposure_Time_MSB
	{0x00, 0x00},		//AGC Gain CTRL for manual
	
	{0x09, 0x00},
	{0xFF, 0xFF}
};

static const regval8_t OV9712_640_400_F30[] =	//640*400,24MHz, 30fps	//copy from 1280p
{
	//{0x12, 0x80},	//reset,[7]set to factory default values for all register										                      	
	//{0x09, 0x10},   	//[4]:Chip sleep mode, [3]:Reset sensor timing when mode changes
	/*Core Setting*/
	{0x1e, 0x07},   	//reserved                                             
	{0x5f, 0x18},    //reserved                                            
	{0x69, 0x04},    //reserved                                            
	{0x65, 0x2a},    //reserved                                  
	{0x68, 0x0a},    //reserved                                  
	{0x39, 0x28},    //reserved                                  
	{0x4d, 0x90},    //reserved                                  
	{0xc1, 0x80},	//added 
			
	//sccb_read(SLAVE_ID, 0x65},				//read 	
		
	/*DSP*/
	{0x96, 0x01}, 	//DSP options enable_0xf1, disable 0  //en_black&white_pixel,isp 0xC1
	{0xbc, 0x68},    //WB_CTRL                                   
	
	/*HYSNC Mode Enable*/
	{0xcb, 0x32},    //Hsync mode enable
	{0xc3, 0x21},    //0x29;Starts to calculate the parameters for HYSNC mode
	{0xc2, 0x88},    //pclk gate disable
		
	{0x15, 0x0},	//HREFswaptoHYSNC(0x40)
	{0x30, 0x05},    //HYSNC Start Point
	{0x31, 0x20},    //HYSNC Stop Point
	{0x0c, 0x30},    //??[0]:single_frame_output                 
	{0x6d, 0x02},    //reserved                                  
		
	/*Resolution and format*/
#if 0	//this config disable dsp still can start sensor
	{0x12, 0x80},   //software reset
	{0x3b, 0x0},    //reserved                                 
	{0x97, 0x0},    //smph_mean                                 
#endif

#if 0	//show test pattern colorBar
	//{0x12, 0x80},    //tune resolution reference vertical or Horizontal timming
	{0x3b, 0x0},    //reserved   
	{0x97, 0x88},    //smph_mean+colorbar
#endif
	
#if 1
	{0x96, 0x01},	//dsp must be enable
	{0x12, 0x40},   	//tune_resolution_reference_vertical_orHorizontal_timming(down sample)
	{0x3b, 0x0},    	//reserved                                  
	{0x97, 0x80},   	//smph_mean     
#endif
		
	//{0x3b, 0x0},    	//reserved                                  
	//{0x97, 0x0},	//smph_mean     
	
	/*HightxWeight*/
	{0x17, 0x25},    //HSTART,MSBs 0x25
	{0x18, 0xa2},    //HSize (0xa20>>1=1296) (H640= 0x510>>1)
	{0x19, 0x01},    //0x0//VSTART_MSBs                               
	{0x1a, 0x78},    //VSize_MSBs  (0x780>>2=480)                             
	{0x03, 0x02},    //[3:2]VSize_LSBs,[1:0]VSTART_LSBs                   
	{0x32, 0x07},    //[3:2]HSize_LSBs,[1:0]HSTART_LSBs (0x07)      
		            
	{0x98, 0x00},   	//pre_out_hoff[7:0]		/*sccb_write-(SLAVE_ID,0x98, 0x00)*/         
	{0x99, 0x00},   	//pre_out_voff[7:0]		/*sccb_write-(SLAVE_ID,0x99, 0x00)*/         
	{0x9a, 0x00},   	//pre_out_voff[9:8],pre_out_hoff[10:8]/*sccb_write-(SLAVE_ID,0x9a, 0x00)*/   
	{0x57, 0x00},    //DSP[4:2] output_Hszie_LSBs,[1:0]output_Vsize_LSBs           
	{0x58, 0x78},   	//DSP output_Vsize_MSBs	/*0x780>>2=480*/ 0xb40=0x2d0=720
	{0x59, 0x50},   	//DSP output_Hsize_MSBs	/*0x500>>1=640*/ 
	{0x4c, 0x09},    //reserved                                            
	{0x4b, 0x9a},    //reserved                                            
	{0x3d, 0x9e},//3c},    //Red_counter_End_Point_LSBs                          
	{0x3e, 0x01},//03},    //Red_counter_End_Point_MSBs                          
	
	/*YAVG_CTRL*/
	{0xc1, 0x80},    //YAVG_CTRL:yacg_win_man en                                 
	{0xbd, 0xa0},   	//yavg_winh	/*sccb_write-(SLAVE_ID,0xbd, 0xa0},*/
	{0xbe, 0xc8},   	//yavg_winv	/*sccb_write-(SLAVE_ID,0xbe, 0xc8},*/
	{0x4e, 0x55},    //Y Avg Select:zone1~4 = weight x 1                 
	{0x4f, 0x55},	//Y Avg Select:zone5~8 = weight x 1            
	{0x50, 0x55},	//Y Avg Select:zone9~12 = weight x 1                  
	{0x51, 0x55},	//Y Avg Select:zone13~16 = weight x 1
	{0x2c, 0x60},	//Reserved
 	{0x23, 0x10},    //Reserved             
	
	/*AEC/AGC operation*/
	{0x24, 0x55},    //Luminance Signal High Range for AEC/AGC                       
	{0x25, 0x40},    //Luminance Signal Low  Range for AEC/AGC                       
	{0x26, 0xa1},    //Thresholds effective                                          

#if 1
	/*AEC/AGC/Banding function*/
	{0x4a, 0x00},	//Banding step MSBs[1:0]
	{0x49, 0xce},	//Banding step LSBs[1:0]
	{0x22, 0x03},	//Max smooth banding steps
	//{0x09, 0x00},
	{0x16, 0x01},
	{0x10, 0x35},
    {0x00, 0x08},	//AGC Gain CTRL
    {0x09, 0x08},   	//[4]:Chip sleep mode, [3]:Reset sensor timing when mode changes  
	{0x55, 0xfc},    //disable Y0,Y1
	{0x56, 0x1f},    //Enable HREF & enable HSync
#endif	

	/* PLL (Pixel CLK)*/				//CLK1=XCLK(MCLK)/r5c[6:5]
#if 0	//pclk=mclk*2
	{0x5c, 0x17},    //CLK2=CLK1*(32-r5c[4:0])  
	{0x5d, 0x84},    	//CLK3  clk2/1
	{0x11, 0x00},		//sysCLK clk3/((reg+1)*2)
#else	//for FPGA pclk=mclk/2
	{0x5c, 0x59},    //CLK2  plck=mclk/4 *7
	{0x5d, 0x00},  	//CLK3=CLK2/(r5d[3:2]+1)
	{0x11, 0x01},	//sysCLK clk3/((reg+1)*2)
#endif									//PCLK=sysCLK=CLK3/((r11[5:0]+1)*2)
	
	
	//{0xd6, 0x0c},	//0x00 //system control
		
	/*H TP Counter End Point*/
	{0x2a, 0x98},	//LSBs
	{0x2b, 0x06},	//MSBs
	
	/*For night mode*/
	{0x2d, 0x00},	//dummy line_LSBs
	{0x2e, 0x00},	//dummy line_MSBs
	
	{0x13, 0x00},	//OFF Fast AEC adj,Banding ,Auto AGC, Auto Exposure
	{0x14, 0x10},	//AGC Gain*8(0x40)
	{0xFF, 0xFF}
};

static const unsigned short OV9712_r_b_gain[71][2] = 
{
	{33, 143}, // 2
	{36, 142},
	{39, 141},
	{42, 140},
	{44, 140},
	{47, 139},
	{49, 138},
	{52, 137},
	{54, 136},
	{56, 135}, // 3
	{58, 134},
	{60, 132},
	{62, 131},
	{64, 130},
	{66, 129},
	{68, 128}, // 3.6
	{69, 127},
	{71, 126},
	{72, 124},
	{74, 123}, // 4
	{75, 122},
	{77, 121},
	{78, 119},
	{79, 118},
	{80, 117},
	{81, 115},
	{82, 114},
	{83, 113},
	{84, 111},
	{84, 110}, // 5
	{85, 108},
	{86, 107},
	{86, 105},
	{87, 104},
	{88, 102}, // 5.5
	{88, 101},
	{88, 99}, 
	{89, 98}, 
	{89, 96}, 
	{90, 95},  // 6
	{90, 93}, 
	{90, 91}, 
	{90, 90}, 
	{90, 88}, 
	{91, 86},  // 6.5
	{91, 84}, 
	{91, 83}, 
	{91, 81}, 
	{91, 79}, 
	{91, 77}, 
	{91, 76}, 
	{91, 74}, 
	{91, 72}, 
	{91, 70}, 
	{91, 68}, 
	{90, 66}, 
	{90, 64}, 
	{90, 62}, 
	{90, 60}, 
	{90, 58}, 
	{90, 56}, 
	{90, 54}, 
	{89, 52}, 
	{89, 50}, 
	{89, 48}, 
	{89, 46}, 
	{89, 44}, 
	{89, 41}, 
	{89, 39}, 
	{89, 37}, 
	{88, 35}
};

static const unsigned int OV9712_gamma_045_table[] =
{
	0x11110a, 0x11110e, 0x111112, 0x011116, 0x04441a, 0x11041e, 0x011121, 0x044425, 
	0x011128, 0x10442c, 0x04112f, 0x110433, 0x104436, 0x044139, 0x04113c, 0x011040, 
	0x011043, 0x011046, 0x011049, 0x04104c, 0x04104f, 0x104151, 0x104154, 0x010457, 
	0x04105a, 0x10415c, 0x04045f, 0x104161, 0x010464, 0x104166, 0x041069, 0x01016b, 
	0x10406e, 0x040470, 0x010472, 0x004174, 0x104077, 0x101079, 0x04107b, 0x04047d, 
	0x04047f, 0x040481, 0x040483, 0x040485, 0x101087, 0x101089, 0x00408b, 0x00408d, 
	0x01018e, 0x040490, 0x001092, 0x004094, 0x040195, 0x100497, 0x004099, 0x04019a, 
	0x00109c, 0x01009e, 0x10049f, 0x0040a1, 0x1001a2, 0x0040a4, 0x1001a5, 0x0040a7, 
	0x1004a8, 0x0100aa, 0x0004ab, 0x0100ad, 0x0010ae, 0x0401af, 0x0040b1, 0x0004b2, 
	0x0400b4, 0x0010b5, 0x1001b6, 0x0100b8, 0x0010b9, 0x1001ba, 0x0100bc, 0x0010bd, 
	0x1001be, 0x0100c0, 0x0010c1, 0x1001c2, 0x0100c4, 0x0010c5, 0x1001c6, 0x0100c8, 
	0x0010c9, 0x1001ca, 0x0100cc, 0x0010cd, 0x1001ce, 0x0100d0, 0x0010d1, 0x0400d3, 
	0x0040d4, 0x1004d5, 0x0100d7, 0x0010d8, 0x0400da, 0x0010db, 0x0401dc, 0x0010de, 
	0x0401df, 0x0010e1, 0x0401e2, 0x0010e4, 0x0100e6, 0x1004e7, 0x0040e9, 0x0401ea, 
	0x0010ec, 0x0040ee, 0x0401ef, 0x1010f1, 0x0040f3, 0x0100f5, 0x0401f6, 0x0404f8, 
	0x1010fa, 0x1010fc, 0x0010fe, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff
};

static const short OV9712_color_matrix4gamma045[9] = 
{	
	(short) (1.20584525531044860000*64),
	(short) (-0.1508717276180829600*64),
	(short) (-0.0549735276923655550*64),
	(short) (-0.2250626277587189100*64),
	(short) (1.32599878790178850000*64),
	(short) (-0.1009361601430696400*64),
	(short) (-0.0027266614940476691*64),
	(short) (-0.9601597518226134800*64),
	(short) (1.96288641331666120000*64)
};

static const short OV9712_awb_thr[19] = 
{
	200, // awbwinthr
	
	0*64, // sindata
	1*64, // cosdata 
	
	 30, // Ythr0
	 90, // Ythr1
	150, // Ythr2 
	200, // Ythr3 
	
	-4, // UL1N1 
	 6, // UL1P1 
	-4, // VL1N1 
	 4, // VL1P1 
	
	 -6, //UL1N2
	  8, //UL1P2
	 -6, //VL1N2
	  6, // VL1P2
	
	 -9, // UL1N3
	  8, //UL1P3
	 -9, // VL1N3
	  7, //VL1P3
};

static const sensor_calibration_t ov9712_cdsp_calibration =
{
	NULL,							//ob
	NULL, 							//lenscmp
	OV9712_r_b_gain,				//wb_gain
	OV9712_gamma_045_table,			//gamma1
	OV9712_color_matrix4gamma045,	//color_matrix1
	OV9712_gamma_045_table,			//gamma2
	OV9712_color_matrix4gamma045,	//color_matrix2
	OV9712_awb_thr					//awb_thr	
};

static const  int g_OV9712_exp_time_gain_60Hz[OV9712_60HZ_EXP_TIME_TOTAL][3] = 
{ // {time, analog gain, digital gain}
{8,		(int)(1.00*256), (int)(1.00*256)},
{9,		(int)(1.00*256), (int)(1.00*256)},
{9,		(int)(1.00*256), (int)(1.00*256)},
{9,		(int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{10,	(int)(1.00*256), (int)(1.00*256)},
{11,	(int)(1.00*256), (int)(1.00*256)},
{11,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{12,	(int)(1.00*256), (int)(1.00*256)},
{13,	(int)(1.00*256), (int)(1.00*256)},
{13,	(int)(1.00*256), (int)(1.00*256)},
{14,	(int)(1.00*256), (int)(1.00*256)},
{14,	(int)(1.00*256), (int)(1.00*256)},
{15,	(int)(1.00*256), (int)(1.00*256)},
{15,	(int)(1.00*256), (int)(1.00*256)},
{16,	(int)(1.00*256), (int)(1.00*256)},
{16,	(int)(1.00*256), (int)(1.00*256)},
{17,	(int)(1.00*256), (int)(1.00*256)},
{18,	(int)(1.00*256), (int)(1.00*256)},
{18,	(int)(1.00*256), (int)(1.00*256)},
{19,	(int)(1.00*256), (int)(1.00*256)},
{19,	(int)(1.00*256), (int)(1.00*256)},
{20,	(int)(1.00*256), (int)(1.00*256)},
{21,	(int)(1.00*256), (int)(1.00*256)},
{22,	(int)(1.00*256), (int)(1.00*256)},
{22,	(int)(1.00*256), (int)(1.00*256)},
{23,	(int)(1.00*256), (int)(1.00*256)},
{24,	(int)(1.00*256), (int)(1.00*256)},
{25,	(int)(1.00*256), (int)(1.00*256)},
{26,	(int)(1.00*256), (int)(1.00*256)},
{27,	(int)(1.00*256), (int)(1.00*256)},
{28,	(int)(1.00*256), (int)(1.00*256)},
{29,	(int)(1.00*256), (int)(1.00*256)},
{30,	(int)(1.00*256), (int)(1.00*256)},
{31,	(int)(1.00*256), (int)(1.00*256)},
{32,	(int)(1.00*256), (int)(1.00*256)},
{33,	(int)(1.00*256), (int)(1.00*256)},
{34,	(int)(1.00*256), (int)(1.00*256)},
{35,	(int)(1.00*256), (int)(1.00*256)},
{36,	(int)(1.00*256), (int)(1.00*256)},
{38,	(int)(1.00*256), (int)(1.00*256)},
{39,	(int)(1.00*256), (int)(1.00*256)},
{40,	(int)(1.00*256), (int)(1.00*256)},
{42,	(int)(1.00*256), (int)(1.00*256)},
{43,	(int)(1.00*256), (int)(1.00*256)},
{45,	(int)(1.00*256), (int)(1.00*256)},
{46,	(int)(1.00*256), (int)(1.00*256)},
{48,	(int)(1.00*256), (int)(1.00*256)},
{50,	(int)(1.00*256), (int)(1.00*256)},
{51,	(int)(1.00*256), (int)(1.00*256)},
{53,	(int)(1.00*256), (int)(1.00*256)},
{55,	(int)(1.00*256), (int)(1.00*256)},
{57,	(int)(1.00*256), (int)(1.00*256)},
{59,	(int)(1.00*256), (int)(1.00*256)},
{61,	(int)(1.00*256), (int)(1.00*256)},
{63,	(int)(1.00*256), (int)(1.00*256)},
{66,	(int)(1.00*256), (int)(1.00*256)},
{68,	(int)(1.00*256), (int)(1.00*256)},
{70,	(int)(1.00*256), (int)(1.00*256)},
{73,	(int)(1.00*256), (int)(1.00*256)},
{75,	(int)(1.00*256), (int)(1.00*256)},
{78,	(int)(1.00*256), (int)(1.00*256)},
{81,	(int)(1.00*256), (int)(1.00*256)},
{84,	(int)(1.00*256), (int)(1.00*256)},
{87,	(int)(1.00*256), (int)(1.00*256)},
{90,	(int)(1.00*256), (int)(1.00*256)},
{93,	(int)(1.00*256), (int)(1.00*256)},
{96,	(int)(1.00*256), (int)(1.00*256)},
{99,	(int)(1.00*256), (int)(1.00*256)},
{103,	(int)(1.00*256), (int)(1.00*256)},
{107,	(int)(1.00*256), (int)(1.00*256)},
{110,	(int)(1.00*256), (int)(1.00*256)},
{114,	(int)(1.00*256), (int)(1.00*256)},
{118,	(int)(1.00*256), (int)(1.00*256)},
{122,	(int)(1.00*256), (int)(1.00*256)},
{127,	(int)(1.00*256), (int)(1.00*256)},
{131,	(int)(1.00*256), (int)(1.00*256)},
{136,	(int)(1.00*256), (int)(1.00*256)},
{141,	(int)(1.00*256), (int)(1.00*256)},
{145,	(int)(1.00*256), (int)(1.00*256)},
{151,	(int)(1.00*256), (int)(1.00*256)},
{156,	(int)(1.00*256), (int)(1.00*256)},
{161,	(int)(1.00*256), (int)(1.00*256)},
{167,	(int)(1.00*256), (int)(1.00*256)},
{173,	(int)(1.00*256), (int)(1.00*256)},
{179,	(int)(1.00*256), (int)(1.00*256)},
{185,	(int)(1.00*256), (int)(1.00*256)},
{192,	(int)(1.00*256), (int)(1.00*256)},
{199,	(int)(1.00*256), (int)(1.00*256)},
{206,	(int)(1.00*256), (int)(1.00*256)},
{213,	(int)(1.00*256), (int)(1.00*256)},
{213,	(int)(1.00*256), (int)(1.04*256)},
{213,	(int)(1.06*256), (int)(1.00*256)},
{213,	(int)(1.13*256), (int)(1.00*256)},
{213,	(int)(1.13*256), (int)(1.00*256)},
{213,	(int)(1.19*256), (int)(1.00*256)},
{213,	(int)(1.25*256), (int)(1.00*256)},
{213,	(int)(1.25*256), (int)(1.00*256)},
{213,	(int)(1.31*256), (int)(1.00*256)},
{213,	(int)(1.38*256), (int)(1.00*256)},
{213,	(int)(1.44*256), (int)(1.00*256)},
{213,	(int)(1.44*256), (int)(1.00*256)},
{213,	(int)(1.50*256), (int)(1.00*256)},
{213,	(int)(1.56*256), (int)(1.00*256)},
{213,	(int)(1.63*256), (int)(1.00*256)},
{213,	(int)(1.69*256), (int)(1.00*256)},
{213,	(int)(1.75*256), (int)(1.00*256)},
{213,	(int)(1.81*256), (int)(1.00*256)},
{213,	(int)(1.88*256), (int)(1.00*256)},
{213,	(int)(1.94*256), (int)(1.00*256)},
{426,	(int)(1.00*256), (int)(1.00*256)},
{426,	(int)(1.00*256), (int)(1.04*256)},
{426,	(int)(1.06*256), (int)(1.00*256)},
{426,	(int)(1.13*256), (int)(1.00*256)},
{426,	(int)(1.13*256), (int)(1.00*256)},
{426,	(int)(1.19*256), (int)(1.00*256)},
{426,	(int)(1.25*256), (int)(1.00*256)},
{426,	(int)(1.25*256), (int)(1.00*256)},
{426,	(int)(1.31*256), (int)(1.00*256)},
{426,	(int)(1.38*256), (int)(1.00*256)},
{426,	(int)(1.44*256), (int)(1.00*256)},
{426,	(int)(1.44*256), (int)(1.00*256)},
{426,	(int)(1.50*256), (int)(1.00*256)},
{426,	(int)(1.56*256), (int)(1.00*256)},
{426,	(int)(1.63*256), (int)(1.00*256)},
{426,	(int)(1.69*256), (int)(1.00*256)},
{426,	(int)(1.75*256), (int)(1.00*256)},
{426,	(int)(1.81*256), (int)(1.00*256)},
{426,	(int)(1.88*256), (int)(1.00*256)},
{426,	(int)(1.94*256), (int)(1.00*256)},
{639,	(int)(1.31*256), (int)(1.01*256)},
{639,	(int)(1.38*256), (int)(1.00*256)},
{639,	(int)(1.44*256), (int)(1.00*256)},
{639,	(int)(1.44*256), (int)(1.00*256)},
{639,	(int)(1.50*256), (int)(1.00*256)},
{639,	(int)(1.56*256), (int)(1.00*256)},
{639,	(int)(1.63*256), (int)(1.00*256)},
{639,	(int)(1.69*256), (int)(1.00*256)},
{639,	(int)(1.75*256), (int)(1.00*256)},
{639,	(int)(1.81*256), (int)(1.00*256)},
{639,	(int)(1.88*256), (int)(1.00*256)},
{639,	(int)(1.94*256), (int)(1.00*256)},
{852,	(int)(1.50*256), (int)(1.00*256)},
{852,	(int)(1.56*256), (int)(1.00*256)},
{852,	(int)(1.63*256), (int)(1.00*256)},
{852,	(int)(1.69*256), (int)(1.00*256)},
{852,	(int)(1.75*256), (int)(1.00*256)},
{852,	(int)(1.81*256), (int)(1.00*256)},
{852,	(int)(1.88*256), (int)(1.00*256)},
{852,	(int)(1.94*256), (int)(1.00*256)},
{852,	(int)(2.00*256), (int)(1.00*256)},
{852,	(int)(2.00*256), (int)(1.04*256)},
{852,	(int)(2.13*256), (int)(1.00*256)},
{852,	(int)(2.25*256), (int)(1.00*256)},
{852,	(int)(2.25*256), (int)(1.00*256)},
{852,	(int)(2.38*256), (int)(1.00*256)},
{852,	(int)(2.50*256), (int)(1.00*256)},
{852,	(int)(2.50*256), (int)(1.00*256)},
{852,	(int)(2.63*256), (int)(1.00*256)},
{852,	(int)(2.75*256), (int)(1.00*256)},
{852,	(int)(2.88*256), (int)(1.00*256)},
{852,	(int)(2.88*256), (int)(1.00*256)},
{852,	(int)(3.00*256), (int)(1.00*256)},
{852,	(int)(3.13*256), (int)(1.00*256)},
{852,	(int)(3.25*256), (int)(1.00*256)},
{852,	(int)(3.38*256), (int)(1.00*256)},
{852,	(int)(3.50*256), (int)(1.00*256)},
{852,	(int)(3.63*256), (int)(1.00*256)},
{852,	(int)(3.75*256), (int)(1.00*256)},
{852,	(int)(3.88*256), (int)(1.00*256)},
{852,	(int)(4.00*256), (int)(1.00*256)},
{852,	(int)(4.00*256), (int)(1.04*256)},
{852,	(int)(4.25*256), (int)(1.00*256)},
{852,	(int)(4.25*256), (int)(1.04*256)},
{852,	(int)(4.50*256), (int)(1.00*256)},
{852,	(int)(4.75*256), (int)(1.00*256)},
{852,	(int)(5.00*256), (int)(1.00*256)},
{852,	(int)(5.00*256), (int)(1.04*256)},
{852,	(int)(5.25*256), (int)(1.00*256)},
{852,	(int)(5.50*256), (int)(1.00*256)},
{852,	(int)(5.50*256), (int)(1.04*256)},
{852,	(int)(5.75*256), (int)(1.02*256)},
{852,	(int)(6.00*256), (int)(1.00*256)},
{852,	(int)(6.25*256), (int)(1.00*256)},
{852,	(int)(6.50*256), (int)(1.00*256)},
{852,	(int)(6.75*256), (int)(1.00*256)},
{852,	(int)(7.00*256), (int)(1.00*256)},
{852,	(int)(7.00*256), (int)(1.04*256)},
{852,	(int)(7.00*256), (int)(1.07*256)},
{852,	(int)(7.00*256), (int)(1.11*256)},
{852,	(int)(7.00*256), (int)(1.15*256)},
{852,	(int)(7.00*256), (int)(1.19*256)},
{852,	(int)(7.00*256), (int)(1.23*256)},
{852,	(int)(7.00*256), (int)(1.27*256)},
{852,	(int)(7.00*256), (int)(1.32*256)},
{852,	(int)(7.00*256), (int)(1.37*256)},
{852,	(int)(7.00*256), (int)(1.41*256)},
{852,	(int)(7.00*256), (int)(1.46*256)},
{852,	(int)(7.00*256), (int)(1.52*256)},
{852,	(int)(7.00*256), (int)(1.57*256)},
{852,	(int)(7.00*256), (int)(1.62*256)},
{852,	(int)(7.00*256), (int)(1.68*256)},
{852,	(int)(7.00*256), (int)(1.74*256)},
{852,	(int)(7.00*256), (int)(1.80*256)},
{852,	(int)(7.00*256), (int)(1.87*256)},
{852,	(int)(7.00*256), (int)(1.93*256)},
{852,	(int)(7.00*256), (int)(2.00*256)},
{852,	(int)(7.25*256), (int)(2.00*256)},
{852,	(int)(7.50*256), (int)(2.00*256)},
{852,	(int)(7.75*256), (int)(2.00*256)},
{852,	(int)(8.00*256), (int)(2.00*256)}
};

static const  int g_OV9712_exp_time_gain_50Hz[OV9712_50HZ_EXP_TIME_TOTAL][3] = 
{ // {time, analog gain, digital gain}
{8, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{9, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{10, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{11, (int)(1.00*256), (int)(1.00*256)},
{12, (int)(1.00*256), (int)(1.00*256)},
{12, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{13, (int)(1.00*256), (int)(1.00*256)},
{14, (int)(1.00*256), (int)(1.00*256)},
{14, (int)(1.00*256), (int)(1.00*256)},
{15, (int)(1.00*256), (int)(1.00*256)},
{15, (int)(1.00*256), (int)(1.00*256)},
{16, (int)(1.00*256), (int)(1.00*256)},
{17, (int)(1.00*256), (int)(1.00*256)},
{17, (int)(1.00*256), (int)(1.00*256)},
{18, (int)(1.00*256), (int)(1.00*256)},
{18, (int)(1.00*256), (int)(1.00*256)},
{19, (int)(1.00*256), (int)(1.00*256)},
{20, (int)(1.00*256), (int)(1.00*256)},
{20, (int)(1.00*256), (int)(1.00*256)},
{21, (int)(1.00*256), (int)(1.00*256)},
{22, (int)(1.00*256), (int)(1.00*256)},
{23, (int)(1.00*256), (int)(1.00*256)},
{23, (int)(1.00*256), (int)(1.00*256)},
{24, (int)(1.00*256), (int)(1.00*256)},
{25, (int)(1.00*256), (int)(1.00*256)},
{26, (int)(1.00*256), (int)(1.00*256)},
{27, (int)(1.00*256), (int)(1.00*256)},
{28, (int)(1.00*256), (int)(1.00*256)},
{29, (int)(1.00*256), (int)(1.00*256)},
{30, (int)(1.00*256), (int)(1.00*256)},
{31, (int)(1.00*256), (int)(1.00*256)},
{32, (int)(1.00*256), (int)(1.00*256)},
{33, (int)(1.00*256), (int)(1.00*256)},
{34, (int)(1.00*256), (int)(1.00*256)},
{36, (int)(1.00*256), (int)(1.00*256)},
{37, (int)(1.00*256), (int)(1.00*256)},
{38, (int)(1.00*256), (int)(1.00*256)},
{39, (int)(1.00*256), (int)(1.00*256)},
{41, (int)(1.00*256), (int)(1.00*256)},
{42, (int)(1.00*256), (int)(1.00*256)},
{44, (int)(1.00*256), (int)(1.00*256)},
{45, (int)(1.00*256), (int)(1.00*256)},
{47, (int)(1.00*256), (int)(1.00*256)},
{49, (int)(1.00*256), (int)(1.00*256)},
{50, (int)(1.00*256), (int)(1.00*256)},
{52, (int)(1.00*256), (int)(1.00*256)},
{54, (int)(1.00*256), (int)(1.00*256)},
{56, (int)(1.00*256), (int)(1.00*256)},
{58, (int)(1.00*256), (int)(1.00*256)},
{60, (int)(1.00*256), (int)(1.00*256)},
{62, (int)(1.00*256), (int)(1.00*256)},
{64, (int)(1.00*256), (int)(1.00*256)},
{66, (int)(1.00*256), (int)(1.00*256)},
{69, (int)(1.00*256), (int)(1.00*256)},
{71, (int)(1.00*256), (int)(1.00*256)},
{74, (int)(1.00*256), (int)(1.00*256)},
{76, (int)(1.00*256), (int)(1.00*256)},
{79, (int)(1.00*256), (int)(1.00*256)},
{82, (int)(1.00*256), (int)(1.00*256)},
{84, (int)(1.00*256), (int)(1.00*256)},
{87, (int)(1.00*256), (int)(1.00*256)},
{91, (int)(1.00*256), (int)(1.00*256)},
{94, (int)(1.00*256), (int)(1.00*256)},
{97, (int)(1.00*256), (int)(1.00*256)},
{100, (int)(1.00*256), (int)(1.00*256)},
{104, (int)(1.00*256), (int)(1.00*256)},
{108, (int)(1.00*256), (int)(1.00*256)},
{111, (int)(1.00*256), (int)(1.00*256)},
{115, (int)(1.00*256), (int)(1.00*256)},
{119, (int)(1.00*256), (int)(1.00*256)},
{124, (int)(1.00*256), (int)(1.00*256)},
{128, (int)(1.00*256), (int)(1.00*256)},
{133, (int)(1.00*256), (int)(1.00*256)},
{137, (int)(1.00*256), (int)(1.00*256)},
{142, (int)(1.00*256), (int)(1.00*256)},
{147, (int)(1.00*256), (int)(1.00*256)},
{152, (int)(1.00*256), (int)(1.00*256)},
{158, (int)(1.00*256), (int)(1.00*256)},
{163, (int)(1.00*256), (int)(1.00*256)},
{169, (int)(1.00*256), (int)(1.00*256)},
{175, (int)(1.00*256), (int)(1.00*256)},
{181, (int)(1.00*256), (int)(1.00*256)},
{187, (int)(1.00*256), (int)(1.00*256)},
{194, (int)(1.00*256), (int)(1.00*256)},
{201, (int)(1.00*256), (int)(1.00*256)},
{208, (int)(1.00*256), (int)(1.00*256)},
{215, (int)(1.00*256), (int)(1.00*256)},
{223, (int)(1.00*256), (int)(1.00*256)},
{231, (int)(1.00*256), (int)(1.00*256)},
{239, (int)(1.00*256), (int)(1.00*256)},
{247, (int)(1.00*256), (int)(1.00*256)},
{256, (int)(1.00*256), (int)(1.00*256)},
{256, (int)(1.00*256), (int)(1.04*256)},
{256, (int)(1.06*256), (int)(1.00*256)},
{256, (int)(1.13*256), (int)(1.00*256)},
{256, (int)(1.13*256), (int)(1.00*256)},
{256, (int)(1.19*256), (int)(1.00*256)},
{256, (int)(1.25*256), (int)(1.00*256)},
{256, (int)(1.25*256), (int)(1.00*256)},
{256, (int)(1.31*256), (int)(1.00*256)},
{256, (int)(1.38*256), (int)(1.00*256)},
{256, (int)(1.44*256), (int)(1.00*256)},
{256, (int)(1.44*256), (int)(1.00*256)},
{256, (int)(1.50*256), (int)(1.00*256)},
{256, (int)(1.56*256), (int)(1.00*256)},
{256, (int)(1.63*256), (int)(1.00*256)},
{256, (int)(1.69*256), (int)(1.00*256)},
{256, (int)(1.75*256), (int)(1.00*256)},
{256, (int)(1.81*256), (int)(1.00*256)},
{256, (int)(1.88*256), (int)(1.00*256)},
{256, (int)(1.94*256), (int)(1.00*256)},
{512, (int)(1.00*256), (int)(1.00*256)},
{512, (int)(1.00*256), (int)(1.04*256)},
{512, (int)(1.06*256), (int)(1.00*256)},
{512, (int)(1.13*256), (int)(1.00*256)},
{512, (int)(1.13*256), (int)(1.00*256)},
{512, (int)(1.19*256), (int)(1.00*256)},
{512, (int)(1.25*256), (int)(1.00*256)},
{512, (int)(1.25*256), (int)(1.00*256)},
{512, (int)(1.31*256), (int)(1.00*256)},
{512, (int)(1.38*256), (int)(1.00*256)},
{512, (int)(1.44*256), (int)(1.00*256)},
{512, (int)(1.44*256), (int)(1.00*256)},
{512, (int)(1.50*256), (int)(1.00*256)},
{512, (int)(1.56*256), (int)(1.00*256)},
{512, (int)(1.63*256), (int)(1.00*256)},
{512, (int)(1.69*256), (int)(1.00*256)},
{512, (int)(1.75*256), (int)(1.00*256)},
{512, (int)(1.81*256), (int)(1.00*256)},
{512, (int)(1.88*256), (int)(1.00*256)},
{512, (int)(1.94*256), (int)(1.00*256)},
{768, (int)(1.31*256), (int)(1.00*256)},
{768, (int)(1.38*256), (int)(1.00*256)},
{768, (int)(1.44*256), (int)(1.00*256)},
{768, (int)(1.44*256), (int)(1.00*256)},
{768, (int)(1.50*256), (int)(1.00*256)},
{768, (int)(1.56*256), (int)(1.00*256)},
{768, (int)(1.63*256), (int)(1.00*256)},
{768, (int)(1.69*256), (int)(1.00*256)},
{768, (int)(1.75*256), (int)(1.00*256)},
{768, (int)(1.81*256), (int)(1.00*256)},
{768, (int)(1.88*256), (int)(1.00*256)},
{768, (int)(1.94*256), (int)(1.00*256)},
{768, (int)(2.0*256), (int)(1.00*256)},
{768, (int)(2.00*256), (int)(1.04*256)},
{768, (int)(2.13*256), (int)(1.00*256)},
{768, (int)(2.25*256), (int)(1.00*256)},
{768, (int)(2.25*256), (int)(1.00*256)},
{768, (int)(2.38*256), (int)(1.00*256)},
{768, (int)(2.50*256), (int)(1.00*256)},
{768, (int)(2.50*256), (int)(1.00*256)},
{768, (int)(2.63*256), (int)(1.00*256)},
{768, (int)(2.75*256), (int)(1.00*256)},
{768, (int)(2.88*256), (int)(1.00*256)},
{768, (int)(2.88*256), (int)(1.00*256)},
{768, (int)(3.00*256), (int)(1.00*256)},
{768, (int)(3.13*256), (int)(1.00*256)},
{768, (int)(3.25*256), (int)(1.00*256)},
{768, (int)(3.38*256), (int)(1.00*256)},
{768, (int)(3.50*256), (int)(1.00*256)},
{768, (int)(3.63*256), (int)(1.00*256)},
{768, (int)(3.75*256), (int)(1.00*256)},
{768, (int)(3.88*256), (int)(1.00*256)},
{768, (int)(4.00*256), (int)(1.00*256)},
{768, (int)(4.00*256), (int)(1.04*256)},
{768, (int)(4.25*256), (int)(1.00*256)},
{768, (int)(4.25*256), (int)(1.04*256)},
{768, (int)(4.50*256), (int)(1.00*256)},
{768, (int)(4.75*256), (int)(1.00*256)},
{768, (int)(5.00*256), (int)(1.00*256)},
{768, (int)(5.00*256), (int)(1.04*256)},
{768, (int)(5.25*256), (int)(1.00*256)},
{768, (int)(5.50*256), (int)(1.00*256)},
{768, (int)(5.50*256), (int)(1.04*256)},
{768, (int)(5.75*256), (int)(1.02*256)},
{768, (int)(6.00*256), (int)(1.00*256)},
{768, (int)(6.25*256), (int)(1.00*256)},
{768, (int)(6.50*256), (int)(1.00*256)},
{768, (int)(6.75*256), (int)(1.00*256)},
{768, (int)(7.00*256), (int)(1.00*256)},
{768, (int)(7.00*256), (int)(1.04*256)},
{768, (int)(7.00*256), (int)(1.07*256)},
{768, (int)(7.00*256), (int)(1.11*256)},
{768, (int)(7.00*256), (int)(1.15*256)},
{768, (int)(7.00*256), (int)(1.19*256)},
{768, (int)(7.00*256), (int)(1.23*256)},
{768, (int)(7.00*256), (int)(1.27*256)},
{768, (int)(7.00*256), (int)(1.32*256)},
{768, (int)(7.00*256), (int)(1.37*256)},
{768, (int)(7.00*256), (int)(1.41*256)},
{768, (int)(7.00*256), (int)(1.46*256)},
{768, (int)(7.00*256), (int)(1.52*256)},
{768, (int)(7.00*256), (int)(1.57*256)},
{768, (int)(7.00*256), (int)(1.62*256)},
{768, (int)(7.00*256), (int)(1.68*256)},
{768, (int)(7.00*256), (int)(1.74*256)},
{768, (int)(7.00*256), (int)(1.80*256)},
{768, (int)(7.00*256), (int)(1.87*256)},
{768, (int)(7.00*256), (int)(1.93*256)},
{768, (int)(7.00*256), (int)(2.00*256)},
{768, (int)(7.25*256), (int)(2.00*256)},
{768, (int)(7.50*256), (int)(2.00*256)},
{768, (int)(7.75*256), (int)(2.00*256)},
{768, (int)(8.00*256), (int)(2.00*256)}
};

static const int OV9712_analog_gain_table[65] = 
{
	// coarse gain = 0
	(int)(1.00*256+0.5), (int)(1.06*256+0.5), (int)(1.13*256+0.5), (int)(1.19*256+0.5), 
	(int)(1.25*256+0.5), (int)(1.31*256+0.5), (int)(1.38*256+0.5), (int)(1.44*256+0.5), 
	(int)(1.50*256+0.5), (int)(1.56*256+0.5), (int)(1.63*256+0.5), (int)(1.69*256+0.5), 
	(int)(1.75*256+0.5), (int)(1.81*256+0.5), (int)(1.88*256+0.5), (int)(1.94*256+0.5),
	
	// coarse gain = 1
	(int)(2.00*256+0.5), (int)(2.13*256+0.5), (int)(2.25*256+0.5), (int)(2.38*256+0.5), 
	(int)(2.50*256+0.5), (int)(2.63*256+0.5), (int)(2.75*256+0.5), (int)(2.88*256+0.5),
	(int)(3.00*256+0.5), (int)(3.13*256+0.5), (int)(3.25*256+0.5), (int)(3.38*256+0.5),
	(int)(3.50*256+0.5), (int)(3.63*256+0.5), (int)(3.75*256+0.5), (int)(3.88*256+0.5),

	// coarse gain = 3
	(int)(4.00*256+0.5), (int)(4.25*256+0.5), (int)(4.50*256+0.5), (int)(4.75*256+0.5), 
	(int)(5.00*256+0.5), (int)(5.25*256+0.5), (int)(5.50*256+0.5), (int)(5.75*256+0.5), 
	(int)(6.00*256+0.5), (int)(6.25*256+0.5), (int)(6.50*256+0.5), (int)(6.75*256+0.5), 
	(int)(7.00*256+0.5), (int)(7.25*256+0.5), (int)(7.50*256+0.5), (int)(7.75*256+0.5), 
	
	// coarse gain = 7
	(int)(8.00*256+0.5), (int)(8.50*256+0.5), (int)(9.00*256+0.5), (int)(9.50*256+0.5), 
	(int)(10.00*256+0.5),(int)(10.50*256+0.5),(int)(11.00*256+0.5),(int)(11.50*256+0.5), 
	(int)(12.00*256+0.5),(int)(12.50*256+0.5),(int)(13.00*256+0.5),(int)(13.50*256+0.5), 
	(int)(14.00*256+0.5),(int)(14.50*256+0.5),(int)(15.00*256+0.5),(int)(15.50*256+0.5), 
	
	// coarse gain = 15
	(int)(16.00*256+0.5)
};

static INT32S ov9712_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	ov9712_handle = drv_l2_sccb_open(OV9712_ID, 8, 8);
	if(ov9712_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	ov9712_handle.slaveAddr = OV9712_ID;
	ov9712_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void ov9712_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(ov9712_handle) {
		drv_l2_sccb_close(ov9712_handle);
		ov9712_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	ov9712_handle.slaveAddr = 0;
	ov9712_handle.clkRate = 0;
#endif
}

static INT32S ov9712_sccb_write(INT8U reg, INT8U value)
{
#if SCCB_MODE == SCCB_GPIO
	return drv_l2_sccb_write(ov9712_handle, reg, value);

#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[2];
	
	data[0] = reg;
	data[1] = value;
	return drv_l1_i2c_bus_write(&ov9712_handle, data, 2);
#endif
}

static INT32S ov9712_sccb_read(INT8U reg, INT8U *value)
{
#if SCCB_MODE == SCCB_GPIO
	INT16U data;
	
	if(drv_l2_sccb_read(ov9712_handle, reg, &data) >= 0) {
		*value = (INT8U)data;
		return STATUS_OK;
	} else {
		*value = 0xFF;
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	INT8U data[1];
	
	data[0] = reg;
	if(drv_l1_i2c_bus_write(&ov9712_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	if(drv_l1_i2c_bus_read(&ov9712_handle, data, 1) < 0) {
		*value = 0xFF;
		return STATUS_FAIL;
	}
	
	*value = data[0];
#endif
	return STATUS_OK;
}

static INT32S ov9712_sccb_write_table(regval8_t *pTable)
{
	while(1) {
		if(pTable->reg_num == 0xFF && pTable->value == 0xFF) {
			break;
		}
		
		if(ov9712_sccb_write(pTable->reg_num, pTable->value) < 0) {
			DBG_PRINT("sccb write fail.\r\n");
			continue;
		}
		pTable++;
	}
	return STATUS_OK;
}

static int OV9712_cvt_analog_gain(int analog_gain)
{
	int i;
	int coarse_gain, fine_gain;
	int *p_analog_gain_table = (int *)OV9712_analog_gain_table;

	for(i=0; i<65; i++)
	{
		if(analog_gain >= p_analog_gain_table[i] && analog_gain < p_analog_gain_table[i+1])
			break;
	} 

	if(i < 16) {
		coarse_gain = 0;
		fine_gain = i;
	}
	else if(i < (16 + 16)) {
		coarse_gain = 1;
		fine_gain = (i - 16); 
	}
	else if(i < (16 + 16 + 16)) {
		coarse_gain = 3;
		fine_gain = (i - 16 - 16);
	}
	else if(i < (16 + 16 + 16 + 16)) {
		coarse_gain = 7;
		fine_gain = (i - 16 - 16 -16);
	}
	else {
		coarse_gain = 15;
		fine_gain = 0;
	}

	return ((coarse_gain << 4) | fine_gain);
}

static int OV9712_get_real_analog_gain(int analog_gain)
{
	int real_analog_gain;
	int coarse_gain, fine_gain;
	int *p_analog_gain_table = (int *)OV9712_analog_gain_table;
	
	coarse_gain = (analog_gain >> 4) & 0x7;
	fine_gain = (analog_gain & 0xf);

	if(coarse_gain == 0) {
		real_analog_gain = p_analog_gain_table[fine_gain];
	}
	else if(coarse_gain == 1) {
		real_analog_gain = p_analog_gain_table[16 + fine_gain];
	}
	else if(coarse_gain == 3) {
		real_analog_gain = p_analog_gain_table[16 + 16 + fine_gain];
	}
	else if(coarse_gain == 7) {
		real_analog_gain = p_analog_gain_table[16 + 16 + 16 + fine_gain];
	}
	else if(coarse_gain == 15) {
		real_analog_gain = p_analog_gain_table[16 + 16 + 16 + 16];
	}
	else {
		DBG_PRINT("H22 analog gain Err!\r\n");
	}
	
	return real_analog_gain;
}

static INT32S ov9712_set_exposure_time(sensor_exposure_t *si)
{
	INT8U data;
	INT32S cvt_gain;
	
	// group latch on
	ov9712_sccb_read(0x04, &data);
	ov9712_sccb_write(0x04, data | 0x01);		
	
	// AGC Gain / analog gain	
	cvt_gain = OV9712_cvt_analog_gain(si->analog_gain);
	ov9712_sccb_write(0x00, cvt_gain);
	
	// exposure time
	ov9712_sccb_write(0x10, (si->time & 0xFF));
	ov9712_sccb_write(0x16, ((si->time >> 8) & 0xFF));
	
	// group latch off
	ov9712_sccb_read(0x04, &data);
	ov9712_sccb_write(0x04, data & 0xFE);	
	
	// group write trigger
	ov9712_sccb_write(0xff, 0xff);
	
	//DBG_PRINT("wtime = 0x%x, cvt_gain = 0x%x\r\n", si->time, cvt_gain);
	return STATUS_OK;			
}

static INT32S OV9712_set_xfps_exposure_time(sensor_exposure_t *si)
{
	int idx;
	
	si->sensor_ev_idx += si->ae_ev_idx;
	if(si->sensor_ev_idx >= si->max_ev_idx) {
		si->sensor_ev_idx = si->max_ev_idx - 1;
	}
	
	if(si->sensor_ev_idx < 0) {
		si->sensor_ev_idx = 0;
	}
	
	idx = si->sensor_ev_idx * 3;
	si ->time = p_expTime_table[idx];
	si ->analog_gain = p_expTime_table[idx+1];
	si ->digital_gain = p_expTime_table[idx+2];

	return ov9712_set_exposure_time(si);
}

static void ov9712_get_exposure_time(sensor_exposure_t *se)
{
	INT8U cvt_gain;
	INT8U t1, t2;
	
	// AGC Gain / analog gain	
	ov9712_sccb_read(0x00, &cvt_gain); 
	se->analog_gain = OV9712_get_real_analog_gain(cvt_gain);	

	// exposure time
	ov9712_sccb_read(0x10, &t1);
	ov9712_sccb_read(0x16, &t2);
	se->time = (t1 & 0xFF) | ((t2 & 0xFF) << 8);
	//DBG_PRINT("rtime = 0x%x, cvt_gain = 0x%x\r\n", se->time, cvt_gain);                          
}

static void ov9712_set_exp_freq(int freq)
{
	if(freq == 50)
	{
		ov9712_seInfo.sensor_ev_idx = OV9712_50HZ_INIT_EV_IDX;
		ov9712_seInfo.ae_ev_idx = 0;
		ov9712_seInfo.daylight_ev_idx= OV9712_50HZ_DAY_EV_IDX;
		ov9712_seInfo.night_ev_idx= OV9712_50HZ_NIGHT_EV_IDX;
		ov9712_seInfo.max_ev_idx = OV9712_50HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_OV9712_exp_time_gain_50Hz;
	}
	else if(freq == 60)
	{
		ov9712_seInfo.sensor_ev_idx = OV9712_60HZ_INIT_EV_IDX;
		ov9712_seInfo.ae_ev_idx = 0;
		ov9712_seInfo.daylight_ev_idx= OV9712_60HZ_DAY_EV_IDX;
		ov9712_seInfo.night_ev_idx= OV9712_60HZ_NIGHT_EV_IDX;
		ov9712_seInfo.max_ev_idx = OV9712_60HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_OV9712_exp_time_gain_60Hz;
	}
}

static INT32S ov9712_set_ctrl(gpCdspCtrl_t *ctrl)
{
	INT32S ret;
	
	switch(ctrl->id)
	{
	case V4L2_CID_BACKLIGHT_COMPENSATION:
		DBG_PRINT("NightMode = %d\n", ctrl->value);
		if(ctrl->value) {	// NIGH MODE ON
			
		} else {	// NIGH MODE OFF
			
		}
		break;
		
	case V4L2_CID_EXPOSURE:
		{
			sensor_exposure_t *si;
			
			si = (sensor_exposure_t *) ctrl->value;
			ov9712_seInfo.userISO = si->userISO;
			if(p_expTime_table != 0) {
				ov9712_seInfo.ae_ev_idx = si->ae_ev_idx;
				ret = OV9712_set_xfps_exposure_time(&ov9712_seInfo);
			} else {
				ov9712_seInfo.time = si->time;
				ov9712_seInfo.digital_gain = si->digital_gain >> 1;
				ov9712_seInfo.analog_gain = si->analog_gain;
				ret = ov9712_set_exposure_time(&ov9712_seInfo);
			}
		}
		break;
	
	case V4L2_CID_POWER_LINE_FREQUENCY:
		ov9712_set_exp_freq(ctrl->value);
		break;
	
	case V4L2_CID_VFLIP:
		{
			INT8U data;
			
			ov9712_sccb_read(0x04, &data);
			if(ctrl->value) {
				data |= (1 << 6);
			} else {
				data &= ~(1 << 6);
			}
			ov9712_sccb_write(0x04, data);
		}
		break;
		
	case V4L2_CID_HFLIP:
		{
			INT8U data;
			
			ov9712_sccb_read(0x04, &data);
			if(ctrl->value) {
				data |= (1 << 7);
			} else {
				data &= ~(1 << 7);
			}
			ov9712_sccb_write(0x04, data);
		}
		break; 
		
	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

static INT32S ov9712_get_ctrl(gpCdspCtrl_t *ctrl)
{
	switch(ctrl->id)
	{
	case V4L2_CID_EXPOSURE:
		ov9712_get_exposure_time(&ov9712_seInfo);
		ctrl->value = (int)&ov9712_seInfo;
		break;
		
	case V4L2_CID_BRIGHTNESS:
		//ctrl->value = 64 - (sensor_total_ev - ov9712_seInfo.max_ev_idx);
		break;	
	
	case V4L2_CID_VFLIP:
		{
			INT8U data;
			
			ov9712_sccb_read(0x04, &data);
			if(data & (1 << 6)) {
				ctrl->value = 1;
			} else {
				ctrl->value = 0;
			}
		}
		break;
		
	case V4L2_CID_HFLIP:
		{
			INT8U data;
			
			ov9712_sccb_read(0x04, &data);
			if(data & (1 << 7)) {
				ctrl->value = 1;
			} else {
				ctrl->value = 0;
			}
		}
		break;
	
	default:
		return STATUS_FAIL;		
	}

	return STATUS_OK;
}

static const gpCdspSensor_t ov9712_ctrl =
{
	ov9712_set_ctrl,
	ov9712_get_ctrl
};

/**
 * @brief   ov9712 initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void ov9712_cdsp_init(void)
{
	// ae/awb init
	ov9712_seInfo.max_time = OV9712_MAX_EXPOSURE_TIME;
	ov9712_seInfo.min_time = OV9712_MIN_EXPOSURE_TIME;

	ov9712_seInfo.max_digital_gain = OV9712_MAX_DIGITAL_GAIN ;
	ov9712_seInfo.min_digital_gain = OV9712_MIN_DIGITAL_GAIN ;

	ov9712_seInfo.max_analog_gain = OV9712_MAX_ANALOG_GAIN;
	ov9712_seInfo.min_analog_gain = OV9712_MIN_ANALOG_GAIN;

	ov9712_seInfo.analog_gain = ov9712_seInfo.min_analog_gain;
	ov9712_seInfo.digital_gain = ov9712_seInfo.min_digital_gain;
	ov9712_seInfo.time = ov9712_seInfo.min_time;
	ov9712_seInfo.userISO = ISO_AUTO;

	ov9712_set_exp_freq(60);
	
	// Turn on LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(1, LDO_LDO28_2P8V);
	
	// mclk output
	drv_l2_sensor_set_mclkout(ov9712_cdsp_ops.info[0].mclk);
	
	// request sccb
	ov9712_sccb_open();
			
	// reset sensor
	ov9712_sccb_write_table((regval8_t *)OV9712_SW_Reset);
	OSTimeDly(20); 
	
	// cdsp init
	drv_l2_cdsp_open();
	drv_l2_CdspTableRegister((sensor_calibration_t *)&ov9712_cdsp_calibration);
	drv_l2_CdspSensorCtrlRegister((gpCdspSensor_t *)&ov9712_ctrl);
	DBG_PRINT("Sensor ov9712 cdsp init completed\r\n");
}	

/**
 * @brief   ov9712 un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void ov9712_cdsp_uninit(void)
{
	// disable mclk 
	drv_l2_sensor_set_mclkout(MCLK_NONE);
	
	// cdsp disabale
	drv_l2_cdsp_close();
	
	// release sccb
	ov9712_sccb_close();
	
	// Turn off LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   ov9712 stream start function
 * @param   info index
 *			
 * @return 	none
 */
static void ov9712_cdsp_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h;
	gpCdspFmt_t format;
	
	// set sensor 
	DBG_PRINT("%s = %d\r\n", __func__, index);
	switch(index)
	{
	case 0:
		ov9712_sccb_write_table((regval8_t *)OV9712_1280_720_F30);
		break;
		
	case 1:
		ov9712_sccb_write_table((regval8_t *)OV9712_1280_800_F30);
		break;
		
	case 2:
	#if 1
		ov9712_sccb_write_table((regval8_t *)OV9712_1280_720_F30);
	#else	
		ov9712_sccb_write_table((regval8_t *)OV9712_640_480_F30);
	#endif	
		break;
		
	case 3:
		ov9712_sccb_write_table((regval8_t *)OV9712_640_400_F30);
		break;
		
	default:
		while(1);	
	}

	// mclk output
	drv_l2_sensor_set_mclkout(ov9712_cdsp_ops.info[index].mclk);

	// set cdsp format
	format.image_source = C_CDSP_FRONT;
	format.input_format = ov9712_cdsp_ops.info[index].input_format;
	format.output_format = ov9712_cdsp_ops.info[index].output_format;
	target_w = ov9712_cdsp_ops.info[index].target_w;
	target_h = ov9712_cdsp_ops.info[index].target_h;
	format.hpixel = ov9712_cdsp_ops.info[index].sensor_w;
	format.vline = ov9712_cdsp_ops.info[index].sensor_h;
	format.hoffset = ov9712_cdsp_ops.info[index].hoffset;
	format.voffset = ov9712_cdsp_ops.info[index].voffset;
	format.sensor_timing_mode = ov9712_cdsp_ops.info[index].interface_mode;
	format.sensor_hsync_mode = ov9712_cdsp_ops.info[index].hsync_active;
	format.sensor_vsync_mode = ov9712_cdsp_ops.info[index].vsync_active;
	if(drv_l2_cdsp_set_fmt(&format) < 0) {
		DBG_PRINT("cdsp set fmt err!!!\r\n");
	}
	
	// set scale down
	if((format.hpixel > target_w) || (format.vline > target_h)) {
		drv_l2_cdsp_set_yuv_scale(target_w, target_h);
	}
	
	// cdsp start
#if 0	
	drv_l2_cdsp_stream_on(DISABLE, bufA, bufB);
#else
	drv_l2_cdsp_stream_on(ENABLE, bufA, bufB);
	
	// set ae/awb
	drv_l2_cdsp_enable(target_w, target_h);
#endif	
}

/**
 * @brief   ov9712 stream stop function
 * @param   none
 * @return 	none
 */ 
static void ov9712_cdsp_stream_off(void)
{
	drv_l2_cdsp_stream_off();
}	

/**
 * @brief   ov9712 get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
static drv_l2_sensor_info_t* ov9712_cdsp_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)) {
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&ov9712_cdsp_ops.info[index];
	}
}

/*********************************************
*	sensor ops declaration
*********************************************/
const drv_l2_sensor_ops_t ov9712_cdsp_ops =
{
	SENSOR_OV9712_CDSP_NAME,			/* sensor name */
	ov9712_cdsp_init,
	ov9712_cdsp_uninit,
	ov9712_cdsp_stream_on,
	ov9712_cdsp_stream_off,
	ov9712_cdsp_get_info,
	{
		/* 1nd info */	
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SRBGB8,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */	
			1280,						/* target width */
			720, 						/* target height */		
			1280,						/* sensor width */
			720, 						/* sensor height */
			0x190,						/* sensor h offset */ 
			0x60,						/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 2nd info */	
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SRBGB8,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */	
			1280,						/* sensor width */
			800, 						/* sensor height */		
			1280,						/* sensor width */
			800, 						/* sensor height */
			0x1,						/* sensor h offset */ 
			0x1,						/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 3st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SRBGB8,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */	
			640,						/* target width */
			480, 						/* target height */		
		#if 1	
			1280,						/* sensor width */
			720, 						/* sensor height */
			0x190,						/* sensor h offset */ 
			0x60,						/* sensor v offset */
		#else
			640,						/* sensor width */
			480, 						/* sensor height */
			0x1C0,						/* sensor h offset */ 
			0x02,						/* sensor v offset */
		#endif	
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		},
		/* 4nd info */	
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SRBGB8,		/* input format */
			V4L2_PIX_FMT_VYUY,			/* output format */
			30,							/* FPS in sensor */			
			640,						/* target width */
			400, 						/* target height */
			640,						/* sensor width */
			400, 						/* sensor height */
			0x1,						/* sensor h offset */ 
			0x1,						/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_LOW,			/* vsync pin active level */
		}
	}
};
#endif //(defined _SENSOR_OV9712_CDSP) && (_SENSOR_OV9712_CDSP == 1)
