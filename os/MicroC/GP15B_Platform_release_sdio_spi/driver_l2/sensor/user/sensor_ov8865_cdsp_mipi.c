#include "driver_l1.h"
#include "drv_l1_power.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_i2c.h"
#include "drv_l1_csi.h"
#include "drv_l1_mipi.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_front.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_cdsp_config.h"
#include "drv_l2_sensor.h"
#include "drv_l2_sccb.h"
#include "gp_aeawb.h"
#include "sensor_ov8865_cdsp_temp.h"
#define OV8865_SLAVE_ID		0x20

#define GRP_WR				1
#define SENSOR_FLIP			1

//max resolution 1296*742
#define ov8865_RAW					0x01
#define ov8865_MIPI					0x02


//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined _SENSOR_OV8865_CDSP_MIPI) && (_SENSOR_OV8865_CDSP_MIPI == 1)
//================================================================//



/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//#define GC_48MHZ							800
//#define GC_42MHZ							810
//#define GC_PCLK								GC_48MHZ

#define ov8865_MAX_EXPOSURE_TIME			(1051)
#define ov8865_MIN_EXPOSURE_TIME			(12)

#define ov8865_MAX_ANALOG_GAIN				(8*100)
#define ov8865_MIN_ANALOG_GAIN				(1*100)

#define ov8865_MAX_DIGITAL_GAIN				(140)//(1.4*100)
#define ov8865_MIN_DIGITAL_GAIN 			(140)//0x100:1x //SENSOR_ov8865

#define ov8865_50HZ_EXP_TIME_TOTAL			184
#define	ov8865_50HZ_INIT_EV_IDX				130
#define ov8865_50HZ_NIGHT_EV_IDX				156
#define ov8865_50HZ_DAY_EV_IDX				96//200//120//202//56
#define ov8865_50HZ_MAX_EXP_IDX				(ov8865_50HZ_EXP_TIME_TOTAL-6)//31)  // <= ov8865_50HZ_EXP_TIME_TOTAL

#define ov8865_60HZ_EXP_TIME_TOTAL			180
#define	ov8865_60HZ_INIT_EV_IDX				122
#define ov8865_60HZ_NIGHT_EV_IDX				149
#define ov8865_60HZ_DAY_EV_IDX				96
#define ov8865_60HZ_MAX_EXP_IDX				(ov8865_60HZ_EXP_TIME_TOTAL-1)  // <= ov8865_60HZ_EXP_TIME_TOTAL

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
#define TwoByte(x) x //(x&0xFF),((x>>8)&0xFF)

 typedef struct regval16_t 
{
	INT16U reg_num;
	INT16U value;
} regval16_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
//INT32U GC_CLK	= GC_48MHZ;

#if SCCB_MODE == SCCB_GPIO
	static void *ov8865_handle;
#elif SCCB_MODE == SCCB_HW_I2C
	static drv_l1_i2c_bus_handle_t ov8865_handle; 
#endif

static sensor_exposure_t ov8865_seInfo;
//static sensor_calibration_t ov8865_cdsp_calibration;
//static INT32U ov8865_digital_gain = 0x180; //SENSOR_ov8865
//static INT32U ov8865_analog_gain = 0x100;
static int *p_expTime_table = 0;
//static int sensor_max_ev_idx;
static gpCdspWBGain_t ov8865_wbgain;
//static INT32U sensor_format = ov8865_MIPI;

static INT8U ov8865_read_en;
extern INT32U g_cdsp_rawcal_mode;

#if CAPTURE_RAW_MODE
extern INT32U g_frame_addr1;
#endif

static gpCdspFav_t myFav;

static mipi_config_t ov8865_mipi_cfg =
{
	DISABLE, 			/* low power, 0:disable, 1:enable */
	D_PHY_SAMPLE_NEG, 	/* byte clock edge, 0:posedge, 1:negedge */
	NULL,				/* RSD 0 */
	NULL,				/* RSD 1 */
	
	MIPI_AUTO_DETECT,	/* data mode, 0:auto detect, 1:user define */
	MIPI_RAW10,			/* data type, valid when data mode is 1*/
	MIPI_DATA_TO_CDSP,	/* data type, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */  
	NULL,				/* RSD 2 */
	
	0x660,				/* width, 0~0xFFFF */
	0x4C0,//960+4,				/* height, 0~0xFFFF */
	4, 					/* back porch, 0~0xF */
	4,					/* front porch, 0~0xF */
	ENABLE,				/* blanking_line_en, 0:disable, 1:enable */
	NULL,				/* RSD 3 */
	
	ENABLE,				/* ecc, 0:disable, 1:enable */
	MIPI_ECC_ORDER3,	/* ecc order */
	100,				/* data mask, unit is ns */
	MIPI_CHECK_HS_SEQ	/* check hs sequence or LP00 for clock lane */
};


static INT16U OV8865_1280_960_F30[][2] =	
{

	{0x0103,0x01},
	{0x0100,0x00},
	{0x0100,0x00},
	{0x0100,0x00},
	{0x0100,0x00},
	{0x3638,0xff},
	{0x0302,0x1e},
	{0x0303,0x00},
	{0x0304,0x03},//3
	{0x030d,0x1e},
	{0x030e,0x00},//0
	{0x030f,0x09},
	{0x0312,0x01},
	{0x031e,0x0c},//
	{0x3015,0x01},
	{0x3018,0x12},//mipi sc 
	{0x3020,0x93},
	{0x3022,0x01},
	//{0x3030,0x10},
	{0x3031,0x0A},//bit mode
	{0x3106,0x01},
	{0x3305,0xf1},
	{0x3308,0x00},
	{0x3309,0x28},
	{0x330a,0x00},
	{0x330b,0x20},
	{0x330c,0x00},
	{0x330d,0x00},
	{0x330e,0x00},
	{0x330f,0x40},
	{0x3307,0x04},
	{0x3604,0x04},
	{0x3602,0x30},
	{0x3605,0x00},
	{0x3607,0x20},
	{0x3608,0x11},
	{0x3609,0x68},
	{0x360a,0x40},
	{0x360c,0xdd},
	{0x360e,0x0c},
	{0x3610,0x07},
	{0x3612,0x86},
	{0x3613,0x58},
	{0x3614,0x28},
	{0x3617,0x40},
	{0x3618,0x5a},
	{0x3619,0x9b},
	{0x361c,0x00},
	{0x361d,0x60},
	{0x3631,0x60},
	{0x3633,0x10},
	{0x3634,0x10},
	{0x3635,0x10},
	{0x3636,0x10},
	{0x3641,0x55},
	{0x3646,0x86},//
	{0x3647,0x27},
	{0x364a,0x1b},
	{0x3500,0x00},//exposure[19:16]
	{0x3501,0x4c},//exposure[15:8]
	{0x3502,0x00},//exposure[7:0]
	{0x3503,0x03},//exposure not delay
	{0x3508,0x02},//Gain[12:8]
	{0x3509,0x00},//Gain[7:0],  0x350A[13:6],0x350B[5:0] is Digi gain
	{0x3700,0x24},
	{0x3701,0x0c},
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3705,0x00},
	{0x3706,0x38},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x40},
	{0x370a,0x00},
	{0x370b,0xb8},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3719,0x31},
	{0x3712,0x42},
	{0x3714,0x12},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x02},
	{0x3725,0x02},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3733,0x10},
	{0x3734,0x40},
	{0x3736,0x20},
	{0x373a,0x02},
	{0x373b,0x0c},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x3755,0x40},
	{0x3758,0x00},
	{0x3759,0x4c},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375c,0x40},
	{0x375d,0x02},
	{0x375e,0x00},
	{0x375f,0x14},
	{0x3767,0x1c},
	{0x3768,0x04},
	{0x3769,0x20},
	{0x376c,0xc0},
	{0x376d,0xc0},
	{0x376a,0x08},
	{0x3761,0x00},
	{0x3762,0x00},
	{0x3763,0x00},
	{0x3766,0xff},
	{0x376b,0x42},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x01},
	{0x37a4,0x00},
	{0x37a5,0x08},
	{0x37a6,0x00},
	{0x37a7,0x44},
	{0x37a8,0x58},
	{0x37a9,0x58},
	{0x3760,0x00},
	{0x376f,0x01},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b0,0x00},
	{0x37b1,0x00},
	{0x37b2,0x00},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x33},
	{0x37b6,0x00},
	{0x37b7,0x00},
	{0x37b8,0x00},
	{0x37b9,0xff},
	{0x3800,0x00},//xs
	{0x3801,0x0c},
	{0x3802,0x00},//ys
	{0x3803,0x0c},
	{0x3804,0x0c},//xe
	{0x3805,0xd3},
	{0x3806,0x09},//ye
	{0x3807,0xa3},
	{0x3808,0x06},//xo 660
	{0x3809,0x60},
	{0x380a,0x04},//yo 4c8
	{0x380b,0xc8},
	{0x380c,0x07},//hts
	{0x380d,0x80},
	{0x380e,0x04},//vts
	{0x380f,0xE0},
	{0x3810,0x00},//offsetx
	{0x3811,0x04},
	{0x3813,0x00},//offsety lsb
	{0x3814,0x03},
	{0x3815,0x01},
#if SENSOR_FLIP == 1
	{0x3820,0x02},
#else
	{0x3820,0x00},
#endif
	{0x3821,0x67},
	{0x382a,0x03},
	{0x382b,0x01},
	{0x3830,0x08},
	{0x3836,0x02},
	{0x3837,0x18},
	{0x3841,0xff},
	{0x3846,0x88},
	{0x3d85,0x06},
	{0x3d8c,0x75},
	{0x3d8d,0xef},
	{0x3f08,0x0b},
	{0x4000,0xf1},
	{0x4001,0x14},
	{0x4005,0x10},
	{0x400b,0x0c},
	{0x400d,0x10},
	{0x401b,0x00},
	{0x401d,0x00},
	{0x4020,0x01},
	{0x4021,0x20},
	{0x4022,0x01},
	{0x4023,0x9f},
	{0x4024,0x03},
	{0x4025,0xe0},
	{0x4026,0x04},
	{0x4027,0x5f},
	{0x4028,0x00},
	{0x4029,0x02},
	{0x402a,0x04},
	{0x402b,0x04},
	{0x402c,0x02},
	{0x402d,0x02},
	{0x402e,0x08},
	{0x402f,0x02},
	{0x401f,0x00},
	{0x4034,0x3f},
	{0x4300,0xff},
	{0x4301,0x00},
	{0x4302,0x0f},
	{0x4500,0x40},
	{0x4503,0x10},
	{0x4601,0x74},
	{0x481f,0x32},
	{0x4837,0x16},
	{0x4850,0x10},
	{0x4851,0x32},
	{0x4b00,0x2a},
	{0x4b0d,0x00},
	{0x4d00,0x04},
	{0x4d01,0x18},
	{0x4d02,0xc3},
	{0x4d03,0xff},
	{0x4d04,0xff},
	{0x4d05,0xff},
	{0x5000,0x16},//DSP 16
	{0x5001,0x01},//01
	{0x5002,0x08},//08
	{0x5901,0x00},
	{0x5e00,0x00},//test pattern mode
	{0x5e01,0x41},
	{0x5b00,0x02},//OTP DPC
	{0x5b01,0xd0},
	{0x5b02,0x03},
	{0x5b03,0xff},
	{0x5b05,0x6c},
	{0x5780,0xfc},//DPC control
	{0x5781,0xdf},
	{0x5782,0x3f},
	{0x5783,0x08},
	{0x5784,0x0c},
	{0x5786,0x20},
	{0x5787,0x40},
	{0x5788,0x08},
	{0x5789,0x08},
	{0x578a,0x02},
	{0x578b,0x01},
	{0x578c,0x01},
	{0x578d,0x0c},
	{0x578e,0x02},
	{0x578f,0x01},
	{0x5790,0x01},
	{0x0100,0x01}

};


static const  int g_ov8865_exp_time_gain_50Hz[ov8865_50HZ_EXP_TIME_TOTAL][3] =  //203,recount by u2
{ // {time, analog gain, digital gain}
//{6	,(int)(1.00*256), (int)(1.00*255)},
{11	,(int)(1.00*256), (int)(1.00*256)},
{12	,(int)(1.00*256), (int)(1.00*256)},
{12	,(int)(1.00*256), (int)(1.00*256)},
{13	,(int)(1.00*256), (int)(1.00*256)},
{13	,(int)(1.00*256), (int)(1.00*256)},
{13	,(int)(1.00*256), (int)(1.00*256)},
{14	,(int)(1.00*256), (int)(1.00*256)},
{14	,(int)(1.00*256), (int)(1.00*256)},
{15	,(int)(1.00*256), (int)(1.00*256)},
{15	,(int)(1.00*256), (int)(1.00*256)},
{16	,(int)(1.00*256), (int)(1.00*256)},
{17	,(int)(1.00*256), (int)(1.00*256)},
{17	,(int)(1.00*256), (int)(1.00*256)},
{18	,(int)(1.00*256), (int)(1.00*256)},
{18	,(int)(1.00*256), (int)(1.00*256)},
{19	,(int)(1.00*256), (int)(1.00*256)},
{20	,(int)(1.00*256), (int)(1.00*256)},
{20	,(int)(1.00*256), (int)(1.00*256)},
{21	,(int)(1.00*256), (int)(1.00*256)},
{22	,(int)(1.00*256), (int)(1.00*256)},
{23	,(int)(1.00*256), (int)(1.00*256)},
{23	,(int)(1.00*256), (int)(1.00*256)},
{24	,(int)(1.00*256), (int)(1.00*256)},
{25	,(int)(1.00*256), (int)(1.00*256)},
{26	,(int)(1.00*256), (int)(1.00*256)},
{27	,(int)(1.00*256), (int)(1.00*256)},
{28	,(int)(1.00*256), (int)(1.00*256)},
{29	,(int)(1.00*256), (int)(1.00*256)},
{30	,(int)(1.00*256), (int)(1.00*256)},
{31	,(int)(1.00*256), (int)(1.00*256)},
{32	,(int)(1.00*256), (int)(1.00*256)},
{33	,(int)(1.00*256), (int)(1.00*256)},
{34	,(int)(1.00*256), (int)(1.00*256)},
{36	,(int)(1.00*256), (int)(1.00*256)},
{37	,(int)(1.00*256), (int)(1.00*256)},
{38	,(int)(1.00*256), (int)(1.00*256)},
{39	,(int)(1.00*256), (int)(1.00*256)},
{41	,(int)(1.00*256), (int)(1.00*256)},
{42	,(int)(1.00*256), (int)(1.00*256)},
{44	,(int)(1.00*256), (int)(1.00*256)},
{45	,(int)(1.00*256), (int)(1.00*256)},
{47	,(int)(1.00*256), (int)(1.00*256)},
{49	,(int)(1.00*256), (int)(1.00*256)},
{50	,(int)(1.00*256), (int)(1.00*256)},
{52	,(int)(1.00*256), (int)(1.00*256)},
{54	,(int)(1.00*256), (int)(1.00*256)},
{56	,(int)(1.00*256), (int)(1.00*256)},
{58	,(int)(1.00*256), (int)(1.00*256)},
{60	,(int)(1.00*256), (int)(1.00*256)},
{62	,(int)(1.00*256), (int)(1.00*256)},
{64	,(int)(1.00*256), (int)(1.00*256)},
{66	,(int)(1.00*256), (int)(1.00*256)},
{69	,(int)(1.00*256), (int)(1.00*256)},
{71	,(int)(1.00*256), (int)(1.00*256)},
{74	,(int)(1.00*256), (int)(1.00*256)},
{76	,(int)(1.00*256), (int)(1.00*256)},
{79	,(int)(1.00*256), (int)(1.00*256)},
{82	,(int)(1.00*256), (int)(1.00*256)},
{85	,(int)(1.00*256), (int)(1.00*256)},
{88	,(int)(1.00*256), (int)(1.00*256)},
{91	,(int)(1.00*256), (int)(1.00*256)},
{94	,(int)(1.00*256), (int)(1.00*256)},
{97	,(int)(1.00*256), (int)(1.00*256)},
{101,	(int)(1.00*256), (int)(1.00*256)},
{104,	(int)(1.00*256), (int)(1.00*256)},
{108,	(int)(1.00*256), (int)(1.00*256)},
{112,	(int)(1.00*256), (int)(1.00*256)},
{116,	(int)(1.00*256), (int)(1.00*256)},
{120,	(int)(1.00*256), (int)(1.00*256)},
{124,	(int)(1.00*256), (int)(1.00*256)},
{128,	(int)(1.00*256), (int)(1.00*256)},
{133,	(int)(1.00*256), (int)(1.00*256)},
{137,	(int)(1.00*256), (int)(1.00*256)},
{142,	(int)(1.00*256), (int)(1.00*256)},
{147,	(int)(1.00*256), (int)(1.00*256)},
{153,	(int)(1.00*256), (int)(1.00*256)},
{158,	(int)(1.00*256), (int)(1.00*256)},
{163,	(int)(1.00*256), (int)(1.00*256)},
{169,	(int)(1.00*256), (int)(1.00*256)},
{175,	(int)(1.00*256), (int)(1.00*256)},
{181,	(int)(1.00*256), (int)(1.00*256)},
{188,	(int)(1.00*256), (int)(1.00*256)},
{194,	(int)(1.00*256), (int)(1.00*256)},
{201,	(int)(1.00*256), (int)(1.00*256)},
{208,	(int)(1.00*256), (int)(1.00*256)},
{216,	(int)(1.00*256), (int)(1.00*256)},
{223,	(int)(1.00*256), (int)(1.00*256)},
{231,	(int)(1.00*256), (int)(1.00*256)},
{239,	(int)(1.00*256), (int)(1.00*256)},
{248,	(int)(1.00*256), (int)(1.00*256)},
{257,	(int)(1.00*256), (int)(1.00*256)},
{266,	(int)(1.00*256), (int)(1.00*256)},
{275,	(int)(1.00*256), (int)(1.00*256)},
{285,	(int)(1.00*256), (int)(1.00*256)},
{295,	(int)(1.00*256), (int)(1.00*256)},
{305,	(int)(1.00*256), (int)(1.00*256)},
{316,	(int)(1.00*256), (int)(1.00*256)},
{327,	(int)(1.00*256), (int)(1.00*256)},
{338,	(int)(1.00*256), (int)(1.00*256)},
{350,	(int)(1.00*256), (int)(1.00*256)},
{350,	(int)(1.04*256), (int)(1.00*256)},
{350,	(int)(1.07*256), (int)(1.00*256)},
{350,	(int)(1.11*256), (int)(1.00*256)},
{350,	(int)(1.15*256), (int)(1.00*256)},
{350,	(int)(1.19*256), (int)(1.00*256)},
{350,	(int)(1.23*256), (int)(1.00*256)},
{350,	(int)(1.27*256), (int)(1.00*256)},
{350,	(int)(1.32*256), (int)(1.00*256)},
{350,	(int)(1.37*256), (int)(1.00*256)},
{350,	(int)(1.41*256), (int)(1.00*256)},
{350,	(int)(1.46*256), (int)(1.00*256)},
{350,	(int)(1.52*256), (int)(1.00*256)},
{350,	(int)(1.57*256), (int)(1.00*256)},
{350,	(int)(1.62*256), (int)(1.00*256)},
{350,	(int)(1.68*256), (int)(1.00*256)},
{350,	(int)(1.74*256), (int)(1.00*256)},
{350,	(int)(1.80*256), (int)(1.00*256)},
{350,	(int)(1.87*256), (int)(1.00*256)},
{350,	(int)(1.93*256), (int)(1.00*256)},
{701,	(int)(1.00*256), (int)(1.00*256)},
{701,	(int)(1.04*256), (int)(1.00*256)},
{701,	(int)(1.07*256), (int)(1.00*256)},
{701,	(int)(1.11*256), (int)(1.00*256)},
{701,	(int)(1.15*256), (int)(1.00*256)},
{701,	(int)(1.19*256), (int)(1.00*256)},
{701,	(int)(1.23*256), (int)(1.00*256)},
{701,	(int)(1.27*256), (int)(1.00*256)},
{701,	(int)(1.32*256), (int)(1.00*256)},
{701,	(int)(1.37*256), (int)(1.00*256)},
{701,	(int)(1.41*256), (int)(1.00*256)},
{701,	(int)(1.46*256), (int)(1.00*256)},
{1051,	(int)(1.00*256), (int)(1.00*256)},
{1051,	(int)(1.04*256), (int)(1.00*256)},
{1051,	(int)(1.07*256), (int)(1.00*256)},
{1051,	(int)(1.11*256), (int)(1.00*256)},
{1051,	(int)(1.15*256), (int)(1.00*256)},
{1051,	(int)(1.19*256), (int)(1.00*256)},
{1051,	(int)(1.23*256), (int)(1.00*256)},
{1051,	(int)(1.27*256), (int)(1.00*256)},
{1051,	(int)(1.32*256), (int)(1.00*256)},
{1051,	(int)(1.37*256), (int)(1.00*256)},
{1051,	(int)(1.41*256), (int)(1.00*256)},
{1051,	(int)(1.46*256), (int)(1.00*256)},
{1051,	(int)(1.52*256), (int)(1.00*256)},
{1051,	(int)(1.57*256), (int)(1.00*256)},
{1051,	(int)(1.62*256), (int)(1.00*256)},
{1051,	(int)(1.68*256), (int)(1.00*256)},
{1051,	(int)(1.74*256), (int)(1.00*256)},
{1051,	(int)(1.80*256), (int)(1.00*256)},
{1051,	(int)(1.87*256), (int)(1.00*256)},
{1051,	(int)(1.93*256), (int)(1.00*256)},
{1051,	(int)(2.00*256), (int)(1.00*256)},
{1051,	(int)(2.07*256), (int)(1.00*256)},
{1051,	(int)(2.14*256), (int)(1.00*256)},
{1051,	(int)(2.22*256), (int)(1.00*256)},
{1051,	(int)(2.30*256), (int)(1.00*256)},
{1051,	(int)(2.38*256), (int)(1.00*256)},
{1051,	(int)(2.46*256), (int)(1.00*256)},
{1051,	(int)(2.55*256), (int)(1.00*256)},
{1051,	(int)(2.64*256), (int)(1.00*256)},
{1051,	(int)(2.73*256), (int)(1.00*256)},
{1051,	(int)(2.83*256), (int)(1.00*256)},
{1051,	(int)(2.93*256), (int)(1.00*256)},
{1051,	(int)(3.03*256), (int)(1.00*256)},
{1051,	(int)(3.14*256), (int)(1.00*256)},
{1051,	(int)(3.25*256), (int)(1.00*256)},
{1051,	(int)(3.36*256), (int)(1.00*256)},
{1051,	(int)(3.48*256), (int)(1.00*256)},
{1051,	(int)(3.61*256), (int)(1.00*256)},
{1051,	(int)(3.73*256), (int)(1.00*256)},
{1051,	(int)(3.86*256), (int)(1.00*256)},
{1051,	(int)(4.00*256), (int)(1.00*256)},
{1051,	(int)(4.14*256), (int)(1.00*256)},
{1051,	(int)(4.29*256), (int)(1.00*256)},
{1051,	(int)(4.44*256), (int)(1.00*256)},
{1051,	(int)(4.59*256), (int)(1.00*256)},
{1051,	(int)(4.76*256), (int)(1.00*256)},
{1051,	(int)(4.92*256), (int)(1.00*256)},
{1051,	(int)(5.10*256), (int)(1.00*256)},
{1051,	(int)(5.28*256), (int)(1.00*256)},
{1051,	(int)(5.46*256), (int)(1.00*256)},
{1051,	(int)(5.66*256), (int)(1.00*256)},
{1051,	(int)(5.86*256), (int)(1.00*256)},
{1051,	(int)(6.06*256), (int)(1.00*256)}

};

static const  int g_ov8865_exp_time_gain_60Hz[ov8865_60HZ_EXP_TIME_TOTAL][3] = 
{ // {time, analog gain, digital gain}
{10	,(int)(1.00*256), (int)(1.00*256)},
{10	,(int)(1.00*256), (int)(1.00*256)},
{11	,(int)(1.00*256), (int)(1.00*256)},
{11	,(int)(1.00*256), (int)(1.00*256)},
{12	,(int)(1.00*256), (int)(1.00*256)},
{12	,(int)(1.00*256), (int)(1.00*256)},
{12	,(int)(1.00*256), (int)(1.00*256)},
{13	,(int)(1.00*256), (int)(1.00*256)},
{13	,(int)(1.00*256), (int)(1.00*256)},
{14	,(int)(1.00*256), (int)(1.00*256)},
{14	,(int)(1.00*256), (int)(1.00*256)},
{15	,(int)(1.00*256), (int)(1.00*256)},
{15	,(int)(1.00*256), (int)(1.00*256)},
{16	,(int)(1.00*256), (int)(1.00*256)},
{16	,(int)(1.00*256), (int)(1.00*256)},
{17	,(int)(1.00*256), (int)(1.00*256)},
{18	,(int)(1.00*256), (int)(1.00*256)},
{18	,(int)(1.00*256), (int)(1.00*256)},
{19	,(int)(1.00*256), (int)(1.00*256)},
{20	,(int)(1.00*256), (int)(1.00*256)},
{20	,(int)(1.00*256), (int)(1.00*256)},
{21	,(int)(1.00*256), (int)(1.00*256)},
{22	,(int)(1.00*256), (int)(1.00*256)},
{22	,(int)(1.00*256), (int)(1.00*256)},
{23	,(int)(1.00*256), (int)(1.00*256)},
{24	,(int)(1.00*256), (int)(1.00*256)},
{25	,(int)(1.00*256), (int)(1.00*256)},
{26	,(int)(1.00*256), (int)(1.00*256)},
{27	,(int)(1.00*256), (int)(1.00*256)},
{28	,(int)(1.00*256), (int)(1.00*256)},
{29	,(int)(1.00*256), (int)(1.00*256)},
{30	,(int)(1.00*256), (int)(1.00*256)},
{31	,(int)(1.00*256), (int)(1.00*256)},
{32	,(int)(1.00*256), (int)(1.00*256)},
{33	,(int)(1.00*256), (int)(1.00*256)},
{34	,(int)(1.00*256), (int)(1.00*256)},
{35	,(int)(1.00*256), (int)(1.00*256)},
{36	,(int)(1.00*256), (int)(1.00*256)},
{38	,(int)(1.00*256), (int)(1.00*256)},
{39	,(int)(1.00*256), (int)(1.00*256)},
{40	,(int)(1.00*256), (int)(1.00*256)},
{42	,(int)(1.00*256), (int)(1.00*256)},
{43	,(int)(1.00*256), (int)(1.00*256)},
{45	,(int)(1.00*256), (int)(1.00*256)},
{46	,(int)(1.00*256), (int)(1.00*256)},
{48	,(int)(1.00*256), (int)(1.00*256)},
{50	,(int)(1.00*256), (int)(1.00*256)},
{51	,(int)(1.00*256), (int)(1.00*256)},
{53	,(int)(1.00*256), (int)(1.00*256)},
{55	,(int)(1.00*256), (int)(1.00*256)},
{57	,(int)(1.00*256), (int)(1.00*256)},
{59	,(int)(1.00*256), (int)(1.00*256)},
{61	,(int)(1.00*256), (int)(1.00*256)},
{63	,(int)(1.00*256), (int)(1.00*256)},
{66	,(int)(1.00*256), (int)(1.00*256)},
{68	,(int)(1.00*256), (int)(1.00*256)},
{70	,(int)(1.00*256), (int)(1.00*256)},
{73	,(int)(1.00*256), (int)(1.00*256)},
{75	,(int)(1.00*256), (int)(1.00*256)},
{78	,(int)(1.00*256), (int)(1.00*256)},
{81	,(int)(1.00*256), (int)(1.00*256)},
{84	,(int)(1.00*256), (int)(1.00*256)},
{87	,(int)(1.00*256), (int)(1.00*256)},
{90	,(int)(1.00*256), (int)(1.00*256)},
{93	,(int)(1.00*256), (int)(1.00*256)},
{96	,(int)(1.00*256), (int)(1.00*256)},
{99	,(int)(1.00*256), (int)(1.00*256)},
{103	,(int)(1.00*256), (int)(1.00*256)},
{107	,(int)(1.00*256), (int)(1.00*256)},
{110	,(int)(1.00*256), (int)(1.00*256)},
{114	,(int)(1.00*256), (int)(1.00*256)},
{118	,(int)(1.00*256), (int)(1.00*256)},
{122	,(int)(1.00*256), (int)(1.00*256)},
{127	,(int)(1.00*256), (int)(1.00*256)},
{131	,(int)(1.00*256), (int)(1.00*256)},
{136	,(int)(1.00*256), (int)(1.00*256)},
{141	,(int)(1.00*256), (int)(1.00*256)},
{146	,(int)(1.00*256), (int)(1.00*256)},
{151	,(int)(1.00*256), (int)(1.00*256)},
{156	,(int)(1.00*256), (int)(1.00*256)},
{162	,(int)(1.00*256), (int)(1.00*256)},
{167	,(int)(1.00*256), (int)(1.00*256)},
{173	,(int)(1.00*256), (int)(1.00*256)},
{179	,(int)(1.00*256), (int)(1.00*256)},
{186	,(int)(1.00*256), (int)(1.00*256)},
{192	,(int)(1.00*256), (int)(1.00*256)},
{199	,(int)(1.00*256), (int)(1.00*256)},
{206	,(int)(1.00*256), (int)(1.00*256)},
{213	,(int)(1.00*256), (int)(1.00*256)},
{221	,(int)(1.00*256), (int)(1.00*256)},
{228	,(int)(1.00*256), (int)(1.00*256)},
{236	,(int)(1.00*256), (int)(1.00*256)},
{245	,(int)(1.00*256), (int)(1.00*256)},
{253	,(int)(1.00*256), (int)(1.00*256)},
{262	,(int)(1.00*256), (int)(1.00*256)},
{272	,(int)(1.00*256), (int)(1.00*256)},
{281	,(int)(1.00*256), (int)(1.00*256)},
{291	,(int)(1.00*256), (int)(1.00*256)},
{301	,(int)(1.00*256), (int)(1.00*256)},
{312	,(int)(1.00*256), (int)(1.00*256)},
{312	,(int)(1.04*256), (int)(1.00*256)},
{312	,(int)(1.07*256), (int)(1.00*256)},
{312	,(int)(1.11*256), (int)(1.00*256)},
{312	,(int)(1.15*256), (int)(1.00*256)},
{312	,(int)(1.19*256), (int)(1.00*256)},
{312	,(int)(1.23*256), (int)(1.00*256)},
{312	,(int)(1.27*256), (int)(1.00*256)},
{312	,(int)(1.32*256), (int)(1.00*256)},
{312	,(int)(1.37*256), (int)(1.00*256)},
{312	,(int)(1.41*256), (int)(1.00*256)},
{312	,(int)(1.46*256), (int)(1.00*256)},
{312	,(int)(1.52*256), (int)(1.00*256)},
{312	,(int)(1.57*256), (int)(1.00*256)},
{312	,(int)(1.62*256), (int)(1.00*256)},
{312	,(int)(1.68*256), (int)(1.00*256)},
{312	,(int)(1.74*256), (int)(1.00*256)},
{312	,(int)(1.80*256), (int)(1.00*256)},
{312	,(int)(1.87*256), (int)(1.00*256)},
{312	,(int)(1.93*256), (int)(1.00*256)},
{624	,(int)(1.00*256), (int)(1.00*256)},
{624	,(int)(1.04*256), (int)(1.00*256)},
{624	,(int)(1.07*256), (int)(1.00*256)},
{624	,(int)(1.11*256), (int)(1.00*256)},
{624	,(int)(1.15*256), (int)(1.00*256)},
{624	,(int)(1.19*256), (int)(1.00*256)},
{624	,(int)(1.23*256), (int)(1.00*256)},
{624	,(int)(1.27*256), (int)(1.00*256)},
{624	,(int)(1.32*256), (int)(1.00*256)},
{624	,(int)(1.37*256), (int)(1.00*256)},
{624	,(int)(1.41*256), (int)(1.00*256)},
{624	,(int)(1.46*256), (int)(1.00*256)},
{624	,(int)(1.52*256), (int)(1.00*256)},
{624	,(int)(1.57*256), (int)(1.00*256)},
{624	,(int)(1.62*256), (int)(1.00*256)},
{624	,(int)(1.68*256), (int)(1.00*256)},
{624	,(int)(1.74*256), (int)(1.00*256)},
{624	,(int)(1.80*256), (int)(1.00*256)},
{624	,(int)(1.87*256), (int)(1.00*256)},
{624	,(int)(1.93*256), (int)(1.00*256)},
{936	,(int)(1.32*256), (int)(1.00*256)},
{936	,(int)(1.37*256), (int)(1.00*256)},
{936	,(int)(1.41*256), (int)(1.00*256)},
{936	,(int)(1.46*256), (int)(1.00*256)},
{936	,(int)(1.52*256), (int)(1.00*256)},
{936	,(int)(1.57*256), (int)(1.00*256)},
{936	,(int)(1.62*256), (int)(1.00*256)},
{936	,(int)(1.68*256), (int)(1.00*256)},
{936	,(int)(1.74*256), (int)(1.00*256)},
{936	,(int)(1.80*256), (int)(1.00*256)},
{936	,(int)(1.87*256), (int)(1.00*256)},
{936	,(int)(1.93*256), (int)(1.00*256)},
{1248	,(int)(2.00*256), (int)(1.00*256)},
{1248	,(int)(2.07*256), (int)(1.00*256)},
{1248	,(int)(2.14*256), (int)(1.00*256)},
{1248	,(int)(2.22*256), (int)(1.00*256)},
{1248	,(int)(2.30*256), (int)(1.00*256)},
{1248	,(int)(2.38*256), (int)(1.00*256)},
{1248	,(int)(2.46*256), (int)(1.00*256)},
{1248	,(int)(2.55*256), (int)(1.00*256)},
{1248	,(int)(2.64*256), (int)(1.00*256)},
{1248	,(int)(2.73*256), (int)(1.00*256)},
{1248	,(int)(2.83*256), (int)(1.00*256)},
{1248	,(int)(2.93*256), (int)(1.00*256)},
{1248	,(int)(3.03*256), (int)(1.00*256)},
{1248	,(int)(3.14*256), (int)(1.00*256)},
{1248	,(int)(3.25*256), (int)(1.00*256)},
{1248	,(int)(3.36*256), (int)(1.00*256)},
{1248	,(int)(3.48*256), (int)(1.00*256)},
{1248	,(int)(3.61*256), (int)(1.00*256)},
{1248	,(int)(3.73*256), (int)(1.00*256)},
{1248	,(int)(3.86*256), (int)(1.00*256)},
{1248	,(int)(4.00*256), (int)(1.00*256)},
{1248	,(int)(4.14*256), (int)(1.00*256)},
{1248	,(int)(4.29*256), (int)(1.00*256)},
{1248	,(int)(4.44*256), (int)(1.00*256)},
{1248	,(int)(4.59*256), (int)(1.00*256)},
{1248	,(int)(4.76*256), (int)(1.00*256)},
{1248	,(int)(4.92*256), (int)(1.00*256)},
{1248	,(int)(5.10*256), (int)(1.00*256)},
{1248	,(int)(5.28*256), (int)(1.00*256)}

};

static const sensor_calibration_t ov8865_cdsp_calibration =
{

	g_ov8865_badpix_ob_table,			//ob
	g_ov8865_LiTable_rgb,				//lincorr
	g_ov8865_StepFac,					//lenscmp1	stepfactor
	g_ov8865_MaxTan8,					//lenscmp2	MaxTan8[32]
	g_ov8865_Slope4, 					//lenscmp3	Slop4[16]
	g_ov8865_CLPoint, 					//lenscmp4	CLPoint[8]
	g_ov8865_Radius_File_0, 			//lenscmp5	Radius_File_0[512]
	g_ov8865_Radius_File_1, 			//lenscmp6	Radius_File_1[512]
	g_ov8865_r_b_gain,					//wb_gain
	g_ov8865_gamma_045_table,			//gamma1
	g_ov8865_color_matrix4gamma045,	//color_matrix1
	g_ov8865_gamma_045_table,			//gamma2
	g_ov8865_color_matrix4gamma045,	//color_matrix2
	g_ov8865_awb_thr					//awb_thr	

};

//static const  int g_ov8865_exp_time_gain_50Hz[ov8865_50HZ_EXP_TIME_TOTAL_48MHz][3] =  //203,recount by u2

/**************************************************************************
 *                         SENSEOR FUNCTION                          *
 **************************************************************************/

static INT32S ov8865_sccb_open(void)
{
#if SCCB_MODE == SCCB_GPIO
	ov8865_handle = drv_l2_sccb_open(OV8865_SLAVE_ID, 16, 8);
	if(ov8865_handle == 0) {
		DBG_PRINT("Sccb open fail.\r\n");
		return STATUS_FAIL;
	}
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_init();
	ov8865_handle.slaveAddr = OV8865_SLAVE_ID;
	ov8865_handle.clkRate = 100;
#endif
	return STATUS_OK;
}

static void ov8865_sccb_close(void)
{
#if SCCB_MODE == SCCB_GPIO
	if(ov8865_handle) {
		drv_l2_sccb_close(ov8865_handle);
		ov8865_handle = NULL;
	}	
#elif SCCB_MODE == SCCB_HW_I2C	
	drv_l1_i2c_uninit();
	ov8865_handle.slaveAddr = 0;
	ov8865_handle.clkRate = 0;
#endif
}

static INT32S ov8865_sccb_write(INT16U reg, INT8U value)
{
	return drv_l2_sccb_write(ov8865_handle, reg, value);
}

static INT32S ov8865_sccb_read(INT16U reg, INT16U *value)
{
	return drv_l2_sccb_read(ov8865_handle, reg, value);
}

static INT32S ov8865_sccb_write_table(regval16_t *pTable)
{
	int j=0;
	INT16U data_buf2[1];
	for(j=0;j<sizeof(OV8865_1280_960_F30)/4;j++)
	{	
		ov8865_sccb_write(pTable->reg_num, pTable->value);
		pTable++;
	}
	return STATUS_OK;
}

INT32S ov8865_sccb_read_table(regval16_t *pTable)
{
	int j=0;
	INT16U data_buf[1];
	DBG_PRINT("read back\r\n");
	
	for(j=0;j<sizeof(OV8865_1280_960_F30)/4;j++)
	{	
		DBG_PRINT("%d 0x%x 0x%x ",j,pTable->reg_num,pTable->value);
		ov8865_sccb_read(pTable->reg_num, data_buf);
		DBG_PRINT("=0x%x ",data_buf[0]);
		pTable++;
		DBG_PRINT("\r\n");
	}
	return STATUS_OK;
}

sensor_exposure_t *ov8865_get_senInfo(void)
{
	return &ov8865_seInfo;
}


int ov8865_get_night_ev_idx(void)
{
	return ov8865_seInfo.night_ev_idx;
}

int ov8865_get_max_ev_idx(void)
{
	return ov8865_seInfo.max_ev_idx;
}

gpCdspWBGain_t *ov8865_awb_r_b_gain_boundary(void)
{
	int i;
	int max_r_gain, max_b_gain, min_r_gain, min_b_gain;
	
	max_r_gain = max_b_gain = 0;
	min_r_gain = min_b_gain = 255;
	
	for(i = 10 ; i < 55 ; i++)
	{
		if(max_r_gain < g_ov8865_r_b_gain[i][0]) max_r_gain = g_ov8865_r_b_gain[i][0];
		if(max_b_gain < g_ov8865_r_b_gain[i][1]) max_b_gain = g_ov8865_r_b_gain[i][1];
		if(min_r_gain > g_ov8865_r_b_gain[i][0]) min_r_gain = g_ov8865_r_b_gain[i][0];
		if(min_b_gain > g_ov8865_r_b_gain[i][1]) min_b_gain = g_ov8865_r_b_gain[i][1];
	}
	
	ov8865_wbgain.max_rgain = max_r_gain;
	ov8865_wbgain.max_bgain = max_b_gain;
	ov8865_wbgain.min_rgain = min_r_gain;
	ov8865_wbgain.min_bgain = min_b_gain;
	
	return &ov8865_wbgain;
}
/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/



static void ov8865_cvt_analog_gain(INT32U analog_gain)
{
	INT32U coarse_gain, fine_gain, temp_gain;


	coarse_gain = (analog_gain/256);
	fine_gain = ((analog_gain%256)/16);
	
	temp_gain = ((coarse_gain << 4) | fine_gain);


	//ov8865_sccb_write( 0x350A, Decimal>>6);//Digi Gain[13:6]
	//ov8865_sccb_write( 0x350B, (Decimal<<2)&0xfc);//Digi Gain[5:0]
	ov8865_sccb_write( 0x3508, temp_gain);//analog Gain[12:8]
	//ov8865_sccb_write( 0x3509, 0x06);//analog Gain[7:0]


}

int ov8865_set_exposure_time(void)
{	
	INT8U lsb_time, msb_time;
	int idx;

	// From agoritham calc new data update to ov8865_seInfo.
	ov8865_seInfo.sensor_ev_idx += ov8865_seInfo.ae_ev_idx;
	if(ov8865_seInfo.sensor_ev_idx >= ov8865_seInfo.max_ev_idx) ov8865_seInfo.sensor_ev_idx = ov8865_seInfo.max_ev_idx;
	if(ov8865_seInfo.sensor_ev_idx < 0) ov8865_seInfo.sensor_ev_idx = 0;
	
	idx = ov8865_seInfo.sensor_ev_idx * 3;
	ov8865_seInfo.time = p_expTime_table[idx];
	ov8865_seInfo.analog_gain = p_expTime_table[idx+1];
	ov8865_seInfo.digital_gain = p_expTime_table[idx+2];
	//DBG_PRINT("ev idx %d.\r\n", ov8865_seInfo.sensor_ev_idx );
	//DBG_PRINT("T %d, ag %d, ev %d.\r\n", ov8865_seInfo.time, ov8865_seInfo.analog_gain, ov8865_seInfo.sensor_ev_idx );
	//DBG_PRINT("Time = %d, a gain = %d, d gain = %d, ev idx = %d [%d]\r\n", ov8865_seInfo.time, ov8865_seInfo.analog_gain, ov8865_seInfo.digital_gain, ov8865_seInfo.sensor_ev_idx, ov8865_seInfo.ae_ev_idx );
	//digital_gain = ((si->digital_gain >> 2) & 0xFF);	//0x40:1x, 0xff:4x
	// set exposure time
	lsb_time = (INT8U)(ov8865_seInfo.time & 0xFF);
	msb_time = (INT8U)((ov8865_seInfo.time >>8 ) & 0x1F);
	
	#if GRP_WR	/*Group write start*/
	ov8865_sccb_write(0x3208, 0x00);	
	#endif
	ov8865_sccb_write(0x3501, lsb_time);
	ov8865_sccb_write(0x3500, msb_time);

	ov8865_cvt_analog_gain(ov8865_seInfo.analog_gain);
	
	#if GRP_WR	/*Group write end*/
	ov8865_sccb_write(0x3208, 0x10);	//grp 0 end
	ov8865_sccb_write(0x3208, 0xA0);	//launch group 0
	#endif
	ov8865_seInfo.ae_ev_idx = 0;
	return 0;
}



void ov8865_get_exposure_time(sensor_exposure_t *se)
{
	//int ret=0;
	gp_memcpy((INT8S *)se, (INT8S *)&ov8865_seInfo, sizeof(sensor_exposure_t));

}

void ov8865_set_exp_freq(int freq)
{
	if(freq == 50)
	{
		ov8865_seInfo.sensor_ev_idx = ov8865_50HZ_INIT_EV_IDX;
		ov8865_seInfo.ae_ev_idx = 0;
		ov8865_seInfo.daylight_ev_idx= ov8865_50HZ_DAY_EV_IDX;
		ov8865_seInfo.night_ev_idx= ov8865_50HZ_NIGHT_EV_IDX;
		ov8865_seInfo.max_ev_idx = ov8865_50HZ_MAX_EXP_IDX - 1;
		ov8865_seInfo.total_ev_idx = ov8865_50HZ_EXP_TIME_TOTAL;
		p_expTime_table = (int *)g_ov8865_exp_time_gain_50Hz;
	}
	else if(freq == 60)
	{
		ov8865_seInfo.sensor_ev_idx = ov8865_60HZ_INIT_EV_IDX;
		ov8865_seInfo.ae_ev_idx = 0;
		ov8865_seInfo.daylight_ev_idx = ov8865_60HZ_DAY_EV_IDX;
		ov8865_seInfo.night_ev_idx = ov8865_60HZ_NIGHT_EV_IDX;
		ov8865_seInfo.max_ev_idx = ov8865_60HZ_MAX_EXP_IDX - 1;
		ov8865_seInfo.total_ev_idx = ov8865_60HZ_EXP_TIME_TOTAL;
		p_expTime_table = (int *)g_ov8865_exp_time_gain_60Hz;
	}
}

static void ov8865_cdsp_eof_isr(void)
{

	if(ov8865_read_en == 1) {		
		ov8865_set_exposure_time();
		ov8865_read_en = 0;
		//DBG_PRINT("SET ov8865 exp done\r\n");
	}

}


static INT32S ov8865_set_ctrl(gpCdspCtrl_t *ctrl)
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
			ov8865_seInfo.userISO = si->userISO;
			if(p_expTime_table != 0 && ov8865_read_en == 0) {				
				ov8865_seInfo.ae_ev_idx = si->ae_ev_idx;
				ov8865_read_en = 1;
				//DBG_PRINT("V4L2_CID_EXPOSURE ov8865_seInfo.ae_ev_idx = %d\r\n", ov8865_seInfo.ae_ev_idx);
			} 
		}
		break;
		
	case V4L2_CID_POWER_LINE_FREQUENCY:
		ov8865_set_exp_freq(ctrl->value);
		break;
		
	default:
		return STATUS_FAIL;			
	}

	return STATUS_OK;
}

static INT32S ov8865_get_ctrl(gpCdspCtrl_t *ctrl)
{
	switch(ctrl->id)
	{
	case V4L2_CID_EXPOSURE:
		//ov8865_get_exposure_time(&ov8865_seInfo);
		ctrl->value = (int)&ov8865_seInfo;
		break;
		
	case V4L2_CID_BRIGHTNESS:
		//ctrl->value = 64 - (sensor_total_ev - ov9712_seInfo.max_ev_idx);
		break;
		
	default:
		return STATUS_FAIL;		
	}

	return STATUS_OK;
}

static const gpCdspSensor_t ov8865_ctrl =
{
	ov8865_set_ctrl,
	ov8865_get_ctrl
};

static void ov8865_set_favorite(void)
{
	myFav.ob_val = (INT16U)g_ov8865_FavTable[0];
	myFav.ae_target= (INT16U)g_ov8865_FavTable[1];
	myFav.ae_target_night= (INT16U)g_ov8865_FavTable[2];
	myFav.awb_mode = (INT16U)g_ov8865_FavTable[3];
	myFav.color_mode = (INT16U)g_ov8865_FavTable[4];
	myFav.iso = (INT16U)g_ov8865_FavTable[5];
	myFav.ev = (INT16U)g_ov8865_FavTable[6];
	myFav.day_hue_u_offset = (INT8S)g_ov8865_FavTable[7];
	myFav.day_hue_v_offset = (INT8S)g_ov8865_FavTable[8];
	myFav.day_hue_y_offset = (INT8S)g_ov8865_FavTable[9];
	myFav.day_sat_y_scale = (INT8S)g_ov8865_FavTable[10];
	myFav.day_sat_u_scale = (INT8S)g_ov8865_FavTable[11];
	myFav.day_sat_v_scale = (INT8S)g_ov8865_FavTable[12];
	myFav.day_edge= (INT8U)g_ov8865_FavTable[13];

	myFav.night_wb_offset = (INT8S)g_ov8865_FavTable[14];
	myFav.night_hue_u_offset = (INT8S)g_ov8865_FavTable[15];
	myFav.night_hue_v_offset = (INT8S)g_ov8865_FavTable[16];
	myFav.night_hue_y_offset = (INT8S)g_ov8865_FavTable[17];
	myFav.night_sat_y_scale = (INT8S)g_ov8865_FavTable[18];
	myFav.night_sat_u_scale = (INT8S)g_ov8865_FavTable[19];
	myFav.night_sat_v_scale = (INT8S)g_ov8865_FavTable[20];
	myFav.night_edge= (INT8U)g_ov8865_FavTable[21];
	myFav.night_wb_offset = (INT8S)g_ov8865_FavTable[22];
}


static int ov8865_init(void)
{
	ov8865_seInfo.max_time = ov8865_MAX_EXPOSURE_TIME;
	ov8865_seInfo.min_time = ov8865_MIN_EXPOSURE_TIME;

	ov8865_seInfo.max_digital_gain = ov8865_MAX_DIGITAL_GAIN ;
	ov8865_seInfo.min_digital_gain = ov8865_MIN_DIGITAL_GAIN ;

	ov8865_seInfo.max_analog_gain = ov8865_MAX_ANALOG_GAIN;
	ov8865_seInfo.min_analog_gain = ov8865_MIN_ANALOG_GAIN;

	ov8865_seInfo.analog_gain = ov8865_seInfo.min_analog_gain;
	ov8865_seInfo.digital_gain = ov8865_seInfo.min_digital_gain;
	ov8865_seInfo.time = ov8865_seInfo.max_time;
	ov8865_seInfo.userISO = ISO_AUTO;

	ov8865_set_exp_freq(50);
	
	ov8865_set_favorite();	
	
	DBG_PRINT("ov8865_init\r\n");
	return 0;
}


void ov8865_cdsp_init(void)
{
	
	ov8865_init();
	// Turn on LDO 2.8V for CSI sensor
	drv_l1_power_ldo_28_ctrl(1, LDO_LDO28_3P3V);

	// set Mclk(CSI_CLKO) to IOD12, for MIPI Mclk
	R_FUNPOS1 |= 0x42; 
	// set Mclk(CSI_CLKO) to IOD7, for MIPI Mclk	
	//R_FUNPOS1 |= 0x21;
	drv_l2_sensor_set_mclkout(ov8865_cdsp_mipi_ops.info[0].mclk);

	drv_l1_mipi_init(ENABLE);
	drv_l1_mipi_set_parameter(&ov8865_mipi_cfg);
		
	//ov8865_sensor_calibration_str();
	ov8865_sccb_open();
	
	// cdsp init
	drv_l2_cdsp_open();
	drv_l2_CdspTableRegister((sensor_calibration_t *)&ov8865_cdsp_calibration);
	drv_l2_CdspSensorCtrlRegister((gpCdspSensor_t *)&ov8865_ctrl);

}

/**
 * @brief   un-initialization function
 * @param   sensor format parameters
 * @return 	none
 */
static void ov8865_cdsp_uninit(void)
{
	// disable mclk
	drv_l2_sensor_set_mclkout(MCLK_NONE);

	// cdsp disable
	drv_l2_cdsp_close();

	drv_l1_mipi_init(DISABLE);

	// release sccb
	ov8865_sccb_close();
	
	/* Turn off LDO 2.8V for CSI sensor */
	drv_l1_power_ldo_28_ctrl(0, LDO_LDO28_2P8V);
}	

/**
 * @brief   stream start function
 * @param   info index
 *			
 * @return 	none
 */
static void ov8865_cdsp_stream_on(INT32U index, INT32U bufA, INT32U bufB)
{
	INT16U target_w, target_h;
	gpCdspFmt_t format;
	
	DBG_PRINT("ov8865 index = %d\r\n", index);
	switch(index)
	{
	case 0:
	case 1:
	case 2:
			ov8865_sccb_write_table((regval16_t *)OV8865_1280_960_F30);
			//ov8865_sccb_read_table((regval16_t *)OV8865_1280_960_F30);
		break;
		
	default:
		return;	
	}

	// mclk output

		drv_l2_sensor_set_mclkout(ov8865_cdsp_mipi_ops.info[index].mclk);
		format.image_source = C_CDSP_MIPI;
		format.input_format = ov8865_cdsp_mipi_ops.info[index].input_format;
		format.output_format = ov8865_cdsp_mipi_ops.info[index].output_format;
		target_w = ov8865_cdsp_mipi_ops.info[index].target_w;
		target_h = ov8865_cdsp_mipi_ops.info[index].target_h;
		format.hpixel = ov8865_cdsp_mipi_ops.info[index].sensor_w;
		format.vline = ov8865_cdsp_mipi_ops.info[index].sensor_h;
		format.hoffset = ov8865_cdsp_mipi_ops.info[index].hoffset;
		format.voffset = ov8865_cdsp_mipi_ops.info[index].voffset;
		format.sensor_timing_mode = ov8865_cdsp_mipi_ops.info[index].interface_mode;
		format.sensor_hsync_mode = ov8865_cdsp_mipi_ops.info[index].hsync_active;
		format.sensor_vsync_mode = ov8865_cdsp_mipi_ops.info[index].vsync_active;

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
	drv_l2_cdsp_enable(target_w, target_h, myFav);	

	if( drv_l2_get_raw_yuv_path() == 0)
	{
		#if CAPTURE_RAW_MODE
		save_raw10(g_cdsp_rawcal_mode, g_frame_addr1);
		#else
		save_raw10(g_cdsp_rawcal_mode, DUMMY_BUFFER_ADDRS2);
		#endif	
	}
	drv_l2_CdspIsrRegister(C_ISR_SENSOR, ov8865_cdsp_eof_isr);	

	R_CDSP_MIPI_CTRL &= ~0x7;
	
#endif
	
}

/**
 * @brief   stream stop function
 * @param   none
 * @return 	none
 */ 
static void ov8865_cdsp_stream_off(void)
{
	drv_l2_cdsp_stream_off();
}	

/**
 * @brief   get info function
 * @param   none
 * @return 	pointer to sensor information data
 */ 
static drv_l2_sensor_info_t* ov8865_cdsp_get_info(INT32U index)
{
	if(index > (MAX_INFO_NUM - 1)) {
		return NULL;
	} else {
		return (drv_l2_sensor_info_t*)&ov8865_cdsp_mipi_ops.info[index];

	}
}
/*
INT32S ov8865_switch_pclk(INT8U flag)
{
	if(flag) {
		GC_CLK = GC_42MHZ;
	} else {
		GC_CLK = GC_48MHZ;
	}
	return 0;
}
*/
const drv_l2_sensor_ops_t ov8865_cdsp_mipi_ops =
{
	SENSOR_OV8865_CDSP_MIPI_NAME,		/* sensor name */
	ov8865_cdsp_init,
	ov8865_cdsp_uninit,
	ov8865_cdsp_stream_on,
	ov8865_cdsp_stream_off,
	ov8865_cdsp_get_info,
	{
		/* 1st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SBGGR10,		/* input format */
			V4L2_PIX_FMT_YVYU,			/* output format */
			30,							/* FPS in sensor */
			640,						/* target width */
			480, 						/* target height */	
			640,						/* sensor width */
			480, 						/* sensor height */	
			0,							/* sensor h offset */ 
			0,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 2st info */
		{
			MCLK_24M,					/* MCLK */
			#if SENSOR_FLIP == 1
			V4L2_PIX_FMT_SGBRG10,
			#else
			V4L2_PIX_FMT_SRGGB10,		/* input format */
			#endif
			V4L2_PIX_FMT_YVYU,			/* output format */
			30,							/* FPS in sensor */
			1280,						/* target width */
			960, 						/* target height */	
			0x650,						/* sensor width */
			0x4B8, 						/* sensor height */
			5,							/* sensor h offset */ 
			1,                          /* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
		/* 3st info */
		{
			MCLK_24M,					/* MCLK */
			V4L2_PIX_FMT_SRGGB10,		/* input format */
			V4L2_PIX_FMT_YVYU,			/* output format */
			30,							/* FPS in sensor */
			1280,						/* target width */
			720, 						/* target height */	
			0x5E0,						/* sensor width */
			0x460,		 				/* sensor height */
			1,							/* sensor h offset */ 
			1,							/* sensor v offset */
			MODE_CCIR_601,				/* input interface */
			MODE_NONE_INTERLACE,		/* interlace mode */			
			MODE_ACTIVE_HIGH,			/* hsync pin active level */
			MODE_ACTIVE_HIGH,			/* vsync pin active level */
		},
	}
};


//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //
//================================================================//
