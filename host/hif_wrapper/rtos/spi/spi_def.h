/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _SPI_DEF_H_
#define _SPI_DEF_H_

/******************************************************************************/
/* transfer operations */
/******************************************************************************/
/* transferOptions-Bit0: If this bit is 0 then it means that the transfer size provided is in bytes */
#define	SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES			0x00000000
/* transferOptions-Bit0: If this bit is 1 then it means that the transfer size provided is in bytes */
#define	SPI_TRANSFER_OPTIONS_SIZE_IN_BITS			0x00000001
/* transferOptions-Bit1: if BIT1 is 1 then CHIP_SELECT line will be enables at start of transfer */
#define	SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE		0x00000002
/* transferOptions-Bit2: if BIT2 is 1 then CHIP_SELECT line will be disabled at end of transfer */
#define SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE		0x00000004
/* transferOptions-Bit3: if BIT3 is 1 then transfer by cpu polling mode */
#define SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE		0x00000008
/* transferOptions-Bit4: if BIT4 is 1 then without a temp write buf during read phase */
#define SPI_TRANSFER_OPTIONS_NO_TEMP_READ_BUF		0x00000010

#endif
