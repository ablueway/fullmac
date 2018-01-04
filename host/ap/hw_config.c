/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#define __SFILE__ "hw_config.c"

#if (AP_MODE_ENABLE == 1)

//---------------------------------------------------------------------------------------------------------------------------------------
// #define	AFTER(x)	((x)+1)
// /*
//  * The following describe the bit masks for different passive scan
//  * capability/requirements per regdomain.
//  */
// #define	NO_PSCAN	0x0ULL			/* NB: must be zero */
// #define	PSCAN_FCC	0x0000000000000001ULL
// #define	PSCAN_FCC_T	0x0000000000000002ULL
// #define	PSCAN_ETSI	0x0000000000000004ULL
// #define	PSCAN_MKK1	0x0000000000000008ULL
// #define	PSCAN_MKK2	0x0000000000000010ULL
// #define	PSCAN_MKKA	0x0000000000000020ULL
// #define	PSCAN_MKKA_G	0x0000000000000040ULL
// #define	PSCAN_ETSIA	0x0000000000000080ULL
// #define	PSCAN_ETSIB	0x0000000000000100ULL
// #define	PSCAN_ETSIC	0x0000000000000200ULL
// #define	PSCAN_WWR	0x0000000000000400ULL
// #define	PSCAN_MKKA1	0x0000000000000800ULL
// #define	PSCAN_MKKA1_G	0x0000000000001000ULL
// #define	PSCAN_MKKA2	0x0000000000002000ULL
// #define	PSCAN_MKKA2_G	0x0000000000004000ULL
// #define	PSCAN_MKK3	0x0000000000008000ULL
// #define	PSCAN_DEFER	0x7FFFFFFFFFFFFFFFULL
// #define	IS_ECM_CHAN	0x8000000000000000ULL
// 
// /* Bit masks for DFS per regdomain */
// enum {
// 	NO_DFS   = 0x0000000000000000ULL,	/* NB: must be zero */
// 	DFS_FCC3 = 0x0000000000000001ULL,
// 	DFS_ETSI = 0x0000000000000002ULL,
// 	DFS_MKK4 = 0x0000000000000004ULL,
// };
// 
// 
// typedef struct {
// 	u16	lowChannel;	/* Low channel center in MHz */
// 	u16	highChannel;	/* High Channel center in MHz */
// 	u8		powerDfs;	/* Max power (dBm) for channel
// 					   range when using DFS */
// 	u8		antennaMax;	/* Max allowed antenna gain */
// 	u8		channelBW;	/* Bandwidth of the channel */
// 	u8		channelSep;	/* Channel separation within
// 					   the band */
// 	u64	useDfs;		/* Use DFS in the RegDomain
// 					   if corresponding bit is set */
// 	u64	usePassScan;	/* Use Passive Scan in the RegDomain
// 					   if corresponding bit is set */
// 	u8		regClassId;	/* Regulatory class id */
// } REG_DMN_FREQ_BAND;
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/*
 * 2GHz 11b channel tags
 */
// static REG_DMN_FREQ_BAND regDmn2GhzFreq[] = {
// 	{ 2312, 2372, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2312_2372	0
// 	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F2_2312_2372	AFTER(F1_2312_2372)
// 
// 	{ 2412, 2472, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2412_2472	AFTER(F2_2312_2372)
// 	{ 2412, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA, 0},
// #define	F2_2412_2472	AFTER(F1_2412_2472)
// 	{ 2412, 2472, 30, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F3_2412_2472	AFTER(F2_2412_2472)
// 
// 	{ 2412, 2462, 27, 6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2412_2462	AFTER(F3_2412_2472)
// 	{ 2412, 2462, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA, 0},
// #define	F2_2412_2462	AFTER(F1_2412_2462)
// 
// 	{ 2432, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2432_2442	AFTER(F2_2412_2462)
// 
// 	{ 2457, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2457_2472	AFTER(F1_2432_2442)
// 
// 	{ 2467, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA2 | PSCAN_MKKA, 0},
// #define	F1_2467_2472	AFTER(F1_2457_2472)
// 
// 	{ 2484, 2484, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2484_2484	AFTER(F1_2467_2472)
// 	{ 2484, 2484, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA | PSCAN_MKKA1 | PSCAN_MKKA2, 0},
// #define	F2_2484_2484	AFTER(F1_2484_2484)
// 
// 	{ 2512, 2732, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	F1_2512_2732	AFTER(F2_2484_2484)
// 
// 	/*
// 	 * WWR have powers opened up to 20dBm.
// 	 * Limits should often come from CTL/Max powers
// 	 */
// 	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2312_2372	AFTER(F1_2512_2732)
// 	{ 2412, 2412, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2412_2412	AFTER(W1_2312_2372)
// 	{ 2417, 2432, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2417_2432	AFTER(W1_2412_2412)
// 	{ 2437, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2437_2442	AFTER(W1_2417_2432)
// 	{ 2447, 2457, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2447_2457	AFTER(W1_2437_2442)
// 	{ 2462, 2462, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	W1_2462_2462	AFTER(W1_2447_2457)
// 	{ 2467, 2467, 20, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0},
// #define	W1_2467_2467	AFTER(W1_2462_2462)
// 	{ 2467, 2467, 20, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},
// #define	W2_2467_2467	AFTER(W1_2467_2467)
// 	{ 2472, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0},
// #define	W1_2472_2472	AFTER(W2_2467_2467)
// 	{ 2472, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},
// #define	W2_2472_2472	AFTER(W1_2472_2472)
// 	{ 2484, 2484, 20, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0},
// #define	W1_2484_2484	AFTER(W2_2472_2472)
// 	{ 2484, 2484, 20, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},
// #define	W2_2484_2484	AFTER(W1_2484_2484)
// };
// 
// 
// /*
//  * 2GHz 11g channel tags
//  */
// static REG_DMN_FREQ_BAND regDmn2Ghz11gFreq[] = {
// 	{ 2312, 2372, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2312_2372	0
// 	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G2_2312_2372	AFTER(G1_2312_2372)
// 	{ 2312, 2372, 5,  6, 10, 5, NO_DFS, NO_PSCAN, 0},
// #define	G3_2312_2372	AFTER(G2_2312_2372)
// 	{ 2312, 2372, 5,  6,  5, 5, NO_DFS, NO_PSCAN, 0},
// #define	G4_2312_2372	AFTER(G3_2312_2372)
// 
// 	{ 2412, 2472, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2412_2472	AFTER(G4_2312_2372)
// 	{ 2412, 2472, 20, 0, 20, 5,  NO_DFS, PSCAN_MKKA_G, 0},
// #define	G2_2412_2472	AFTER(G1_2412_2472)
// 	{ 2412, 2472, 30, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G3_2412_2472	AFTER(G2_2412_2472)
// 	{ 2412, 2472, 5,  6, 10, 5, NO_DFS, NO_PSCAN, 0},
// #define	G4_2412_2472	AFTER(G3_2412_2472)
// 	{ 2412, 2472, 5,  6,  5, 5, NO_DFS, NO_PSCAN, 0},
// #define	G5_2412_2472	AFTER(G4_2412_2472)
// 
// 	{ 2412, 2462, 27, 6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2412_2462	AFTER(G5_2412_2472)
// 	{ 2412, 2462, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA_G, 0},
// #define	G2_2412_2462	AFTER(G1_2412_2462)
// 	{ 2412, 2462, 27, 6, 10, 5, NO_DFS, NO_PSCAN, 0},
// #define	G3_2412_2462	AFTER(G2_2412_2462)
// 	{ 2412, 2462, 27, 6,  5, 5, NO_DFS, NO_PSCAN, 0},
// #define	G4_2412_2462	AFTER(G3_2412_2462)
// 	
// 	{ 2432, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2432_2442	AFTER(G4_2412_2462)
// 
// 	{ 2457, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2457_2472	AFTER(G1_2432_2442)
// 
// 	{ 2512, 2732, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	G1_2512_2732	AFTER(G1_2457_2472)
// 	{ 2512, 2732, 5,  6, 10, 5, NO_DFS, NO_PSCAN, 0},
// #define	G2_2512_2732	AFTER(G1_2512_2732)
// 	{ 2512, 2732, 5,  6,  5, 5, NO_DFS, NO_PSCAN, 0},
// #define	G3_2512_2732	AFTER(G2_2512_2732)
// 
// 	{ 2467, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA2 | PSCAN_MKKA, 0 },
// #define	G1_2467_2472	AFTER(G3_2512_2732)
// 
// 	/*
// 	 * WWR open up the power to 20dBm
// 	 */
// 	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2312_2372	AFTER(G1_2467_2472)
// 	{ 2412, 2412, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2412_2412	AFTER(WG1_2312_2372)
// 	{ 2417, 2432, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2417_2432	AFTER(WG1_2412_2412)
// 	{ 2437, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2437_2442	AFTER(WG1_2417_2432)
// 	{ 2447, 2457, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2447_2457	AFTER(WG1_2437_2442)
// 	{ 2462, 2462, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},
// #define	WG1_2462_2462	AFTER(WG1_2447_2457)
// 	{ 2467, 2467, 20, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0},
// #define	WG1_2467_2467	AFTER(WG1_2462_2462)
// 	{ 2467, 2467, 20, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},
// #define	WG2_2467_2467	AFTER(WG1_2467_2467)
// 	{ 2472, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0},
// #define	WG1_2472_2472	AFTER(WG2_2467_2467)
// 	{ 2472, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},
// #define	WG2_2472_2472	AFTER(WG1_2472_2472)
// 
// 	/*
// 	 * Mapping for 900MHz cards like Ubiquiti SR9 and XR9
// 	 * and ZComax GZ-901.
// 	 */
// 	{ 2422, 2437, 30, 0,  5, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_907_922_5	AFTER(WG2_2472_2472)
// 	{ 2422, 2437, 30, 0, 10, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_907_922_10	AFTER(S1_907_922_5)
// 	{ 2427, 2432, 30, 0, 20, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_912_917	AFTER(S1_907_922_10)
// 	{ 2427, 2442, 30, 0,  5, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S2_907_922_5	AFTER(S1_912_917)
// 	{ 2427, 2442, 30, 0, 10, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S2_907_922_10	AFTER(S2_907_922_5)
// 	{ 2432, 2437, 30, 0, 20, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S2_912_917	AFTER(S2_907_922_10)
// 	{ 2452, 2467, 30, 0,  5, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_908_923_5	AFTER(S2_912_917)
// 	{ 2457, 2467, 30, 0, 10, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_913_918_10	AFTER(S1_908_923_5)
// 	{ 2457, 2467, 30, 0, 20, 5, NO_DFS, PSCAN_FCC, 0 },
// #define	S1_913_918	AFTER(S1_913_918_10)
// };

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// typedef struct regDomain {
// 	uint16_t regDmnEnum;		/* value from EnumRd table */
// 	uint8_t conformanceTestLimit;
// 	uint32_t flags;			/* Requirement flags (AdHoc disallow,
// 					   noise floor cal needed, etc) */
// 	uint64_t dfsMask;		/* DFS bitmask for 5Ghz tables */
// 	uint64_t pscan;			/* Bitmask for passive scan */
// 	chanbmask_t chan11a;		/* 11a channels */
// 	chanbmask_t chan11a_turbo;	/* 11a static turbo channels */
// 	chanbmask_t chan11a_dyn_turbo;	/* 11a dynamic turbo channels */
// 	chanbmask_t chan11a_half;	/* 11a 1/2 width channels */
// 	chanbmask_t chan11a_quarter;	/* 11a 1/4 width channels */
// 	chanbmask_t chan11b;		/* 11b channels */
// 	chanbmask_t chan11g;		/* 11g channels */
// 	chanbmask_t chan11g_turbo;	/* 11g dynamic turbo channels */
// 	chanbmask_t chan11g_half;	/* 11g 1/2 width channels */
// 	chanbmask_t chan11g_quarter;	/* 11g 1/4 width channels */
// } REG_DOMAIN;
// 
// static REG_DOMAIN regDomains[] = {
// 
// 	{.regDmnEnum		= DEBUG_REG_DMN,
// 	 .conformanceTestLimit	= FCC,
// 	 .dfsMask		= DFS_FCC3,
// 	 .chan11a		= BM4(F1_4950_4980,
// 				      F1_5120_5240,
// 				      F1_5260_5700,
// 				      F1_5745_5825),
// 	 .chan11a_half		= BM4(F1_4945_4985,
// 				      F2_5120_5240,
// 				      F2_5260_5700,
// 				      F7_5745_5825),
// 	 .chan11a_quarter	= BM4(F1_4942_4987,
// 				      F3_5120_5240,
// 				      F3_5260_5700,
// 				      F8_5745_5825),
// 	 .chan11a_turbo		= BM8(T1_5130_5210,
// 				      T1_5250_5330,
// 				      T1_5370_5490,
// 				      T1_5530_5650,
// 				      T1_5150_5190,
// 				      T1_5230_5310,
// 				      T1_5350_5470,
// 				      T1_5510_5670),
// 	 .chan11a_dyn_turbo	= BM4(T1_5200_5240,
// 				      T1_5280_5280,
// 				      T1_5540_5660,
// 				      T1_5765_5805),
// 	 .chan11b		= BM4(F1_2312_2372,
// 				      F1_2412_2472,
// 				      F1_2484_2484,
// 				      F1_2512_2732),
// 	 .chan11g		= BM3(G1_2312_2372, G1_2412_2472, G1_2512_2732),
// 	 .chan11g_turbo		= BM3(T1_2312_2372, T1_2437_2437, T1_2512_2732),
// 	 .chan11g_half		= BM3(G2_2312_2372, G4_2412_2472, G2_2512_2732),
// 	 .chan11g_quarter	= BM3(G3_2312_2372, G5_2412_2472, G3_2512_2732),
// 	},
// 
// 	{.regDmnEnum		= APL1,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM1(F4_5745_5825),
// 	},
// 
// 	{.regDmnEnum		= APL2,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM1(F1_5745_5805),
// 	},
// 
// 	{.regDmnEnum		= APL3,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM2(F1_5280_5320, F2_5745_5805),
// 	},
// 
// 	{.regDmnEnum		= APL4,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM2(F4_5180_5240, F3_5745_5825),
// 	},
// 
// 	{.regDmnEnum		= APL5,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM1(F2_5745_5825),
// 	},
// 
// 	{.regDmnEnum		= APL6,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_FCC_T | PSCAN_FCC,
// 	 .chan11a		= BM3(F4_5180_5240, F2_5260_5320, F3_5745_5825),
// 	 .chan11a_turbo		= BM3(T2_5210_5210, T1_5250_5290, T1_5760_5800),
// 	},
// 
// 	{.regDmnEnum		= APL8,
// 	 .conformanceTestLimit	= ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A|DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM2(F6_5260_5320, F4_5745_5825),
// 	},
// 
// 	{.regDmnEnum		= APL9,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A|DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(F1_5180_5320, F1_5500_5620, F3_5745_5805),
// 	},
// 
// 	{.regDmnEnum		= ETSI1,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(W2_5180_5240, F2_5260_5320, F2_5500_5700),
// 	},
// 
// 	{.regDmnEnum		= ETSI2,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM1(F3_5180_5240),
// 	},
// 
// 	{.regDmnEnum		= ETSI3,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM2(W2_5180_5240, F2_5260_5320),
// 	},
// //--------------------------------------------------------
// 	{.regDmnEnum		= ETSI4,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM2(F3_5180_5240, F1_5260_5320),
// 	},
// //---------------------------------------------------
// 	{.regDmnEnum		= ETSI5,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM1(F1_5180_5240),
// 	},
// 
// 	{.regDmnEnum		= ETSI6,
// 	 .conformanceTestLimit	= ETSI,
// 	 .dfsMask		= DFS_ETSI,
// 	 .pscan			= PSCAN_ETSI,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(F5_5180_5240, F1_5260_5280, F3_5500_5700),
// 	},
// 
// 	{.regDmnEnum		= FCC1,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM3(F2_5180_5240, F4_5260_5320, F5_5745_5825),
// 	 .chan11a_turbo		= BM3(T1_5210_5210, T2_5250_5290, T2_5760_5800),
// 	 .chan11a_dyn_turbo	= BM3(T1_5200_5240, T1_5280_5280, T1_5765_5805),
// 	},
// 
// 	{.regDmnEnum		= FCC2,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM3(F6_5180_5240, F5_5260_5320, F6_5745_5825),
// 	 .chan11a_dyn_turbo	= BM3(T2_5200_5240, T1_5280_5280, T1_5765_5805),
// 	},
// 
// 	{.regDmnEnum		= FCC3,
// 	 .conformanceTestLimit	= FCC,
// 	 .dfsMask		= DFS_FCC3,
// 	 .pscan			= PSCAN_FCC | PSCAN_FCC_T,
// 	 .chan11a		= BM4(F2_5180_5240,
// 				      F3_5260_5320,
// 				      F1_5500_5700,
// 				      F5_5745_5825),
// 	 .chan11a_turbo		= BM4(T1_5210_5210,
// 				      T1_5250_5250,
// 				      T1_5290_5290,
// 				      T2_5760_5800),
// 	 .chan11a_dyn_turbo	= BM3(T1_5200_5240, T2_5280_5280, T1_5540_5660),
// 	},
// 
// 	{.regDmnEnum		= FCC4,
// 	 .conformanceTestLimit	= FCC,
// 	 .dfsMask		= DFS_FCC3,
// 	 .pscan			= PSCAN_FCC | PSCAN_FCC_T,
// 	 .chan11a		= BM1(F1_4950_4980),
// 	 .chan11a_half		= BM1(F1_4945_4985),
// 	 .chan11a_quarter	= BM1(F1_4942_4987),
// 	},
// 
// 	/* FCC1 w/ 1/2 and 1/4 width channels */
// 	{.regDmnEnum		= FCC5,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11a		= BM3(F2_5180_5240, F4_5260_5320, F5_5745_5825),
// 	 .chan11a_turbo		= BM3(T1_5210_5210, T2_5250_5290, T2_5760_5800),
// 	 .chan11a_dyn_turbo	= BM3(T1_5200_5240, T1_5280_5280, T1_5765_5805),
// 	 .chan11a_half		= BM3(F7_5180_5240, F7_5260_5320, F9_5745_5825),
// 	 .chan11a_quarter	= BM3(F8_5180_5240, F8_5260_5320,F10_5745_5825),
// 	},
// 
// 	{.regDmnEnum		= MKK1,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKK1,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM1(F1_5170_5230),
// 	},
// 
// 	{.regDmnEnum		= MKK2,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKK2,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(F1_4920_4980, F1_5040_5080, F1_5170_5230),
// 	.chan11a_half		= BM4(F1_4915_4925,
// 				      F1_4935_4945,
// 				      F1_5035_5040,
// 				      F1_5055_5055),
// 	},
// 
// 	/* UNI-1 even */
// 	{.regDmnEnum		= MKK3,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM1(F4_5180_5240),
// 	},
// 
// 	/* UNI-1 even + UNI-2 */
// 	{.regDmnEnum		= MKK4,
// 	 .conformanceTestLimit	= MKK,
// 	 .dfsMask		= DFS_MKK4,
// 	 .pscan			= PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM2(F4_5180_5240, F2_5260_5320),
// 	},
// 
// 	/* UNI-1 even + UNI-2 + mid-band */
// 	{.regDmnEnum		= MKK5,
// 	 .conformanceTestLimit	= MKK,
// 	 .dfsMask		= DFS_MKK4,
// 	 .pscan			= PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(F4_5180_5240, F2_5260_5320, F4_5500_5700),
// 	},
// 
// 	/* UNI-1 odd + even */
// 	{.regDmnEnum		= MKK6,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKK1,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM2(F2_5170_5230, F4_5180_5240),
// 	},
// 
// 	/* UNI-1 odd + UNI-1 even + UNI-2 */
// 	{.regDmnEnum		= MKK7,
// 	 .conformanceTestLimit	= MKK,
// 	 .dfsMask		= DFS_MKK4,
// 	 .pscan			= PSCAN_MKK1 | PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM3(F1_5170_5230, F4_5180_5240, F2_5260_5320),
// 	},
// 
// 	/* UNI-1 odd + UNI-1 even + UNI-2 + mid-band */
// 	{.regDmnEnum		= MKK8,
// 	 .conformanceTestLimit	= MKK,
// 	 .dfsMask		= DFS_MKK4,
// 	 .pscan			= PSCAN_MKK1 | PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11a		= BM4(F1_5170_5230,
// 				      F4_5180_5240,
// 				      F2_5260_5320,
// 				      F4_5500_5700),
// 	},
// 
//         /* UNI-1 even + 4.9 GHZ */
//         {.regDmnEnum		= MKK9,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
//          .chan11a		= BM7(F1_4915_4925,
// 				      F1_4935_4945,
// 				      F1_4920_4980,
// 				      F1_5035_5040,
// 				      F1_5055_5055,
// 				      F1_5040_5080,
// 				      F4_5180_5240),
//         },
// 
//         /* UNI-1 even + UNI-2 + 4.9 GHZ */
//         {.regDmnEnum		= MKK10,
// 	 .conformanceTestLimit	= MKK,
// 	 .dfsMask		= DFS_MKK4,
// 	 .pscan			= PSCAN_MKK3,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
//          .chan11a		= BM8(F1_4915_4925,
// 				      F1_4935_4945,
// 				      F1_4920_4980,
// 				      F1_5035_5040,
// 				      F1_5055_5055,
// 				      F1_5040_5080,
// 				      F4_5180_5240,
// 				      F2_5260_5320),
//         },
// 
// 	/* Defined here to use when 2G channels are authorised for country K2 */
// 	{.regDmnEnum		= APLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .chan11b		= BM2(F2_2312_2372,F2_2412_2472),
// 	 .chan11g		= BM2(G2_2312_2372,G2_2412_2472),
// 	},
// 
// 	{.regDmnEnum		= ETSIA,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .pscan			= PSCAN_ETSIA,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11b		= BM1(F1_2457_2472),
// 	 .chan11g		= BM1(G1_2457_2472),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= ETSIB,
// 	 .conformanceTestLimit	= ETSI,
// 	 .pscan			= PSCAN_ETSIB,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11b		= BM1(F1_2432_2442),
// 	 .chan11g		= BM1(G1_2432_2442),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= ETSIC,
// 	 .conformanceTestLimit	= ETSI,
// 	 .pscan			= PSCAN_ETSIC,
// 	 .flags			= DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
// 	 .chan11b		= BM1(F3_2412_2472),
// 	 .chan11g		= BM1(G3_2412_2472),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= FCCA,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11b		= BM1(F1_2412_2462),
// 	 .chan11g		= BM1(G1_2412_2462),
// 	 .chan11g_turbo		= BM1(T2_2437_2437),
// 	},
// 
// 	/* FCCA w/ 1/2 and 1/4 width channels */
// 	{.regDmnEnum		= FCCB,
// 	 .conformanceTestLimit	= FCC,
// 	 .chan11b		= BM1(F1_2412_2462),
// 	 .chan11g		= BM1(G1_2412_2462),
// 	 .chan11g_turbo		= BM1(T2_2437_2437),
// 	 .chan11g_half		= BM1(G3_2412_2462),
// 	 .chan11g_quarter	= BM1(G4_2412_2462),
// 	},
// 
// 	{.regDmnEnum		= MKKA,
// 	 .conformanceTestLimit	= MKK,
// 	 .pscan			= PSCAN_MKKA | PSCAN_MKKA_G
// 				| PSCAN_MKKA1 | PSCAN_MKKA1_G
// 				| PSCAN_MKKA2 | PSCAN_MKKA2_G,
// 	 .flags			= DISALLOW_ADHOC_11A_TURB,
// 	 .chan11b		= BM3(F2_2412_2462, F1_2467_2472, F2_2484_2484),
// 	 .chan11g		= BM2(G2_2412_2462, G1_2467_2472),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= MKKC,
// 	 .conformanceTestLimit	= MKK,
// 	 .chan11b		= BM1(F2_2412_2472),
// 	 .chan11g		= BM1(G2_2412_2472),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= WORLD,
// 	 .conformanceTestLimit	= ETSI,
// 	 .chan11b		= BM1(F2_2412_2472),
// 	 .chan11g		= BM1(G2_2412_2472),
// 	 .chan11g_turbo		= BM1(T2_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= WOR0_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_PER_11D,
// 	 .chan11a		= BM5(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM8(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467,
// 				      W1_2484_2484),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= WOR01_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_PER_11D,
// 	 .chan11a		= BM5(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM5(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2417_2432,
// 				      W1_2447_2457),
// 	 .chan11g		= BM5(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR02_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_PER_11D,
// 	 .chan11a		= BM5(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM7(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= EU1_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_PER_11D,
// 	 .chan11a		= BM5(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM7(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W2_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W2_2467_2467),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG2_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG2_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR1_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM5(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11b		= BM8(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467,
// 				      W1_2484_2484),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)
// 	},
// 
// 	{.regDmnEnum		= WOR2_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM5(W1_5260_5320,
// 	 W1_5180_5240,
// 	 W1_5170_5230,
// 	 W1_5745_5825,
// 	 W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM8(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467,
// 				      W1_2484_2484),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR3_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_PER_11D,
// 	 .chan11a		= BM4(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5170_5230,
// 				      W1_5745_5825),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM7(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR4_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM4(W2_5260_5320,
// 				      W2_5180_5240,
// 				      F2_5745_5805,
// 				      W2_5825_5825),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM5(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2417_2432,
// 				      W1_2447_2457),
// 	 .chan11g		= BM5(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR5_ETSIC,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM3(W1_5260_5320, W2_5180_5240, F6_5745_5825),
// 	 .chan11b		= BM7(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W2_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W2_2467_2467),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG2_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG2_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WOR9_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM4(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11a_turbo		= BM3(WT1_5210_5250,
// 				      WT1_5290_5290,
// 				      WT1_5760_5800),
// 	 .chan11b		= BM5(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2417_2432,
// 				      W1_2447_2457),
// 	 .chan11g		= BM5(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= WORA_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .dfsMask		= DFS_FCC3 | DFS_ETSI,
// 	 .pscan			= PSCAN_WWR,
// 	 .flags			= ADHOC_NO_11A,
// 	 .chan11a		= BM4(W1_5260_5320,
// 				      W1_5180_5240,
// 				      W1_5745_5825,
// 				      W1_5500_5700),
// 	 .chan11b		= BM7(W1_2412_2412,
// 				      W1_2437_2442,
// 				      W1_2462_2462,
// 				      W1_2472_2472,
// 				      W1_2417_2432,
// 				      W1_2447_2457,
// 				      W1_2467_2467),
// 	 .chan11g		= BM7(WG1_2412_2412,
// 				      WG1_2437_2442,
// 				      WG1_2462_2462,
// 				      WG1_2472_2472,
// 				      WG1_2417_2432,
// 				      WG1_2447_2457,
// 				      WG1_2467_2467),
// 	 .chan11g_turbo		= BM1(T3_2437_2437)},
// 
// 	{.regDmnEnum		= SR9_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .pscan			= PSCAN_FCC | PSCAN_FCC_T,
// 	 .chan11g		= BM1(S1_912_917),
// 	 .chan11g_half		= BM1(S1_907_922_10),
// 	 .chan11g_quarter	= BM1(S1_907_922_5),
// 	},
// 
// 	{.regDmnEnum		= XR9_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .pscan			= PSCAN_FCC | PSCAN_FCC_T,
// 	 .chan11g		= BM1(S2_912_917),
// 	 .chan11g_half		= BM1(S2_907_922_10),
// 	 .chan11g_quarter	= BM1(S2_907_922_5),
// 	},
// 
// 	{.regDmnEnum		= GZ901_WORLD,
// 	 .conformanceTestLimit	= NO_CTL,
// 	 .pscan			= PSCAN_FCC | PSCAN_FCC_T,
// 	 .chan11g		= BM1(S1_913_918),
// 	 .chan11g_half		= BM1(S1_913_918_10),
// 	 .chan11g_quarter	= BM1(S1_908_923_5),
// 	},
// 
// 	{.regDmnEnum		= NULL1,
// 	 .conformanceTestLimit	= NO_CTL,
// 	}
// };
#endif
