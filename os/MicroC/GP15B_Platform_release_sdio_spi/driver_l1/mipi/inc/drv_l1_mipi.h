#ifndef __drv_l1_MIPI_H__
#define __drv_l1_MIPI_H__

/****************************************************
*		include file								*
****************************************************/
#include "driver_l1.h"

/****************************************************
*		constant 									*
****************************************************/
/* mipi clock sample edge */
#define D_PHY_SAMPLE_POS		0x0
#define D_PHY_SAMPLE_NEG		0x1

/* mipi ecc order*/
#define MIPI_ECC_ORDER0			0x0
#define MIPI_ECC_ORDER1			0x1
#define MIPI_ECC_ORDER2			0x2
#define MIPI_ECC_ORDER3			0x3

/* mipi check method */
#define MIPI_CHECK_LP_00		0x0
#define MIPI_CHECK_HS_SEQ		0x1

/* detect input format */
#define MIPI_AUTO_DETECT		0
#define MIPI_USER_DEFINE		1

/* mipi data format */
#define MIPI_YUV422				0x0
#define MIPI_RGB888				0x1
#define MIPI_YUV565				0x2
#define MIPI_RAW8				0x3
#define MIPI_RAW10				0x4
#define MIPI_RAW12				0x5
#define MIPI_GENERIC_8_BIT		0x6
#define MIPI_USER_DEFINE_BYTE	0x7

/* mipi data type */
#define MIPI_DATA_TO_CSI		0
#define MIPI_DATA_TO_CDSP		1

/* interrupt flag */
#define MIPI_LANE0_SOT_SYNC_ERR	0x01
#define MIPI_HD_1BIT_ERR 		0x02
#define MIPI_HD_NBIT_ERR		0x04
#define MIPI_DATA_CRC_ERR		0x08
#define MIPI_LANE1_SOT_SYNC_ERR	0x10
#define MIPI_CCIR_SOF 			0x20
#define MIPI_INT_ALL			0x3F

/****************************************************
*		data type 									*
*****************************************************/
typedef struct mipi_config_s 
{
	/* global configure */
	INT8U low_power_en;		/* 0:disable, 1:enable */
	INT8U byte_clk_edge;	/* 0:posedge, 1:negedge */
	INT8U reserved0;
	INT8U reserved1;
		
	/* format */
	INT8U data_from_mmr;	/* 0:auto detect, 1:user define */
	INT8U data_type;		/* MIPI_YUV422 ~ MIPI_USER_DEFINE_BYTE */
	INT8U data_type_to_cdsp;/* cdsp raw10, 1:data[7:0]+2':00 to cdsp, 0: 2'00+data[7:0] to csi */ 
	INT8U reserved2;
	
	/* ccir601 timing */
	INT16U h_size;			/* 0~0xFFFF */
	INT16U v_size;			/* 0~0xFFFF */
	INT8U h_back_porch;		/* 0~0xF */
	INT8U h_front_porch;	/* 0~0xF */
	INT8U blanking_line_en;	/* 0:disable, 1:enable */
	INT8U reserved3;
	
	/* ecc */
	INT8U ecc_check_en;		/* 0:disable, 1:enable */
	INT8U ecc_order;		/* MIPI_ECC_ORDER0 ~ MIPI_ECC_ORDER3 */
	INT8U data_mask_time;	/* unit is ns */
	INT8U check_hs_seq;		/* 0:disable, 1:enable */
} mipi_config_t;


/****************************************************
*		function
*****************************************************/
extern void drv_l1_mipi_init(INT8U enable);
extern INT32S drv_l1_mipi_isr_register(void (*user_isr_callback)(INT32U event));
extern void drv_l1_mipi_isr_unregister(void);
extern INT32S drv_l1_mipi_set_irq_enable(INT32U enable, INT32U bit);
extern void drv_l1_mipi_set_global_cfg(INT8U low_power_en, INT8U byte_clk_edge);
extern void drv_l1_mipi_set_ecc(INT8U ecc_order, INT8U ecc_check_en);
extern void drv_l1_mipi_set_mask_cnt(INT8U da_mask_cnt, INT8U check_hs_seq);
extern void drv_l1_mipi_set_ccir601_if(INT8U h_back_porch, INT8U h_front_porch, INT8U blanking_line_en);
extern void drv_l1_mipi_set_image_size(INT16U h_size, INT16U v_size);
extern void drv_l1_mipi_set_data_fmt(INT8U data_from_mmr, INT8U data_type_mmr, INT8U data_type_cdsp);
extern INT32S drv_l1_mipi_set_parameter(mipi_config_t *argp);
#endif