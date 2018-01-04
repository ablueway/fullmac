#ifndef __drv_l1_FRONT_H__
#define __drv_l1_FRONT_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//GPIO sel oen
#define SEN_BIT			(1 << 0)
#define FLASH_CTRL_BIT	(1 << 1)
#define SCK_BIT			(1 << 2)
#define SDO_BIT			(1 << 3)
#define HD_BIT			(1 << 4)
#define VD_BIT			(1 << 5)
#define PIX_CLK_BIT		(1 << 6)
#define MCLK_BIT		(1 << 7)
#define SEN1_BIT		(1 << 8)
#define SCK1_BIT		(1 << 9)
#define SD_BIT			(1 << 10)

//TG input signal gating
#define EXT_HDI_BIT		(1 << 0)
#define EXT_VDI_BIT		(1 << 1)
#define TV_VVALIDI_BIT	(1 << 2)
#define TV_HVALIDI_BIT	(1 << 3)
#define TV_DVALIDI_BIT	(1 << 4)
#define SDI_BIT			(1 << 5)
#define SDI2_BIT		(1 << 6)
#define SCLKI_BIT		(1 << 7)
#define SCLKI2_BIT		(1 << 8)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpHalFrontRoi_s
 {
	INT16U hSize;
	INT16U hOffset;
	INT16U vSize;
	INT16U vOffset;
} gpHalFrontRoi_t;

typedef struct gpHalFrontReshape_s
 {
	INT32U mode; // 0: RAW ; 1: YUV ; 2:MIPI
	INT8U hReshapeEn;
	INT8U vReshapeEn;
	INT8U vReshapeClkType; // 0: H-sync ; 1: pclk
	INT8U vBackOffEn;
	INT16U hRise;
	INT16U hFall;
	INT16U vRise;
	INT16U vFall;
} gpHalFrontReshape_t;

extern void drv_l1_FrontReset(void);
extern void drv_l1_FrontStart(void);
extern void drv_l1_FrontStop(void);
extern void drv_l1_FrontInputPathSet(INT8U path);
extern void drv_l1_FrontInputModeSet(INT8U mode);
extern void drv_l1_FrontYuvSubEn(INT32U ySub, INT32U uSub, INT32U vSub);
extern void drv_l1_FrontDataTransMode(INT8U mode);
extern INT32U drv_l1_FrontDataTransModeGet(void);
extern void drv_l1_FrontYuvOrderSet(UINT32 mode);
extern void drv_l1_FrontSetSyncPolarity(INT8U hPol, INT8U vPol, INT8U in1den);
extern void drv_l1_FrontGetSyncPolarity(INT8U *hPol, INT8U *vPol, INT8U *in1den);
extern void drv_l1_FrontInputGate(UINT32 tggz);
extern void drv_l1_FrontIntCfg(INT32U interrupten, INT32U vdrintnum, INT32U vdfintnum, INT32U hdfintnum);
extern void drv_l1_FrontClrIntEvent(INT32U clrintevt);
extern void drv_l1_FrontGetIntEvent(INT32U *pintevtvalue);
extern void drv_l1_FrontSnapTrigger(INT32U number);
extern void drv_l1_FrontSetFlash(INT32U width, INT32U linenum, INT32U mode);
extern void drv_l1_FrontFlashTrigger(void);
extern void drv_l1_FrontSetRoi(gpHalFrontRoi_t roi, INT8U syncVdEn);
extern void drv_l1_FrontGetRoi(gpHalFrontRoi_t* roi);
extern void drv_l1_FrontVdUpdateEn(INT32U enable);
extern INT32U drv_l1_FrontVdIsEnable(INT32U vdflag);
extern void drv_l1_FrontSetReshape(gpHalFrontReshape_t reshapeCtl);
extern void drv_l1_FrontSetSiggen(INT8U siggen_en, INT8U sig_clr_bar, INT32U sigmode);
extern void drv_l1_FrontWaitVd(INT32U mode, INT32U number);
extern INT32U drv_l1_FrontWaitVValid(INT32U mode, INT32U number);

extern INT32S drv_l1_FrontSetInputFmt(INT8U format);
extern void drv_l1_FrontVdIntEn(INT32U enable, INT32U vdtype);
#endif //__drv_l1_FRONT_H__