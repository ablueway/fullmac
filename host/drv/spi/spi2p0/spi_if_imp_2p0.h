/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _SPI_IF_IMP_2P0_H_
#define _SPI_IF_IMP_2P0_H_


/******************************************************************************/
/* Macro and type defines */
/******************************************************************************/
#define CHECK_SPI_MUTEX

#define SPI_DATA_WAKEUP             (0xBE)
#define SPI_WRITE_LOC_REG_CMD_F1    (0xB6)
#define SPI_WRITE_LOC_REG_CMD_F0    (0xB4)
#define SPI_READ_LOC_REG_CMD_F1     (0xB2)
#define SPI_READ_LOC_REG_CMD_F0     (0xB0)
#define SPI_WRITE_REG_CMD           (0xAA)
#define SPI_READ_REG_ADDR_CMD       (0xA8)
#define SPI_READ_REG_DATA_CMD       (0xA9)
#define SPI_READ_STS_CMD            (0xAC)
#define SPI_WRITE_STS_CMD           (0xAE)
#define SPI_TX_DATA_CMD             (0xA6)
#define SPI_RX_DATA_CMD             (0xA4)
#define SPI_DUMMY_DATA              (0x00)
#define SPI_POSITIVE_EDGE_CMD       (0x92)
#define SPI_NEGATIVE_EDGE_CMD       (0x90)

#define SPI_TX_BLOCK_SHIFT      (4)
#define SPI_TX_BLOCK_SIZE       (1 << SPI_TX_BLOCK_SHIFT)

#define M_SPI_FW_BLOCK_SIZE     (FW_BLOCK_SIZE)

/******************************************************************************/
/* Read staus */
/******************************************************************************/
#define READ_STATUS_BYTE0 0
#define READ_STATUS_BYTE1 1
#define READ_STATUS_BYTE2 2
#define READ_STATUS_BYTE3 3

#define READ_STATUS_BYTE0_RX_RDY (1<<0)
#define READ_STATUS_BYTE0_RX_FIFO_FAIL (1<<1)
#define READ_STATUS_BYTE0_RX_HOST_FAIL (1<<2)
#define READ_STATUS_BYTE0_TX_FIFO_FAIL (1<<3)
#define READ_STATUS_BYTE0_TX_HOST_FAIL (1<<4)
#define READ_STATUS_BYTE0_SPI_DOUBLE_ALLOC (1<<5)
#define READ_STATUS_BYTE0_SPI_TX_NO_ALLOC (1<<6)
#define READ_STATUS_BYTE0_RDATA_RDY (1<<7)

#define READ_STATUS_BYTE1_SPI_ALLOC_OK (1<<0)

#define READ_STATUS_RX_LEN_L READ_STATUS_BYTE2
#define READ_STATUS_RX_LEN_H READ_STATUS_BYTE3

/******************************************************************************/
/* Ctl Code */
/******************************************************************************/
enum SPI2P0_CTL_CODE{
    SPI2P0_LOCAL_REG_WRITE=0,
    SPI2P0_LOCAL_REG_READ,
    SPI2P0_STATUS_WRITE,
    SPI2P0_STATUS_READ,    
};

struct spi2p0_local_reg{
    u32 addr;
    u32 data;
};

#endif //_SPI_IF_IMP_2P0_H_
