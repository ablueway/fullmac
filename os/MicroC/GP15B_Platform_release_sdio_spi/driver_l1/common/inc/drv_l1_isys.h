#include "driver_l1.h"
#include "drv_l1_sfr.h"

#ifndef _SDRAM_DRV
#define _SDRAM_DRV
typedef enum {
    SDRAM_DRV_4mA=0x0000,
    SDRAM_DRV_8mA=0x5555,
    SDRAM_DRV_12mA=0xAAAA,
    SDRAM_DRV_16mA=0xFFFF
} SDRAM_DRV;

typedef enum
{
    CL_2=2,
    CL_3=3
} SDRAM_CL;

typedef enum
{
    SD_24MHZ=24,
    SD_48MHZ=48,
    SD_96MHZ=96
} SDRAM_CLK_MHZ;

typedef enum
{
    SDRAM_OUT_DLY_LV0=0,
    SDRAM_OUT_DLY_LV1 ,
    SDRAM_OUT_DLY_LV2 ,
    SDRAM_OUT_DLY_LV3 ,
    SDRAM_OUT_DLY_LV4 ,
    SDRAM_OUT_DLY_LV5 ,
    SDRAM_OUT_DLY_LV6 ,
    SDRAM_OUT_DLY_LV7 ,
    SDRAM_OUT_DLY_LV8 ,
    SDRAM_OUT_DLY_LV9 ,
    SDRAM_OUT_DLY_LV10,
    SDRAM_OUT_DLY_LV11,
    SDRAM_OUT_DLY_LV12,
    SDRAM_OUT_DLY_LV13,
    SDRAM_OUT_DLY_LV14,
    SDRAM_OUT_DLY_LV15
} SD_CLK_OUT_DELAY_CELL;

typedef enum
{
    SDRAM_IN_DLY_LV0=0,
    SDRAM_IN_DLY_LV1 ,
    SDRAM_IN_DLY_LV2 ,
    SDRAM_IN_DLY_LV3 ,
    SDRAM_IN_DLY_LV4 ,
    SDRAM_IN_DLY_LV5 ,
    SDRAM_IN_DLY_LV6 ,
    SDRAM_IN_DLY_LV7 ,
    SDRAM_IN_DLY_DISABLE
} SD_CLK_IN_DELAY_CELL;

typedef enum
{
    SD_CLK_PHASE_0=0,
    SD_CLK_PHASE_180
} SD_CLK_PHASE;

typedef enum
{
    SD_DISABLE=0,
    SD_ENABLE
} SD_SWITCH;

typedef enum
{
    SD_BUS_16bit=0,
    SD_BUS_32bit
} SD_BUS_WIDTH;


typedef enum
{
    SD_BANK_1M=0,
    SD_BANK_4M
} SD_BANK_BOUND;

typedef enum
{
    SD_16Mb=0x0,
    SD_64Mb=0x1,
    SD_128Mb=0x2,
    SD_256Mb=0x3
} SD_SIZE;

typedef enum
{
    tREFI_3o9u=39,
    tREFI_7o8u=78,    /*suggest: when SDRAM size >= 256Mb*/
    tREFI_15o6u=156,  /*suggest: when SDRAM size < 256Mb*/
    tREFI_31o2u=312
} tREFI_ENUM;
#endif

#ifndef _NEG_SAMPLE
#define _NEG_SAMPLE
typedef enum {
    NEG_OFF=0x0,
    NEG_ON=0x2
} NEG_SAMPLE;
#endif

#ifndef _POWER_STATE
#define _POWER_STATE
typedef enum {
	SYS_POWER_STATE_CLKCHG = 0,
	SYS_POWER_STATE_SLOW,
	SYS_POWER_STATE_FAST,
	SYS_POWER_STATE_32KHZ,
	SYS_POWER_STATE_WAIT,
	SYS_POWER_STATE_HALT = 6,
	SYS_POWER_STATE_SLEEP
} POWER_STATE;
#endif

#define SYS_CLK_SRC0_MASK			(0x00004000) //bit14
#define SYS_CLK_SRC1_MASK			(0x00008000) //bit15
#define SYS_FAST_PLL_MASK           (0x0000003F) //bit[5:0]
#define SYS_CLK_DIV_MASK     		(0x00000007) //bit[2:0]


#ifndef PLL_12MHz
#define PLL_12MHz	12000000
#endif

#ifndef PLL_SRC_CLK
#define PLL_SRC_CLK	PLL_12MHz
#endif

extern INT32S sys_pll_set(INT32U PLL_CLK, INT8U sdram_out_dly, INT8U sdram_in_dly, SD_SIZE sdram_size, NEG_SAMPLE neg, SDRAM_DRV sdram_driving);
extern INT32S sys_clk_set(INT32U PLL_CLK);
extern INT32U sys_clk_get(void);