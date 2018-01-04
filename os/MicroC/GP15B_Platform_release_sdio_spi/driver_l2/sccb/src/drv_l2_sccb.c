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
#include "drv_l1_gpio.h"
#include "drv_l2_sccb.h"
#include "driver_l2.h"
#include "gplib_mm_gplus.h"

#if (defined _DRV_L2_SCCB) && (_DRV_L2_SCCB == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#if (BOARD_TYPE == BOARD_DVP_GPB51PG)
#define SCCB_SCL     	IO_D4
#define SCCB_SDA		IO_D5
#define CSI_PWDN		IO_D12
#elif (BOARD_TYPE == BOARD_EMU_GPB51PG)
#if 1
	#define SCCB_SCL    	IO_B4
	#define SCCB_SDA		IO_B5
#else
	#define SCCB_SCL    	IO_A11
	#define SCCB_SDA		IO_A10
#endif
#define CSI_PWDN		IO_D12
#endif

#define SCCB_SCL_DRV	IOD_DRV_4mA
#define SCCB_SDA_DRV	IOD_DRV_4mA
#define SCCB_TIMEOUT	0x20000

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct sccb_tran_s
{
	INT8U id; 
	INT8U addr_bits;
	INT8U data_bits;
	INT8U ack;
	INT16U addr;
	INT16U data;
} sccb_tran_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if _OPERATING_SYSTEM == _OS_UCOS2
	OS_EVENT *Sccb_Sema;
#endif


static void sccb_delay(INT32U i)
{
	INT32U j;

	for(j=0; j<(i<<4); j++) {
		i = i;
	}
}

static void sccb_lock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT8U err;
	
	OSSemPend(Sccb_Sema, 0, &err);
	OSSchedLock();
#endif
}

static void sccb_unlock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2	
	OSSchedUnlock();
	OSSemPost(Sccb_Sema);
#endif
}

static void sccb_start(void)
{
	gpio_write_io(SCCB_SCL, DATA_HIGH);					//SCL
	gpio_write_io(SCCB_SDA, DATA_HIGH);					//SDA
	sccb_delay (1);
	gpio_write_io(SCCB_SDA, DATA_LOW);					//SDA
	sccb_delay (1);
	gpio_write_io(SCCB_SCL, DATA_LOW);					//SCL
	sccb_delay (1);
}

static void sccb_stop(void)
{
	gpio_write_io(SCCB_SCL, DATA_LOW);					//SCL
	gpio_write_io(SCCB_SDA, DATA_LOW);					//SDA
	sccb_delay (1);
	gpio_write_io(SCCB_SCL, DATA_HIGH);					//SCL
	sccb_delay (1);
	gpio_write_io(SCCB_SDA, DATA_HIGH);					//SDA
	sccb_delay (1);
}

static INT32S sccb_w_phase(INT16U value, INT8U ack)
{
	INT32U i;
	INT32S ret = STATUS_OK;

	for(i=0;i<8;i++){
		gpio_write_io(SCCB_SCL, DATA_LOW);					//SCL0
		if(value & 0x80) {
			gpio_write_io(SCCB_SDA, DATA_HIGH);				//SDA1
		} else {
			gpio_write_io(SCCB_SDA, DATA_LOW);				//SDA0
		}
		
		sccb_delay (1);
		gpio_write_io(SCCB_SCL, DATA_HIGH);					//SCL1
		sccb_delay(1);
		value <<= 1;
	}
	
	// The 9th bit transmission
	gpio_write_io(SCCB_SCL, DATA_LOW);						//SCL0
	gpio_init_io(SCCB_SDA, GPIO_INPUT);						//SDA is Hi-Z mode
	sccb_delay(1);
	gpio_write_io(SCCB_SCL, DATA_HIGH);						//SCL1
	
	// check ack = low
	if(ack) {
		for(i=0; i<SCCB_TIMEOUT; i++) {
			if(gpio_read_io(SCCB_SDA) == 0) {
				break;
			}
		}		
	}

	if(i == SCCB_TIMEOUT) {
		ret = STATUS_FAIL;
	}
	
	sccb_delay(1);
	gpio_write_io(SCCB_SCL, DATA_LOW);					//SCL0
	gpio_init_io(SCCB_SDA, GPIO_OUTPUT);					//SDA is Hi-Z mode
	return ret;
}

static INT16U sccb_r_phase(INT8U phase)
{
	INT16U i;
	INT16U data = 0x00;

	gpio_init_io(SCCB_SDA, GPIO_INPUT);						//SDA is Hi-Z mode
	for(i=0;i<8;i++) {
		gpio_write_io(SCCB_SCL, DATA_LOW);					//SCL0
		sccb_delay(1);
		gpio_write_io(SCCB_SCL, DATA_HIGH);					//SCL1
		data <<= 1;
		data |=( gpio_read_io(SCCB_SDA));
		sccb_delay(1);
	}
	
	// The 9th bit transmission
	gpio_write_io(SCCB_SCL, DATA_LOW);						//SCL0
	gpio_init_io(SCCB_SDA, GPIO_OUTPUT);						//SDA is output mode
	if(phase == 2) {
		gpio_write_io(SCCB_SDA, DATA_LOW);					//SDA0, the nineth bit is ACK must be 1
	} else {
		gpio_write_io(SCCB_SDA, DATA_HIGH);					//SDA0, the nineth bit is NAK must be 1
	}
	
	sccb_delay(1);
	gpio_write_io(SCCB_SCL, DATA_HIGH);						//SCL1
	sccb_delay(1);
	gpio_write_io(SCCB_SCL, DATA_LOW);						//SCL0
	return data;	
}

static INT32S sccb_write(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id; 
	INT8U addr_bits = pData->addr_bits; 
	INT8U data_bits = pData->data_bits; 
	INT8U ack = 1;
	INT16U addr = pData->addr;
	INT16U data = pData->data;
	INT32S ret;

	// 3-Phase write transmission
	// Transmission start 
	sccb_start();			
	
	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(id, 1);	
	if(ret < 0) {
		goto __exit;
	}	
	
	// Phase 2, Register address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}
		
	// Phase 3, Register Data
	while(data_bits >= 8) {
		data_bits -= 8;
		if(data_bits) {
			ack = 1; //ACK
		} else {
			ack = 0; //NACK
		}
		
		temp = data >> data_bits;
		ret = sccb_w_phase(temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}
	
__exit:
	// Transmission stop	
	sccb_stop();	
	return ret;		
}

static INT32S sccb_read(sccb_tran_t *pData)
{
	INT8U temp;
	INT8U id = pData->id; 
	INT8U addr_bits = pData->addr_bits; 
	INT8U data_bits = pData->data_bits; 
	INT8U ack = 1;
	INT16U addr = pData->addr;
	INT32U read_data;
	INT32S ret;
	
	// 2-Phase write transmission cycle
	// Transmission start
	sccb_start();	
	
	// Phase 1, SLAVE_ID
	ret = sccb_w_phase(id, 1);		
	if(ret < 0) {
		goto __exit;
	}
	
	// Phase 2, Register Address
	while(addr_bits >= 8) {
		addr_bits -= 8;
		temp = addr >> addr_bits;
		ret = sccb_w_phase(temp, ack);
		if(ret < 0) {
			goto __exit;
		}
	}
	
	// Transmission stop
	sccb_stop();	

	// 2-Phase read transmission cycle
	// Transmission start
	sccb_start();	
	
	// Phase 1 (read)
	ret = sccb_w_phase(id | 0x01, 1);	
	if(ret < 0) {
		goto __exit;
	}
	
	// Phase 2 (read)
	read_data = 0;
	while(data_bits >= 8) {
		data_bits -= 8;
		if(data_bits) {
			ack = 1; //ACK
		} else {	
			ack = 0; //NACK
		}
		
		temp = sccb_r_phase(ack);
		read_data <<= 8;
		read_data |= temp;
	}
	pData->data = read_data;
	
__exit:
	// Transmission stop
	sccb_stop();	
	return ret;
}

/**
 * @brief   open a sccb request
 * @param   ID[in]: slave address
 * @param   RegBits[in]: register bit number
 * @param   DataBits[in]: data bit number
 * @return 	sccb handle
 */
void *drv_l2_sccb_open(INT8U ID, INT8U RegBits, INT8U DataBits)
{
	sccb_tran_t *pSccb;

#if _OPERATING_SYSTEM == _OS_UCOS2	
	if(Sccb_Sema == 0) {
		Sccb_Sema = OSSemCreate(1);
	}
#endif	
	
	pSccb = (sccb_tran_t *)gp_malloc_align(sizeof(sccb_tran_t), 4);
	if(pSccb == 0) {
		return 0;
	}
	
	pSccb->id = ID; 
	pSccb->addr_bits = RegBits >> 3 << 3;
	pSccb->data_bits = DataBits >> 3 << 3;
	pSccb->ack = 1;
	
	//init IO
	gpio_set_port_attribute(SCCB_SCL, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(SCCB_SDA, ATTRIBUTE_HIGH);
	gpio_init_io(SCCB_SCL, GPIO_OUTPUT);				//set dir
	gpio_init_io(SCCB_SDA, GPIO_OUTPUT);				//set dir
	gpio_write_io(SCCB_SCL, DATA_HIGH);					//SCL1
	gpio_write_io(SCCB_SDA, DATA_HIGH);					//SDA0
	
	gpio_set_port_attribute(CSI_PWDN, ATTRIBUTE_HIGH);
	gpio_init_io(CSI_PWDN, GPIO_OUTPUT);				//set dir
	gpio_write_io(CSI_PWDN, DATA_LOW);					//SDA0
	

	gpio_drving_init_io(SCCB_SCL, SCCB_SCL_DRV);
	gpio_drving_init_io(SCCB_SDA, SCCB_SDA_DRV);
	
	// wait io stable
	sccb_delay(200);
	
	return ((void *)pSccb);
}

/**
 * @brief   close a sccb request
 * @param   handle[in]: sccb handle
 * @return 	none
 */
void drv_l2_sccb_close(void *handle)
{
	if(handle) {
		gp_free((void *)handle);
		handle = 0;
	}
}

/**
 * @brief   sccb write data function
 * @param   addr[in] : register address
 * @param   data[in] : write data	
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_write(void *handle, INT16U addr, INT16U data) 
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;
	
	if(handle == 0) {
		return STATUS_FAIL;
	}
	
	sccb_lock();
	pSccb->addr = addr;
	pSccb->data = data;
	ret = sccb_write(pSccb);
	sccb_unlock();
	return ret;
}

/**
 * @brief   sccb read data function
 * @param   addr[in] : register address
 * @param   data[out] : read data	
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_sccb_read(void *handle, INT16U addr, INT16U *data) 
{
	INT32S ret;
	sccb_tran_t *pSccb = (sccb_tran_t *)handle;
	
	if(handle == 0) {
		return STATUS_FAIL;
	}
	
	sccb_lock();
	pSccb->addr = addr;
	ret = sccb_read(pSccb);
	if(ret >= 0) {
		*data = pSccb->data;
	} else {
		*data = 0xFF;
	}
	
	sccb_unlock();
	return ret;
}

#endif //(defined _DRV_L2_SCCB) && (_DRV_L2_SCCB == 1)
