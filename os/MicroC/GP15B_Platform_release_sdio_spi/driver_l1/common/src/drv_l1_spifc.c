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
#include "drv_l1_spifc.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"

#if (defined _DRV_L1_SPIFC) && (_DRV_L1_SPIFC == 1)					  
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SPIFC_TIMEOUT	3000

#define C_SPIFC_IDLE	(1 << 0)
#define C_CLK_STATE_HI	(1 << 1)
#define C_MANUAL_MODE	(1 << 8)
#define C_IGNORE_CLK	(1 << 9)
#define C_RX_EMPTY		(1 << 14)
#define C_SW_RESET		(1 << 15)

#define C_WITHOUT_CMD	(1 << 8)
#define C_CMD_ONLY		(1 << 9)
#define C_ONE_CMD		(1 << 13)

#define C_WITHOUT_ADDR	(1 << 12)
#define C_UPTO_ADDR		(1 << 13)
#define C_WITHOUT_ENHAN	(1 << 14)

// spi flash command
#define CMD_WRITE_EN		0x06
#define CMD_WRITE_DIS		0x04
#define CMD_READ_ID			0x9F
#define CMD_READ_STATUS		0x05
#define CMD_WRITE_STATUS	0x01
#define CMD_SECTOR_ERASE	0x20
#define CMD_BLOCK_ERASE		0xD8	//0x52
#define CMD_CHIP_ERASE		0x60	//0xC7		
#define CMD_PAGE_READ		0x03
#define CMD_FAST_READ		0x0B
#define CMD_PAGE_READ_2X	0xBB
#define CMD_PAGE_READ_4X	0xEB
#define CMD_PAGE_PROGRAM	0x02
#define CMD_PAGE_PROGRAM_4X	0x38

// spi flash status bit 
#define C_WIP_BIT			(1 << 0)
#define C_QUAD_BIT			(1 << 6)

// spi flash enhance mode
#define ENHANCE_EN_VALUE	0x5A
#define ENHANCE_DIS_VALUE	0xAA

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR_SPIFC_CTRL(ctrl) \
{ \
	ctrl.set_ahb_en = 0; \
	ctrl.command_mode = 0; \
	ctrl.command = 0; \
	ctrl.enhance_data = 0; \
	ctrl.dummy_clk = 0; \
	ctrl.cmd_mio = IO_1BIT; \
	ctrl.addr_mio = IO_1BIT; \
	ctrl.data_mio = IO_1BIT; \
	ctrl.spi_addr = 0; \
	ctrl.tx_buf = 0; \
	ctrl.tx_cblen = 0; \
	ctrl.rx_buf = 0; \
	ctrl.rx_cblen = 0; \
}

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct spifcReg_s 
{
	volatile INT32U	REG_SPI;	//0xC0010000
	volatile INT32U	REG_CMD;	//0xC0010004
	volatile INT32U	REG_PARA;	//0xC0010008
	volatile INT32U	REG_ADDRL;	//0xC001000C
	volatile INT32U	REG_ADDRH;	//0xC0010010
	volatile INT32U	REG_TX_WD;	//0xC0010014
	volatile INT32U	REG_RX_RD;	//0xC0010018
	volatile INT32U	REG_TX_BC;	//0xC001001C
	volatile INT32U	REG_RX_BC;	//0xC0010020
	volatile INT32U	REG_TIMING;	//0xC0010024
	volatile INT32U	REG_OTHER;	//0xC0010028
	volatile INT32U	REG_CTRL;	//0xC001002C
} spifcReg_t;

typedef struct BkupReg_s
{
	INT32U	REG_SPI;
	INT32U	REG_CMD;
	INT32U	REG_PARA;
	INT32U	REG_CTRL;
} BkupReg_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#ifndef __CS_COMPILER__
#pragma arm section rwdata="spifc_section", code="spifc_section"
#else
void drv_l1_spifc_init(INT32U speed) __attribute__ ((section(".spifc_section")));
void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT8U sample_delay) __attribute__ ((section(".spifc_section")));

static void msec_wait(INT32U msec) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_wait_flag(INT32U flag) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_wait_fifo_not_empty(void) __attribute__ ((section(".spifc_section")));
static INT32S drv_l1_spifc_set_mode(spifcCtrl_t *pCtrl) __attribute__ ((section(".spifc_section")));
static void drv_l1_spifc_save_para(BkupReg_t *backup) __attribute__ ((section(".spifc_section")));
static void drv_l1_spifc_restore_para(BkupReg_t *backup) __attribute__ ((section(".spifc_section")));

INT32S drv_l1_spifcflash_write_enable(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_disable(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_read_id(INT8U *ID) __attribute__ ((section(".spifc_section")));
INT32U drv_l1_spifcflash_read_status(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_wait_status(INT32U msec) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_chip_erase(void) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_sector_erase(INT32U addr) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_block_erase(INT32U addr) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_read_page(INT8U mio, INT32U addr, INT8U *rx_buf, INT32U cblen) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifcflash_write_page(INT8U mio, INT32U addr, INT8U *tx_buf, INT32U cblen) __attribute__ ((section(".spifc_section")));
INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en) __attribute__ ((section(".spifc_section")));
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern INT32U MCLK;

/**
 * @brief   SPIFC init
 * @param   speed[in]: clock speed
 * @return 	none
 */
void drv_l1_spifc_init(INT32U speed)
{
	INT32U div;
	INT32U reg = R_SYSTEM_PLLEN;
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
	reg &= ~(1<<15 | 1<<9); //clock source form SYSCLK
	div = ((MCLK / speed) >> 1) - 1;
	reg &= ~(0x1F << 10);		
	reg |= (div << 10);
	R_SYSTEM_PLLEN = reg;
		
	// spifc enable
	pSpifcReg->REG_CTRL = (1 << 0); 
}

/**
 * @brief   set SPIFC timing
 * @param   ignore_last_clk: spi clock ignor last clock for some special flash use
 * @param   clk_idle_level: spi clock pin idle state
 * @param   sample_delay: receive data, spi clock delay n halt T. 
 * @return 	none
 */
void drv_l1_spifc_set_timing(INT8U ignore_last_clk, INT8U clk_idle_level, INT8U sample_delay)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	INT32U reg_spi = pSpifcReg->REG_SPI;
	
	if(ignore_last_clk) {
		reg_spi |= C_IGNORE_CLK;
	} else {
		reg_spi &= ~C_IGNORE_CLK;
	}

	if(clk_idle_level) {
		reg_spi |= C_CLK_STATE_HI;
	} else {
		reg_spi &= ~C_CLK_STATE_HI;
	}
	
	pSpifcReg->REG_SPI = reg_spi;
	pSpifcReg->REG_TIMING = (7 << 8) | (sample_delay & 0x07);	// cs_high_cnt = 7
}

/**
 * @brief   wait msec
 * @param   msec
 * @return 	none
 */
static void msec_wait(INT32U msec)
{
#if 1
	INT32U cnt = msec * (MCLK >> 13); //0.64ms
	INT32U i;
	
	for(i=0; i<cnt; i++) {
		i = i;
	} 
#else
	drv_msec_wait(msec);
#endif
}

/**
 * @brief   wait SPIFC flag
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
static INT32S drv_l1_spifc_wait_flag(INT32U flag)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
#if 0
	INT32S i;

	for(i=0; i<SPIFC_TIMEOUT; i++) { 
		if((pSpifcReg->REG_SPI & flag) == flag) {
			break;
		}
		
		msec_wait(1);
	}
	
	if(i == SPIFC_TIMEOUT) {
		return STATUS_FAIL;
	}
#else
	while(1) {
		if((pSpifcReg->REG_SPI & flag) == flag) {
			break;
		}
	}
#endif	

	return STATUS_OK;
}

/**
 * @brief   wait SPIFC rx fifo not empty
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
static INT32S drv_l1_spifc_wait_fifo_not_empty(void)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
#if 0
	INT32S i;

	for(i=0; i<SPIFC_TIMEOUT; i++) { 
		if((pSpifcReg->REG_SPI & C_RX_EMPTY) == 0) {
			break;
		}
		
		msec_wait(1);
	}
	
	if(i == SPIFC_TIMEOUT) {
		return STATUS_FAIL;
	}
#else
	while(1) {
		if((pSpifcReg->REG_SPI & C_RX_EMPTY) == 0) {
			break;
		}
	}
#endif	
	return STATUS_OK;
}

/**
 * @brief   set SPIFC runing mode
 * @param   pCtrl: mode setting
 * @return 	STATUS_OK/STATUS_FAIL;
 */
static INT32S drv_l1_spifc_set_mode(spifcCtrl_t *pCtrl)
{
	INT8U *pData;
	INT32S cnt, ret = STATUS_OK;
	INT32U *pWData;
	INT32U reg_spi, reg_cmd, reg_para, reg_ctrl;
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
	// wait idle
	if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
		return STATUS_FAIL;
	}

	// set io number
	reg_spi = pSpifcReg->REG_SPI;
	reg_spi &= ~((1 << 10) | (0x3F << 2));
	reg_spi |= ((pCtrl->cmd_mio & 0x03) << 6) | 
				((pCtrl->addr_mio & 0x03) << 4) |
				((pCtrl->data_mio & 0x03) << 2);
		
	// set command
	reg_cmd = pSpifcReg->REG_CMD;
	reg_cmd &= ~0xFF;
	reg_cmd |= pCtrl->command & 0xFF;
			
	// set parameter
	reg_para = pSpifcReg->REG_PARA;
	reg_para &= ~0xFFF;
	reg_para |= ((pCtrl->dummy_clk & 0x0F) << 8) | 
			(pCtrl->enhance_data & 0xFF);
	
	// set address is 24 bit or 32bit mode
	reg_ctrl = pSpifcReg->REG_CTRL;
	if(pCtrl->spi_addr >> 24) {
		reg_ctrl |= 0xFF << 8;
	} 
	else {
		reg_ctrl &= ~(0xFF << 8);
	}
	
	switch(pCtrl->command_mode) 
	{
	case COMMAND_ONLY:				//write enable, write disable, chip earse command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1
	
		reg_cmd &= ~C_WITHOUT_CMD;		//wo_cmd = 0
		reg_cmd |= C_CMD_ONLY;			//cmd_only = 1
		reg_cmd &= ~C_ONE_CMD;			//one_cmd = 0	
		
		pCtrl->tx_cblen = 0;
		pCtrl->rx_cblen = 0;
		break;
		
	case COMMAND_ADDR:				//sector earse, block earse command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1
		
		reg_cmd &= ~C_WITHOUT_CMD;		//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;			//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;			//one_cmd = 0
		
		reg_para &= ~C_WITHOUT_ADDR; 	//wo_addr = 0;
		reg_para |= C_UPTO_ADDR;		//upto_addr = 1;	

		pCtrl->tx_cblen = 0;
		pCtrl->rx_cblen = 0;
		break;
	
	case COMMAND_ADDR_TX:			//write page command
	case COMMAND_ADDR_RX:			//read page command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1
		
		reg_cmd &= ~C_WITHOUT_CMD;		//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;			//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;			//one_cmd = 0
		
		reg_para |= C_WITHOUT_ENHAN; 	//wo_enhan = 1;
		reg_para &= ~C_WITHOUT_ADDR; 	//wo_addr = 0;
		reg_para &= ~C_UPTO_ADDR;	 	//upto_addr = 0;
		
		if(pCtrl->command_mode == COMMAND_ADDR_TX) {
			pCtrl->rx_cblen = 0;
		} else {
			pCtrl->tx_cblen = 0;	
		}
		break;
		
	case COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX:
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1
		
		reg_cmd &= ~C_WITHOUT_CMD;		//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;			//cmd_only = 0
		if(pCtrl->enhance_data == ENHANCE_DIS_VALUE) {	
			reg_cmd &= ~C_ONE_CMD;		//one_cmd = 0, enhance disable 
		} else {
			reg_cmd |= C_ONE_CMD;		//one_cmd = 1, enhance enable 
		}
		
		reg_para &= ~C_WITHOUT_ENHAN; 	//wo_enhan = 0;
		reg_para &= ~C_WITHOUT_ADDR; 	//wo_addr = 0;
		reg_para &= ~C_UPTO_ADDR;	 	//upto_addr = 0;
		
		pCtrl->tx_cblen = 0;
		break;
	
	case COMMAND_TX:	// write status command
	case COMMAND_RX:				//read status, read id command
		reg_spi |= C_MANUAL_MODE;	//manual mode = 1
		
		reg_cmd &= ~C_WITHOUT_CMD;		//wo_cmd = 0
		reg_cmd &= ~C_CMD_ONLY;			//cmd_only = 0
		reg_cmd &= ~C_ONE_CMD;			//one_cmd = 0
		
		reg_para |= C_WITHOUT_ENHAN; 	//wo_enhan = 1;
		reg_para |= C_WITHOUT_ADDR;  	//wo_addr = 1;
		reg_para &= ~C_UPTO_ADDR;	 	//upto_addr = 0;
		
		if(pCtrl->command_mode == COMMAND_TX) {
			pCtrl->rx_cblen = 0;
		} else {
			pCtrl->tx_cblen = 0;
		}
		break;
		
	default:
		ret = STATUS_FAIL;
		goto __exit;	
	}
	
	// for ahb, disable manual mode. 
	if(pCtrl->set_ahb_en) {
		reg_spi &= ~C_MANUAL_MODE;	//auto mode
	}

	// clear register
	pSpifcReg->REG_SPI |= C_SW_RESET;
	
	// set spi flash addr 
	pSpifcReg->REG_ADDRL = pCtrl->spi_addr & 0xFFFF;
	pSpifcReg->REG_ADDRH = (pCtrl->spi_addr >> 16) & 0xFFFF;
	
	// set spi flash address bit number
	pSpifcReg->REG_CTRL = reg_ctrl;

	// transcation size
	pSpifcReg->REG_TX_BC = pCtrl->tx_cblen;
	pSpifcReg->REG_RX_BC = pCtrl->rx_cblen;
	
	// set enhance
	pSpifcReg->REG_PARA = reg_para;

	// set spifc ctrl 
	pSpifcReg->REG_SPI = reg_spi;

	// set command, then start transcation
	pSpifcReg->REG_CMD = reg_cmd;
	
	// transcation
	if(pCtrl->tx_cblen && pCtrl->tx_buf) {
		cnt = pCtrl->tx_cblen;
		pData = pCtrl->tx_buf;
		while(cnt > 0) {			
			pSpifcReg->REG_TX_WD = *pData;
			pData++;		
			cnt--;

			// wait idle = 1
			if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
				ret = STATUS_FAIL;
				goto __exit;
			}
		}
	} 
	
	if(pCtrl->rx_cblen && pCtrl->rx_buf) {
		cnt = (pCtrl->rx_cblen + 3) >> 2; //div4 and 4 align
		pWData = pCtrl->rx_buf;
		while(cnt > 0) {
			// wait idle = 1 & rx_empty = 0
			if(drv_l1_spifc_wait_flag(C_SPIFC_IDLE) < 0) {
				ret = STATUS_FAIL;
				goto __exit;
			}
			
			if(drv_l1_spifc_wait_fifo_not_empty() < 0) {
				ret = STATUS_FAIL;
				goto __exit;
			}
			
			// Must write dummy byte to push data into fifo
			pSpifcReg->REG_RX_RD = 0;
			
			// then read the rx data
			*pWData = pSpifcReg->REG_RX_RD;
			pWData++;		
			cnt--;
		}
	}

__exit:
	return ret;
}

/**
 * @brief   save spifc parameter
 * @param   none
 * @return 	none
 */
static void drv_l1_spifc_save_para(BkupReg_t *backup)
{
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
	// back up register value 
	backup->REG_SPI = pSpifcReg->REG_SPI;
	backup->REG_CMD = pSpifcReg->REG_CMD;
	backup->REG_PARA = pSpifcReg->REG_PARA;
	backup->REG_CTRL = pSpifcReg->REG_CTRL;
}
	
/**
 * @brief   restore spifc parameter
 * @param   none
 * @return 	none
 */
static void drv_l1_spifc_restore_para(BkupReg_t *backup)
{	
	spifcReg_t *pSpifcReg = (spifcReg_t *)SPIFC_BASE;
	
	// recovery register value
	pSpifcReg->REG_SPI = backup->REG_SPI;
	pSpifcReg->REG_PARA = backup->REG_PARA;
	pSpifcReg->REG_TX_BC = 0;
	pSpifcReg->REG_RX_BC = 32;
	pSpifcReg->REG_CTRL = backup->REG_CTRL;
	pSpifcReg->REG_CMD = backup->REG_CMD;
}

/**
 * @brief   set spi flash write enable 
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_enable(void)
{
	spifcCtrl_t ctrl;
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_WRITE_EN;
	return drv_l1_spifc_set_mode(&ctrl);
}

/**
 * @brief   set spi flash write disable 
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_disable(void)
{
	spifcCtrl_t ctrl;
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_WRITE_DIS;
	return drv_l1_spifc_set_mode(&ctrl);
}

/**
 * @brief   read spi flash id 
 * @param   ID: id buffer pointer
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_read_id(INT8U *ID)
{
	INT8U buf[4];
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
		
	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_ID;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 4;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}
	
	ID[0] = buf[0];
	ID[1] = buf[1];
	ID[2] = buf[2];
	ret = STATUS_OK;
__exit:	
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   read spi flash status 
 * @param   none
 * @return 	status or 0;
 */
INT32U drv_l1_spifcflash_read_status(void)
{
	INT8U buf[4];
	INT32S ret;
	spifcCtrl_t ctrl;
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_RX;
	ctrl.command = CMD_READ_STATUS;
	ctrl.rx_buf = (INT32U *)buf;
	ctrl.rx_cblen = 4;	//read length is 4 align
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		ret = 0;
		goto __exit;
	}
	
	ret = (INT32S)buf[0];
__exit:	
	return ret;
}

/**
 * @brief   write spi flash status 
 * @param   enable: enable or disable
 * @param   bit: bit
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_status(INT32U enable, INT32U bit)
{
	INT32S ret;
	INT32U status;
	spifcCtrl_t ctrl;
	
	// read status and check bit is aready set or not
	status = drv_l1_spifcflash_read_status();
	if(enable && (status & bit)) {
		ret = STATUS_OK;
		goto __exit;
	}
	
	if((enable == 0) && ((status & bit) == 0)) {
		ret = STATUS_OK;
		goto __exit;
	}
	
	// set bit
	if(enable) {
		status |= bit;
	} else {
		status &= ~bit;
	}
	
	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}
		
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_TX;
	ctrl.command = CMD_WRITE_STATUS;
	ctrl.tx_buf = (INT8U *)&status;
	ctrl.tx_cblen = 1;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}
	
	// wait program finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
__exit:	
	return ret;
}

/**
 * @brief   set spi flash quad bit 
 * @param   enable: enable or disable
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_set_quad_enable(INT32U enable)
{
	return drv_l1_spifcflash_write_status(enable, C_QUAD_BIT);
}

/**
 * @brief   wait spi flash busy bit 
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_wait_status(INT32U msec)
{
#if 0
	INT32U i, status;

	for(i=0; i<msec; i++) {
		status = drv_l1_spifcflash_read_status();		
		if((status & C_WIP_BIT) == 0) {
			break;
		}
		
		msec_wait(1);
	}
	
	if(i == msec) {
		return STATUS_FAIL;
	}
	
#else
	INT32U status;
	
	while(1) {
		status = drv_l1_spifcflash_read_status();		
		if((status & C_WIP_BIT) == 0) {
			break;
		}
		
		msec_wait(1);
	}
#endif	
	return STATUS_OK;
}

/**
 * @brief   spi flash chip erase 
 * @param   none
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_chip_erase(void)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;
	
	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}
	
	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ONLY;
	ctrl.command = CMD_CHIP_ERASE;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT * 20 * 10);
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;	
}

/**
 * @brief   spi flash sector erase, a sector is 4K 
 * @param   addr: sector erase start address 
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_sector_erase(INT32U addr)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}
	
	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR;
	ctrl.command = CMD_SECTOR_ERASE;
	ctrl.spi_addr = addr >> 12 << 12;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;		
}

/**
 * @brief   spi flash block erase, a block is 64K 
 * @param   addr: block erase start address 
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_block_erase(INT32U addr)
{
	INT32S ret;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}
	
	ret = drv_l1_spifcflash_write_enable();
	if(ret < 0) {
		goto __exit;
	}
	
	CLEAR_SPIFC_CTRL(ctrl);
	ctrl.command_mode = COMMAND_ADDR;
	ctrl.command = CMD_BLOCK_ERASE;
	ctrl.spi_addr = addr >> 16 << 16;
	ret = drv_l1_spifc_set_mode(&ctrl);
	if(ret < 0) {
		goto __exit;
	}

	// wait earse finish
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;		
}

/**
 * @brief   spi flash read page, a page is 256byte 
 * @param   mio: IO_1BIT, IO_2BIT or IO_4BIT
 * @param   addr: spi flash start address
 * @param   buf: read buffer
 * @param   cblen: read length in byte
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_read_page(INT8U mio, INT32U addr, INT8U *rx_buf, INT32U cblen)
{
	INT32S ret;
	INT32U i, PageNum;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
	#if 1
		ctrl.command_mode = COMMAND_ADDR_RX;	
		ctrl.command = CMD_PAGE_READ;
	#else //fast read
		ctrl.command_mode = COMMAND_ADDR_RX;
		ctrl.command = CMD_FAST_READ;
		ctrl.dummy_clk = 8;
	#endif
		break;
	
	case IO_2BIT:
		ctrl.command_mode = COMMAND_ADDR_RX;
		ctrl.command = CMD_PAGE_READ_2X;
		ctrl.dummy_clk = 4;
		ctrl.cmd_mio = IO_1BIT;
		ctrl.addr_mio = IO_2BIT;
		ctrl.data_mio = IO_2BIT;
		break;
		
	case IO_4BIT:	
		// quad bit enable
		ret = drv_l1_spifcflash_set_quad_enable(1);
		if(ret < 0) {
			goto __exit;
		}
		
		ctrl.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
		ctrl.command = CMD_PAGE_READ_4X;
	#if 1	//enhance disable	
		ctrl.enhance_data = ENHANCE_DIS_VALUE;
	#else	//enhance enable
		ctrl.enhance_data = ENHANCE_EN_VALUE;
	#endif
		ctrl.dummy_clk = 4;
		ctrl.cmd_mio = IO_1BIT;
		ctrl.addr_mio = IO_4BIT;
		ctrl.data_mio = IO_4BIT;
		break;
	
	default:
		ret = STATUS_FAIL;
		goto __exit;
	}

	// page read start
	PageNum = cblen >> 8;
	if(PageNum == 0) {
		PageNum = 1;
	}
	
	ctrl.spi_addr = addr >> 8 << 8;
	ctrl.rx_buf = (INT32U *)rx_buf;
	ctrl.rx_cblen = 256;
	for(i=0; i<PageNum; i++) {
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		}
		
		ctrl.spi_addr += 256;
		ctrl.rx_buf = (INT32U *)((INT32U)ctrl.rx_buf + 256);
	}
	
__exit:	
	drv_l1_spifc_restore_para(&backup);
	return ret;
}

/**
 * @brief   spi flash write page, a page is 256byte 
 * @param   mio: IO_1BIT or IO_4BIT 
 * @param   addr: spi flash start address
 * @param   buf: write buffer
 * @param   cblen: write length in byte
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifcflash_write_page(INT8U mio, INT32U addr, INT8U *tx_buf, INT32U cblen)
{
	INT32S ret;
	INT32U i, PageNum;
	spifcCtrl_t ctrl;
	BkupReg_t backup;

	drv_l1_spifc_save_para(&backup);
	ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
	if(ret < 0) {
		goto __exit;
	}
	
	CLEAR_SPIFC_CTRL(ctrl);
	switch(mio)
	{
	case IO_1BIT:
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.command = CMD_PAGE_PROGRAM;
		break;
		
	case IO_4BIT:
		// quad bit enable
		ret = drv_l1_spifcflash_set_quad_enable(1);
		if(ret < 0) {
			goto __exit;
		}
		
		ctrl.command_mode = COMMAND_ADDR_TX;
		ctrl.command = CMD_PAGE_PROGRAM_4X;
		ctrl.cmd_mio = IO_1BIT;
		ctrl.addr_mio = IO_4BIT;
		ctrl.data_mio = IO_4BIT;
		break;
		
	default:
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	// page write start
	PageNum = cblen >> 8;	
	if(PageNum == 0) {
		PageNum = 1;
	}
	
	ctrl.spi_addr = addr >> 8 << 8; 
	ctrl.tx_buf = (INT8U *)tx_buf;
	ctrl.tx_cblen = 256;
	for(i=0; i<PageNum; i++) {
		ret = drv_l1_spifcflash_write_enable();
		if(ret < 0) {
			goto __exit;
		}
	
		ret = drv_l1_spifc_set_mode(&ctrl);
		if(ret < 0) {
			goto __exit;
		} 
		
		ctrl.spi_addr += 256;
		ctrl.tx_buf = (INT8U *)((INT32U)ctrl.tx_buf + 256);
		
		// wait program finish
		ret = drv_l1_spifcflash_wait_status(SPIFC_TIMEOUT);
		if(ret < 0) {
			goto __exit;
		}
	}
	
__exit:
	drv_l1_spifc_restore_para(&backup);
	return ret;	
}

/**
 * @brief   spifc ahb set to 1x read mode 
 * @param   MIO: IO_1BIT, IO_2BIT or IO_4BIT
 * @param   enhance_en: enhance mode enable only for IO_4BIT mode
 * @return 	STATUS_OK/STATUS_FAIL;
 */
INT32S drv_l1_spifc_set_read_mode(INT8U mio, INT8U enhance_en)
{
	INT32S ret;
	spifcCtrl_t ahbSetting;
	
	CLEAR_SPIFC_CTRL(ahbSetting);
	ahbSetting.set_ahb_en = 1;
	ahbSetting.rx_cblen = 32;
	
	switch(mio)
	{
	case IO_1BIT:
	#if 1
		ahbSetting.command_mode = COMMAND_ADDR_RX;	
		ahbSetting.command = CMD_PAGE_READ;
		ahbSetting.dummy_clk = 0;
	#else //fast read
		ahbSetting.command_mode = COMMAND_ADDR_RX;
		ahbSetting.command = CMD_FAST_READ;
		ahbSetting.dummy_clk = 8;
	#endif
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_1BIT;
		ahbSetting.data_mio = IO_1BIT;
		break;
	
	case IO_2BIT:
		ahbSetting.command_mode = COMMAND_ADDR_RX;
		ahbSetting.command = CMD_PAGE_READ_2X;
		ahbSetting.dummy_clk = 4;
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_2BIT;
		ahbSetting.data_mio = IO_2BIT;
		break;
		
	case IO_4BIT:	
		// quad bit enable
		ret = drv_l1_spifcflash_set_quad_enable(1);
		if(ret < 0) {
			goto __exit;
		}
		
		ahbSetting.command_mode = COMMAND_ADDR_ENHAN_DUMMY_RX_ADDR_ENHN_DUMMY_RX;
		ahbSetting.command = CMD_PAGE_READ_4X;
		if(enhance_en) {
			ahbSetting.enhance_data = ENHANCE_EN_VALUE;
		} else {		
			ahbSetting.enhance_data = ENHANCE_DIS_VALUE;
		}

		ahbSetting.dummy_clk = 4;
		ahbSetting.cmd_mio = IO_1BIT;
		ahbSetting.addr_mio = IO_4BIT;
		ahbSetting.data_mio = IO_4BIT;
		break;
	
	default:
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	ret = drv_l1_spifc_set_mode(&ahbSetting);
__exit:	
	return ret;
}

#ifndef __CS_COMPILER__
#pragma arm section rwdata, code
#endif
#endif //(defined _DRV_L1_SPIFC) && (_DRV_L1_SPIFC == 1)
