/**************************************************************************
 *                         G L O B A L    D A T A  For CDSP using                          *
 **************************************************************************/

#if 0
	(USE_SENSOR_NAME == SENSOR_ov8865)
	#define	TARGET_Y  				0x42	//Luminace
	#define TARGET_Y_LOW			(TARGET_Y/4)	//TARGET_Y_LOW = TARGET_Y/4  
	#define MAX_ANALOG_GAIN			4		//3 ~ 5
		
/**************************************************************************
*	ov8865 Daylight Mode																														*
**************************************************************************/	
	//=========================================================================
	// Purpose :  set the color, Hue is one of the main properties of a color
	// Parameters : -128 ~ +127
	// DAYLIGHT_HUE_U_OFFSET:	+: more blue,  	-: more yellow/green	
	// DAYLIGHT_HUE_V_OFFSET:	+: more red,  	-: more blue/green
	//=========================================================================
	#define	DAYLIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	#define	DAYLIGHT_HUE_V_OFFSET  	1 		//-128 ~ +127,   +: more red,  -: more blue/green
		
	//=========================================================================
	// Purpose :  set the contrast, Y_offset relationship with Y_scale
	// Parameters : Default 0x20
	// Ex: 	Y_SCALE = 0x21, Y_OFFSET range = 0~-7, 
	//		Y_SCALE = 0x22, Y_OFFSET range = 0~-15,  
	//=========================================================================
	#define	DAYLIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	#define	DAYLIGHT_SAT_Y_SCALE  	0x22	//Default:0x20
	
	//=========================================================================
	// Purpose :  set the sturatin
	// Parameters : Default 0x20
	// DAYLIGHT_SAT_U_SCALE: Blue, upper 0x20: deep blue, less 0x20: light blue
	// DAYLIGHT_SAT_V_SCALE: Red,  upper 0x20: deep red,  less 0x20: light red
	//========================================================================= 
	#define	DAYLIGHT_SAT_U_SCALE  	0x25	//Default:0x20 	// blud
	#define	DAYLIGHT_SAT_V_SCALE  	0x25	//Default:0x20	// red
	
	//=========================================================================
	// Purpose :  set the sharpness
	// Parameters : 0:soft, 1:smooth, 2:normal, 3:strong
	//========================================================================= 
	#define DAYLIGHT_EDGE			2
	
	//=========================================================================
	// Purpose :  set the white balance offset
	// Parameters : +: warm,   -: cold , +10~-10
	//=========================================================================
	#define DAYLIGHT_WB_OFFSET		0
	#define AUTO_WB_OFFSET			0
	 
/**************************************************************************
*****	ov8865 Night Mode			
***  Contarst: NIGHT_SAT_Y_SCALE +	NIGHT_HUE_Y_OFFSET, Strong: 0x22 + (-15 ~ 0)
***											*
**************************************************************************/	
	//=========================================================================
	// Purpose :  set the color, Hue is one of the main properties of a color
	// Parameters : -128 ~ +127
	// DAYLIGHT_HUE_U_OFFSET:	+: more blue,  	-: more yellow/green	
	// DAYLIGHT_HUE_V_OFFSET:	+: more red,  	-: more blue/green
	//=========================================================================                 	    	
	#define	NIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	#define	NIGHT_HUE_V_OFFSET  	-4		//-128 ~ +127,   +: more red,  -: more blue/green
		
	//=========================================================================
	// Purpose :  set the contrast, Y_offset relationship with Y_scale
	// Parameters : Default 0x20
	// Ex: 	Y_SCALE = 0x21, Y_OFFSET range = 0~-7, 
	//		Y_SCALE = 0x22, Y_OFFSET range = 0~-15,  
	//=========================================================================
	#define	NIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	#define	NIGHT_SAT_Y_SCALE  		0x22	//Default:0x20
	
	//=========================================================================
	// Purpose :  set the sturatin
	// Parameters : Default 0x20
	// DAYLIGHT_SAT_U_SCALE: Blue, upper 0x20: deep blue, less 0x20: light blue
	// DAYLIGHT_SAT_V_SCALE: Red,  upper 0x20: deep red,  less 0x20: light red
	//=========================================================================            		
	#define	NIGHT_SAT_U_SCALE  		0x1B	//Default:0x20	// blud
	#define	NIGHT_SAT_V_SCALE  		0x1B	//Default:0x20	// red
	
	//=========================================================================
	// Purpose :  set the sharpness
	// Parameters : 0:soft, 1:smooth, 2:normal, 3:strong
	//========================================================================= 
	#define NIGHT_EDGE				2	
	
	//=========================================================================
	// Purpose :  set the white balance offset
	// Parameters : +: warm,   -: cold , +10~-10
	//=========================================================================
	#define NIGHT_WB_OFFSET			0
	
 
	myFav.ob_val = (INT16U)g_ov8865_FavTable[0];
	myFav.targetY = (INT16U)g_ov8865_FavTable[1];
	myFav.targetYlow = (INT16U)g_ov8865_FavTable[2];
	myFav.awb_mode = (INT16U)g_ov8865_FavTable[3];
	myFav.color_mode = (INT16U)g_ov8865_FavTable[4];
	myFav.iso = (INT16U)g_ov8865_FavTable[5];
	myFav.ev = (INT16U)g_ov8865_FavTable[6];
	myFav.day_hue_u_offset = (INT8S)g_ov8865_FavTable[7];
	myFav.day_hue_v_offset = (INT8S)g_ov8865_FavTable[8];
	myFav.day_hue_y_offset = (INT8S)g_ov8865_FavTable[9];
	myFav.day_sat_y_offset = (INT8S)g_ov8865_FavTable[10];
	myFav.day_sat_u_offset = (INT8S)g_ov8865_FavTable[11];
	myFav.day_sat_v_offset = (INT8S)g_ov8865_FavTable[12];
	myFav.day_edge= (INT8U)g_ov8865_FavTable[13];

	myFav.night_wb_offset = (INT8S)g_ov8865_FavTable[14];
	myFav.night_hue_u_offset = (INT8S)g_ov8865_FavTable[15];
	myFav.night_hue_v_offset = (INT8S)g_ov8865_FavTable[16];
	myFav.night_hue_y_offset = (INT8S)g_ov8865_FavTable[17];
	myFav.night_sat_y_offset = (INT8S)g_ov8865_FavTable[18];
	myFav.night_sat_u_offset = (INT8S)g_ov8865_FavTable[19];
	myFav.night_sat_v_offset = (INT8S)g_ov8865_FavTable[20];
	myFav.night_edge= (INT8U)g_ov8865_FavTable[21];
	myFav.night_wb_offset = (INT8S)g_ov8865_FavTable[22];

#endif

static const short g_ov8865_FavTable[] =
{
	0,		//ob_val
	50,//0x30,	//targetY
	20,		//targetY low = (TARGET_Y/4)
	AWB_AUTO_CVR,	//awb mode
	MODE_NORMAL,	//color mode
	ISO_AUTO,		//ISO
	6,				//EV // 0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0 
	0,	//	#define	DAYLIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	1,	//	#define	DAYLIGHT_HUE_V_OFFSET  	1 		//-128 ~ +127,   +: more red,  -: more blue/green
	-4,//-1,	//	#define	DAYLIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	0x22,//	#define	DAYLIGHT_SAT_Y_SCALE  	0x22	//Default:0x20
	0x26,//	#define	DAYLIGHT_SAT_U_SCALE  	0x25	//Default:0x20 	// blud
	0x26,//	#define	DAYLIGHT_SAT_V_SCALE  	0x25	//Default:0x20	// red
	2,	//#define DAYLIGHT_EDGE			2
	0,	//#define DAYLIGHT_WB_OFFSET		0
	0,	//	#define	NIGHT_HUE_U_OFFSET  	0		//-128 ~ +127,   +: more blue,  -: more yellow/green	
	-4,//-4,	//	#define	NIGHT_HUE_V_OFFSET  	-4		//-128 ~ +127,   +: more red,  -: more blue/green
	-8,	//	#define	NIGHT_HUE_Y_OFFSET  	-1		//-128 ~ +127	
	0x22,//	#define	NIGHT_SAT_Y_SCALE  		0x22	//Default:0x20
	0x1B,//	#define	NIGHT_SAT_U_SCALE  		0x1B	//Default:0x20	// blud
	0x1B,//	#define	NIGHT_SAT_V_SCALE  		0x1B	//Default:0x20	// red
	2,	//#define NIGHT_EDGE				2	
	0	//	#define NIGHT_WB_OFFSET			0
};


//1 same
//This table requires MODE_PURERAW input.
//Then use ob_util.exe tool to generate
static const unsigned short g_ov8865_badpix_ob_table[12] = 
{
	0, 		// obautoen
	0, 		// obmanuen
	8, 		// maunob
	
	0, 		// wboffseten
	4,		// wbo_r
	0, 		// wbo_gr
	0, 		// wbo_gb
	4, 		// wbo_b
	
	1,		//badPixel En
	160,	//bprthr
	160,	//bpgthr
	160		//bpbthr
};

//This table requires MODE_LENSCMP RAW input.
//Then use _awb_util.exe tool to generate
static const unsigned short g_ov8865_r_b_gain[71][2] = //2014-10-03.u2
{ // { r gain, b gain }
	{38	,107},
	{40	,105},
	{42	,104},
	{45	,102},
	{47	,101},
	{49	,100},
	{51	,98 },
	{53	,97 },
	{54	,96 },
	{56	,95 },
	{58	,93 },
	{59	,92 },
	{61	,91 },
	{62	,90 },
	{64	,89 },
	{65	,88 },
	{66	,87 },
	{68	,85 },
	{69	,84 },
	{70	,83 },
	{71	,82 },//40
	{72	,82 },
	{73	,81 },
	{74	,80 },
	{75	,79 },
	{76	,78 },
	{77	,77 },
	{78	,76 },
	{78	,75 },
	{79	,75 },
	{80	,74 },//50
	{81	,73 },
	{81	,72 },
	{82	,72 },
	{82	,71 },
	{83	,70 },
	{84	,69 },
	{84	,69 },
	{85	,68 },
	{85	,68 },
	{86	,67 },//60
	{86	,66 },
	{87	,66 },
	{87	,65 },
	{88	,65 },
	{88	,64 },
	{89	,64 },
	{89	,63 },
	{90	,63 },
	{90	,62 },
	{91	,62 },//70
	{91	,61 },
	{92	,61 },
	{93	,60 },
	{93	,60 },
	{94	,59 },
	{94	,59 },
	{95	,59 },
	{96	,58 },
	{96	,58 },
	{97	,58 },
	{98	,57 },
	{99	,57 },
	{99	,57 },
	{100,56 },
	{101,56 },
	{102,56 },
	{103,55 },
	{104,55 },
	{105,55 },
	{106,55 }
};

//This table requires MODE_WBGAIN RAW input.
//Then use _gc_util.exe tool to generate
#if 1
static const unsigned int g_ov8865_gamma_045_table[] = 	//2014.10.03.David
{
	0x11110f, 0x111113, 0x044417, 0x11101b, 0x04111e, 0x110422, 0x044125, 0x011029, 
	0x10442c, 0x04412f, 0x041132, 0x011036, 0x010439, 0x01043c, 0x01043f, 0x010442, 
	0x011045, 0x041048, 0x04414a, 0x10414d, 0x010450, 0x041053, 0x104155, 0x041058, 
	0x10415a, 0x04105d, 0x00415f, 0x041062, 0x010464, 0x004166, 0x101069, 0x04046b, 
	0x01046d, 0x01016f, 0x004171, 0x004074, 0x004076, 0x004078, 0x00407a, 0x00407c, 
	0x00407e, 0x01017f, 0x040181, 0x040483, 0x101085, 0x004087, 0x010188, 0x10048a, 
	0x00108c, 0x01008e, 0x10048f, 0x004091, 0x040192, 0x001094, 0x040096, 0x001097, 
	0x040099, 0x00109a, 0x04009c, 0x00109d, 0x10019e, 0x0040a0, 0x0004a1, 0x0400a3, 
	0x0040a4, 0x1004a5, 0x0100a7, 0x0040a8, 0x0004a9, 0x0400ab, 0x0040ac, 0x0004ad, 
	0x1001ae, 0x0100b0, 0x0040b1, 0x0004b2, 0x1000b4, 0x0100b5, 0x0040b6, 0x0004b7, 
	0x1000b9, 0x0100ba, 0x0040bb, 0x0004bc, 0x1001bd, 0x0400bf, 0x0040c0, 0x0010c1, 
	0x1001c2, 0x0400c4, 0x0040c5, 0x0004c6, 0x1001c7, 0x0100c9, 0x0040ca, 0x0004cb, 
	0x0400cd, 0x0040ce, 0x0004cf, 0x0400d1, 0x0040d2, 0x0004d3, 0x0100d5, 0x0010d6, 
	0x0401d7, 0x0040d9, 0x1004da, 0x0100dc, 0x1004dd, 0x0100df, 0x1004e0, 0x0040e2, 
	0x0401e3, 0x0010e5, 0x0100e7, 0x1004e8, 0x0040ea, 0x0101eb, 0x1004ed, 0x0010ef, 
	0x0040f1, 0x0101f2, 0x0404f4, 0x1004f6, 0x1010f8, 0x1010fa, 0x1010fc, 0x0010fe,
};

//This table requires MODE_LUTGAMMA RAW input.
//Then use _cc_lite.exe tool to generate
static const short g_ov8865_color_matrix4gamma045[9] =
{	
	(short)((1.51789520873730450000 *64) + 0.5),
	(short)((-0.70247233522367103000*64) + 0.5),
	(short)((0.18457712648636648000 *64) + 0.5),
	(short)((-0.36429855413750251000*64) + 0.5),
	(short)((1.77421805886091380000 *64) + 0.5),
	(short)((-0.40991950472341110000*64) + 0.5),
	(short)((-0.14057938794322894000*64) + 0.5),
	(short)((-1.01306716435587970000*64) + 0.5),
	(short)((2.15364655229910840000 *64) + 0.5)  
};
#endif

#if 0//H22
static const unsigned int g_ov8865_gamma_045_table[] =
{
0x04510d, 0x051112, 0x111417, 0x14451b, 0x111120, 0x044425, 0x111129, 0x05112d, 
0x044432, 0x044436, 0x04443a, 0x04443e, 0x044442, 0x110446, 0x111149, 0x04444d, 
0x111051, 0x044154, 0x111058, 0x10445b, 0x04415e, 0x011062, 0x011065, 0x010468, 
0x01046b, 0x01046e, 0x041071, 0x041074, 0x104176, 0x010479, 0x04107c, 0x00417e, 
0x041081, 0x010183, 0x101086, 0x040488, 0x01018a, 0x10408d, 0x10108f, 0x041091, 
0x040493, 0x040495, 0x040497, 0x040499, 0x10109b, 0x00109d, 0x00409f, 0x0101a0, 
0x1004a2, 0x0010a4, 0x0100a6, 0x1004a7, 0x0040a9, 0x1001aa, 0x0040ac, 0x1001ad, 
0x0100af, 0x0004b0, 0x0400b2, 0x0040b3, 0x0004b4, 0x0400b6, 0x0100b7, 0x0010b8, 
0x0004b9, 0x1000bb, 0x0400bc, 0x0100bd, 0x0040be, 0x0010bf, 0x0004c0, 0x0004c1, 
0x0001c2, 0x0001c3, 0x0000c5, 0x0000c6, 0x0000c7, 0x0000c8, 0x0000c9, 0x0000ca, 
0x0000cb, 0x0000cc, 0x0000cd, 0x0000ce, 0x0000cf, 0x0000d0, 0x0001d0, 0x0001d1, 
0x0001d2, 0x0001d3, 0x0001d4, 0x0001d5, 0x0001d6, 0x0001d7, 0x0001d8, 0x0001d9, 
0x0000db, 0x0000dc, 0x1000dd, 0x0400de, 0x0400df, 0x0100e0, 0x0040e1, 0x0010e2, 
0x0004e3, 0x1000e5, 0x0100e6, 0x0040e7, 0x0004e8, 0x1000ea, 0x0100eb, 0x0004ec, 
0x0400ee, 0x0040ef, 0x1001f0, 0x0040f2, 0x1004f3, 0x0040f5, 0x0401f6, 0x0010f8, 
0x0100fa, 0x0404fb, 0x1010fd, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff
};


static const short g_ov8865_color_matrix4gamma045[9] = 
{	
	(short) (1.289461127029244000*64),
	(short) (-0.15633132671308050*64),
	(short) (-0.13312980031616348*64),
	(short) (-0.28924304019577968*64),
	(short) (1.448004578314210100*64),
	(short) (-0.15876153811843055*64),
	(short) (-0.02364080679490283*64),
	(short) (-1.03955844263753080*64),
	(short) (2.063199249432433800*64)
};
#endif

//This table requires MODE_LENSCMP RAW input.
//Then use _awb_util.exe tool to generate
static const short g_ov8865_awb_thr[19] =  //2014-10-03.u2
{
	197, // awbwinthr
	
	0*64, // sindata
	1*64, // cosdata 
	
	 31, // Ythr0
	104, // Ythr1
	150, // Ythr2 
	197, // Ythr3 
	
	-4, // UL1N1 
	 5, // UL1P1 
	-4, // VL1N1 
	 4, // VL1P1 
	
	 -6, //UL1N2
	  6, //UL1P2
	 -5, //VL1N2
	  4, // VL1P2
	
	 -6, // UL1N3
	  7, //UL1P3
	 -6, // VL1N3
	  5, //VL1P3
};

//This table requires MODE_PURERAW input.
//Then use lc_lite.exe tool to generate
INT8U g_ov8865_LiTable_rgb[48]=
{	//R*16,Gb/Gr*16,B*16
	0x1e,0x2e,0x3e,0x4e,0x5e,0x6e,0x7e,0x8e,
	0x9e,0xae,0xbe,0xce,0xde,0xee,0xfe,0xff,
	0x0f,0x1f,0x2f,0x3f,0x4f,0x5f,0x6f,0x7f,
	0x8f,0x9f,0xaf,0xbf,0xcf,0xdf,0xef,0xff,
	0x07,0x17,0x27,0x37,0x47,0x57,0x67,0x77,
	0x87,0x97,0xa7,0xb7,0xc7,0xd7,0xe7,0xf7
};

//1 no change

//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_StepFac[2]=
{
	14,
	0
};

//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_MaxTan8[32]=
{
	0x054,
	0x310,
	0x244,
	0x071,
	0x0e1,
	0x123,
	0x189,
	0x0a7,
	0x06c,
	0x260,
	0x23a,
	0x073,
	0x0b6,
	0x167,
	0x17f,
	0x0ab,
	0x065,
	0x28b,
	0x252,
	0x06e,
	0x0bf,
	0x156,
	0x178,
	0x0ae,
	0x06d,
	0x25b,
	0x236,
	0x074,
	0x0b6,
	0x169,
	0x181,
	0x0aa
};

//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_Slope4[16]=
{
	0x310,
	0x244,
	0x189,
	0x123,
	0x260,
	0x23a,
	0x17f,
	0x167,
	0x28b,
	0x252,
	0x178,
	0x156,
	0x25b,
	0x236,
	0x181,
	0x169
};
//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_CLPoint[8]=		//Sensor Center is Weight/2, Hight/2 for RGB
{
	0x2df,
	0x0f0,
	0x294,
	0x116,
	0x29d,
	0x107,
	0x294,
	0x118
};
//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_Radius_File_0[512]=
{
	0x134,
	0x134,
	0x134,
	0x134,
	0x134,
	0x134,
	0x134,
	0x134,
	0x159,
	0x144,
	0x12b,
	0x12e,
	0x150,
	0x15b,
	0x176,
	0x181,
	0x17e,
	0x159,
	0x12b,
	0x138,
	0x15b,
	0x16b,
	0x19a,
	0x1a9,
	0x1a3,
	0x17c,
	0x12b,
	0x143,
	0x162,
	0x1a4,
	0x19a,
	0x1be,
	0x1b1,
	0x17d,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1c7,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1e3,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x120,
	0x120,
	0x120,
	0x120,
	0x120,
	0x120,
	0x120,
	0x120,
	0x13e,
	0x12a,
	0x10d,
	0x124,
	0x126,
	0x12f,
	0x145,
	0x149,
	0x154,
	0x13e,
	0x10d,
	0x126,
	0x138,
	0x14b,
	0x15b,
	0x16c,
	0x164,
	0x14f,
	0x10d,
	0x13b,
	0x152,
	0x175,
	0x15b,
	0x176,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x1a6,
	0x15b,
	0x187,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x130,
	0x130,
	0x130,
	0x130,
	0x130,
	0x130,
	0x130,
	0x130,
	0x145,
	0x138,
	0x118,
	0x12a,
	0x134,
	0x140,
	0x15c,
	0x158,
	0x16b,
	0x151,
	0x118,
	0x136,
	0x151,
	0x153,
	0x17a,
	0x183,
	0x194,
	0x169,
	0x118,
	0x13e,
	0x156,
	0x17b,
	0x17a,
	0x19b,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x1c4,
	0x17a,
	0x1b7,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x11f,
	0x11f,
	0x11f,
	0x11f,
	0x11f,
	0x11f,
	0x11f,
	0x11f,
	0x148,
	0x128,
	0x115,
	0x127,
	0x12b,
	0x138,
	0x145,
	0x14c,
	0x165,
	0x14e,
	0x115,
	0x131,
	0x140,
	0x157,
	0x15d,
	0x17c,
	0x17b,
	0x165,
	0x115,
	0x14e,
	0x161,
	0x180,
	0x15d,
	0x190,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x1e5,
	0x15d,
	0x1a0,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
    0x195
};

//This table requires MODE_LINCORR input.
//Then use lsc_util.exe tool to generate
INT16U g_ov8865_Radius_File_1[512]=
{
	0x14c,
	0x13d,
	0x12b,
	0x137,
	0x147,
	0x153,
	0x15e,
	0x153,
	0x16e,
	0x150,
	0x12b,
	0x13b,
	0x157,
	0x15d,
	0x189,
	0x183,
	0x18b,
	0x16e,
	0x12b,
	0x138,
	0x162,
	0x181,
	0x19a,
	0x1b1,
	0x1ae,
	0x175,
	0x12b,
	0x143,
	0x162,
	0x1dc,
	0x19a,
	0x1c5,
	0x1b6,
	0x179,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1db,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1db,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x1b6,
	0x161,
	0x12b,
	0x143,
	0x162,
	0x1b0,
	0x19a,
	0x1dd,
	0x12e,
	0x125,
	0x112,
	0x120,
	0x11e,
	0x12a,
	0x13e,
	0x139,
	0x14e,
	0x12d,
	0x10d,
	0x128,
	0x12f,
	0x13e,
	0x15b,
	0x15d,
	0x167,
	0x144,
	0x10d,
	0x131,
	0x146,
	0x156,
	0x15b,
	0x176,
	0x173,
	0x151,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17d,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x188,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x173,
	0x127,
	0x10d,
	0x142,
	0x152,
	0x17f,
	0x15b,
	0x17b,
	0x136,
	0x130,
	0x118,
	0x128,
	0x128,
	0x138,
	0x14b,
	0x147,
	0x15e,
	0x148,
	0x118,
	0x130,
	0x13d,
	0x145,
	0x16e,
	0x16e,
	0x18e,
	0x157,
	0x118,
	0x13e,
	0x150,
	0x170,
	0x17a,
	0x189,
	0x182,
	0x16e,
	0x118,
	0x150,
	0x156,
	0x1a0,
	0x17a,
	0x1a6,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1b6,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x182,
	0x134,
	0x118,
	0x150,
	0x156,
	0x18e,
	0x17a,
	0x1a8,
	0x131,
	0x120,
	0x111,
	0x11d,
	0x124,
	0x127,
	0x145,
	0x141,
	0x152,
	0x138,
	0x115,
	0x12a,
	0x138,
	0x13a,
	0x15d,
	0x16b,
	0x173,
	0x155,
	0x115,
	0x13e,
	0x158,
	0x15b,
	0x15d,
	0x180,
	0x191,
	0x174,
	0x115,
	0x151,
	0x161,
	0x19a,
	0x15d,
	0x197,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x1ab,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195,
	0x191,
	0x134,
	0x115,
	0x151,
	0x161,
	0x196,
	0x15d,
	0x195
};

