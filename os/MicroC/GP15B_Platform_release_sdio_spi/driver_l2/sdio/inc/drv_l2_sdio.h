#ifndef __DRV_L2_SDIO_H__
#define __DRV_L2_SDIO_H__

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
**************************************************************************/

//#include "driver_l2.h"

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/

#define MASK_CMD0			0			/*!< @brief GO_IDLE_STATE. */
#define MASK_CMD3			3			/*!< @brief SEND_RELATIVE_ADDR. */
#define MASK_CMD5			5			/*!< @brief SDIO. */
#define MASK_CMD7			7			/*!< @brief SELECT/DESELECT_CARD. */
#define MASK_CMD8           8
// I/O read/write commands
#define MASK_CMD52			52			/*!< @brief IO_RW_DIRECT. */
#define MASK_CMD53			53			/*!< @brief IO_RW_EXTEND. */

#define R5_COM_CRC_ERROR	(1 << 15)
#define R5_ILLEGAL_COMMAND	(1 << 14)
#define R5_ERROR			(1 << 11)
#define R5_FUNCTION_NUMBER	(1 << 9)
#define R5_OUT_OF_RANGE		(1 << 8)
#define R5_STATUS(x)		(x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	((x & 0x3000) >> 12)

/*
 * Card Common Control Registers (CCCR)
 */

#define SDIO_CCCR_CCCR		0x00

#define  SDIO_CCCR_REV_1_00	0	/* CCCR/FBR Version 1.00 */
#define  SDIO_CCCR_REV_1_10	1	/* CCCR/FBR Version 1.10 */
#define  SDIO_CCCR_REV_1_20	2	/* CCCR/FBR Version 1.20 */

#define  SDIO_SDIO_REV_1_00	0	/* SDIO Spec Version 1.00 */
#define  SDIO_SDIO_REV_1_10	1	/* SDIO Spec Version 1.10 */
#define  SDIO_SDIO_REV_1_20	2	/* SDIO Spec Version 1.20 */
#define  SDIO_SDIO_REV_2_00	3	/* SDIO Spec Version 2.00 */

#define SDIO_CCCR_SD		0x01

#define  SDIO_SD_REV_1_01	0	/* SD Physical Spec Version 1.01 */
#define  SDIO_SD_REV_1_10	1	/* SD Physical Spec Version 1.10 */
#define  SDIO_SD_REV_2_00	2	/* SD Physical Spec Version 2.00 */

#define SDIO_CCCR_IOEx		0x02
#define SDIO_CCCR_IORx		0x03

#define SDIO_CCCR_IENx		0x04	/* Function/Master Interrupt Enable */
#define SDIO_CCCR_INTx		0x05	/* Function Interrupt Pending */

#define SDIO_CCCR_ABORT		0x06	/* function abort/card reset */

#define SDIO_CCCR_IF		0x07	/* bus interface controls */

#define  SDIO_BUS_WIDTH_1BIT	0x00
#define  SDIO_BUS_WIDTH_4BIT	0x02

#define  SDIO_BUS_CD_DISABLE     0x80	/* disable pull-up on DAT3 (pin 1) */

#define SDIO_CCCR_CAPS		0x08

#define  SDIO_CCCR_CAP_SDC	0x01		/* can do CMD52 while data transfer */
#define  SDIO_CCCR_CAP_SMB	0x02		/* can do multi-block xfers (CMD53) */
#define  SDIO_CCCR_CAP_SRW	0x04		/* supports read-wait protocol */
#define  SDIO_CCCR_CAP_SBS	0x08		/* supports suspend/resume */
#define  SDIO_CCCR_CAP_S4MI	0x10		/* interrupt during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_E4MI	0x20		/* enable ints during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_LSC	0x40		/* low speed card */
#define  SDIO_CCCR_CAP_4BLS	0x80		/* 4 bit low speed card */

#define SDIO_CCCR_CIS		0x09		/* common CIS pointer (3 bytes) */

/* Following 4 regs are valid only if SBS is set */
#define SDIO_CCCR_SUSPEND	0x0c
#define SDIO_CCCR_SELx		0x0d
#define SDIO_CCCR_EXECx		0x0e
#define SDIO_CCCR_READYx	0x0f

#define SDIO_CCCR_BLKSIZE	0x10

#define SDIO_CCCR_POWER		0x12

#define  SDIO_POWER_SMPC	0x01		/* Supports Master Power Control */
#define  SDIO_POWER_EMPC	0x02		/* Enable Master Power Control */

#define SDIO_CCCR_SPEED		0x13

#define  SDIO_SPEED_SHS		0x01		/* Supports High-Speed mode */
#define  SDIO_SPEED_EHS		0x02		/* Enable High-Speed mode */

/*
 * Function Basic Registers (FBR)
 */

#define SDIO_FBR_BASE(f)	((f) * 0x100) /* base of function f's FBRs */

#define SDIO_FBR_STD_IF		0x00

#define  SDIO_FBR_SUPPORTS_CSA	0x40	/* supports Code Storage Area */
#define  SDIO_FBR_ENABLE_CSA	0x80	/* enable Code Storage Area */

#define SDIO_FBR_STD_IF_EXT	0x01

#define SDIO_FBR_POWER		0x02

#define  SDIO_FBR_POWER_SPS	0x01		/* Supports Power Selection */
#define  SDIO_FBR_POWER_EPS	0x02		/* Enable (low) Power Selection */

#define SDIO_FBR_CIS		0x09		/* CIS pointer (3 bytes) */


#define SDIO_FBR_CSA		0x0C		/* CSA pointer (3 bytes) */

#define SDIO_FBR_CSA_DATA	0x0F

#define SDIO_FBR_BLKSIZE	0x10		/* block size (2 bytes) */


/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/

/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct sdio_func_tuple {
	struct sdio_func_tuple *next;
	INT8U code;
	INT8U size;
	INT8U data[1];
};

struct gpSDIOInfo_s;

/*
 * SDIO function devices
 */
struct sdio_func {
	struct gpSDIOInfo_s	*sdio;		/* the card this device belongs to */
	INT32U		num;			/* function number */
	INT8U		class;			/* standard interface class */
	INT16U		vendor;			/* vendor id */
	INT16U		device;			/* device id */
	INT32U		max_blksize;	/* maximum block size */
	INT32U		cur_blksize;	/* current block size */
	INT32U		enable_timeout;	/* max enable timeout in msec */
	INT32U		state;			/* function state */
	INT8U		tmpbuf[4];		/* DMA:able scratch buffer */
	INT32U		num_info;		/* number of info strings */
	const char	**info;			/* info strings */
	struct sdio_func_tuple *tuples;
};

typedef struct sdio_cccr_s {
	INT32U 	sdio_vsn;
	INT32U 	sd_vsn;
	INT32U 	multi_block:1,
			low_speed:1,
			wide_bus:1,
			high_power:1,
			high_speed:1,
			disable_cd:1;
}sdio_cccr_t;

typedef struct sdio_cis_s {
	INT16U		vendor;
	INT16U		device;
	INT16U		blksize;
	INT32U		max_dtr;
}sdio_cis_t;

typedef struct gpSDIOInfo_s{
	INT32U			device_id;			/*!< @brief Index of SD controller. */
	INT32U			RCA;				/*!< @brief SD relative card address. */
	INT32U			present;			/*!< @brief Card present (initial or not).*/
	sdio_cccr_t		cccr;				/*!< @brief common card info.*/
	sdio_cis_t		cis;				/*!< @brief common tuple info.*/
	INT32U			num_info;			/*!< @brief number of info strings.*/
	const char		**info;				/*!< @brief info strings.*/
	struct sdio_func_tuple	*tuples;	/*!< @brief unknown common tuples.*/
} gpSDIOInfo_t;

/**************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S                *
**************************************************************************/

/**
* @brief 	Get SDIO information(handle).
* @param 	device_id[in]:  Index of SD controller.
* @return 	 SDIO information.
*/
extern gpSDIOInfo_t* drvl2_sdio_get_info(
	INT32U device_id );

/**
* @brief 	Send SDIO IO_RX_DIRECT command(CMD52).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	in[in]: Register data when write operation.
* @param 	out[out]: Read back register data after read/write operation.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_rw_direct(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT8U in,
	INT8U* out);

/**
* @brief 	Send SDIO IO_RX_EXTENDED command(CMD53).
* @param 	sdio[in]: SDIO handle.
* @param 	write[in]: The direction of io operation. 0 for read, and 1 for write.
* @param 	fn[in]: Function number.
* @param 	addr[in]: Register address.
* @param 	incr_addr[in]: Increment address, 0 for R/W fixed address, 1 for R/W incrementing address.
* @param 	buf[in]: Buffer pointer ( must be 4 byte alignment).
* @param 	blocks[in]: Block number.
* @param 	blksz[in]: Block size.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_rw_extended(
	gpSDIOInfo_t *sdio,
	INT32U write,
	INT32U fn,
	INT32U addr,
	INT32U incr_addr,
	INT32U *buf,
	INT32U blocks,
	INT32U blksz);

/**
* @brief 	SDIO interrupt enable, enable will be cleared after interrupt occurred.
* @param 	sdio[in]: SDIO handle.
* @param 	en[in]: Enable(1) or disable(0).
* @return 	None.
*/
extern void drvl2_sdio_irq_en(
	gpSDIOInfo_t *sdio,
	INT8U en);

/**
* @brief 	Receive SDIO interrupt signal.
* @param 	sdio[in]: SDIO handle.
* @param 	wait[in]: Waiting for irq signal or not.
* @param 	timeout[in]: Timeout value in second.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_receive_irq(
	gpSDIOInfo_t *sdio,
	INT32U wait,
	INT16U timeout);

/**
* @brief 	Initial SDIO card.
* @param 	device_id[in]:  Index of SD controller.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_init(
	INT32U device_id );

extern INT32S drvl2_sdio_detect(
	INT32U device_id );
/**
* @brief 	Read common CIS tuple.
* @param 	sdio[in]: SDIO handle.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_read_common_cis(
	gpSDIOInfo_t *sdio);

/**
* @brief 	Free common CIS tuple.
* @param 	sdio[in]: SDIO handle.
* @return 	None.
*/
extern void drvl2_sdio_free_common_cis(
	gpSDIOInfo_t *sdio);

/**
* @brief 	Read function CIS tuple.
* @param 	func[in]:  SDIO fuction struct pointer.
* @return 	0: success, -1: fail.
*/
extern INT32S drvl2_sdio_read_func_cis(
	struct sdio_func *func);

/**
* @brief 	Free function CIS tuple.
* @param 	func[in]:  SDIO fuction struct pointer.
* @return 	None.
*/
extern void drvl2_sdio_free_func_cis(
	struct sdio_func *func);

extern void drvl2_sdio_register_isr_handler(INT32U device_id, void* isr);

#endif		// __DRV_L2_SD_H__
