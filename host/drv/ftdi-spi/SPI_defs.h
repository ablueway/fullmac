/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef __SPI_DEF_H__
#define __SPI_DEF_H__

/* -------------- h/w register ------------------- */
#define BASE_SPI	(0)

#define CABRIO_SPI_SLAVE_ID     (0)

/* Note :
	For now, the reg of SDIO Host & Card Controller are the same.
If it changes in the future, you should define again.
*/
#define SPI_REG_INT_MASK            (BASE_SPI + 0x04)		// 0
#define SPI_REG_INT_STATUS          (BASE_SPI + 0x08)		// 0
#define SPI_REG_INT_TRIGGER         (BASE_SPI + 0x09)		// 0
#define SPI_REG_CARD_PKT_LEN        (BASE_SPI + 0x10)		// 0

#define SPI_REG_CARD_FW_DL_STATUS   (BASE_SPI + 0x12)		// 0
#define SPI_REG_CARD_SELF_TEST      (BASE_SPI + 0x13)		// 0
#define SPI_REG_CARD_RCA_0          (BASE_SPI + 0x20)		// 0
#define SPI_REG_CARD_RCA_1          (BASE_SPI + 0x21)		// 0
//SDIO TX ALLOCATE FUNCTION
#define SPI_REG_TX_ALLOC_SIZE       (BASE_SPI + 0x98)		// 0
#define SPI_REG_TX_ALLOC_SHIFT      (BASE_SPI + 0x99)		// 0
#define SPI_REG_TX_ALLOC_STATE      (BASE_SPI + 0x9a)		// 0

#define TX_ALLOC_SUCCESS            (0x01)
#define TX_NO_ALLOC                 (0x02)
#define TX_DULPICATE_ALLOC          (0x04)

#define SPI_TX_ALLOC_SIZE_SHIFT     (0x04)
#define SPI_TX_ALLOC_ENABLE		    (0x10)

/* -------------- debug        ------------------- */
#define	_SPI_DEBUG                  1
#define _SPI_TRACE                  0
#define _SPI_WARN                   1

#define SPI_DEBUG(fmt, ...)         do { if (_SPI_DEBUG) LOG_PRINTF("[DEBUG ] " fmt, ##__VA_ARGS__); } while (0)
#define SPI_TRACE(fmt, ...)         do { if (_SPI_TRACE) LOG_PRINTF("[TRACE ] " fmt, ##__VA_ARGS__); } while (0)
#define SPI_WARN(fmt, ...)          do { if (_SPI_WARN)  LOG_PRINTF("[WARN  ] " fmt, ##__VA_ARGS__); } while (0)
#define SPI_ERROR(fmt, ...)         do { LOG_PRINTF("[Error!] %s() : ", __FUNCTION__); LOG_PRINTF(fmt, ##__VA_ARGS__); } while (0)
#define SPI_FATAL(fmt, ...)         do { LOG_PRINTF("[Fatal!] %s (#%d) : ", __FILE__, __LINE__); LOG_PRINTF(fmt, ##__VA_ARGS__); LOG_PRINTF("program halt!!!\n"); exit(1); } while (0)
#define SPI_FAIL(fmt, ...)		    do { LOG_PRINTF("[Fail! ] %s() : ", __FUNCTION__); LOG_PRINTF(fmt, ##__VA_ARGS__); } while (0)
#define SPI_FAIL_RET(r, fmt, ...)   do { LOG_PRINTF("[Fail! ] %s() : ", __FUNCTION__); LOG_PRINTF(fmt, ##__VA_ARGS__); return (r); } while (0)
/* -------------- default      ------------------- */
//Option[50MHz 25MHz 12.5MHz 6.25MHz] 
#define SPI_DEF_CLOCK_RATE			12500		// 25MHz

#endif // __SPI_DEF_H__

