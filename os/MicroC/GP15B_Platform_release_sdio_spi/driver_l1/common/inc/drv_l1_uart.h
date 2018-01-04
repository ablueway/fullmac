#ifndef __drv_l1_UART_H__
#define __drv_l1_UART_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

// UART
typedef enum
{
	WORD_LEN_5,
	WORD_LEN_6,
	WORD_LEN_7,
	WORD_LEN_8
} WORD_LEN;

typedef enum
{
	STOP_SIZE_1,
	STOP_SIZE_2
} STOP_BIT_SIZE;

typedef enum
{
	PARITY_ODD,
	PARITY_EVEN
} PARITY_SEL;

// UART0 extern API
#if (defined _DRV_L1_UART0) && (_DRV_L1_UART0 == 1) 
extern void drv_l1_uart0_init(void);
extern void drv_l1_uart0_buad_rate_set(INT32U bps);
extern void drv_l1_uart0_rx_enable(void);
extern void drv_l1_uart0_rx_disable(void);
extern void drv_l1_uart0_tx_enable(void);
extern void drv_l1_uart0_tx_disable(void);
extern void drv_l1_uart0_data_send(INT8U data, INT8U wait);
extern INT32S drv_l1_uart0_data_get(INT8U *data, INT8U wait);
extern INT32S drv_l1_uart0_word_len_set(INT8U word_len);
extern INT32S drv_l1_uart0_stop_bit_size_set(INT8U stop_size);
extern INT32S drv_l1_uart0_parity_chk_set(INT8U status, INT8U parity);
#endif

// UART1 extern API
#if (defined _DRV_L1_UART1) && (_DRV_L1_UART1 == 1) 
extern void drv_l1_uart1_init(void);
extern void drv_l1_uart1_buad_rate_set(INT32U bps);
extern void drv_l1_uart1_rx_enable(void);
extern void drv_l1_uart1_rx_disable(void);
extern void drv_l1_uart1_tx_enable(void);
extern void drv_l1_uart1_tx_disable(void);
extern void drv_l1_uart1_data_send(INT8U data, INT8U wait);
extern INT32S drv_l1_uart1_data_get(INT8U *data, INT8U wait);
extern INT32S drv_l1_uart1_word_len_set(INT8U word_len);
extern INT32S drv_l1_uart1_stop_bit_size_set(INT8U stop_size);
extern INT32S drv_l1_uart1_parity_chk_set(INT8U status, INT8U parity);
#endif

#endif		// __drv_l1_UART_H__