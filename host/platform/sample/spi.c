/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "msgevt.h"


#include "ssv_drv.h"
#include "ssv_drv_config.h"
#include "spi_def.h"
#include "spi.h"


#define SPI_1 	 1
#define SPI_2 	 2




#define SPI_INTERUPT_GPIO    GPIO0
#define SSV_CS_PIN           GPIO1

void (*gpio_spi_isr)(void);

static void _spi_host_cs_init(void);
static void _spi_host_irq_pin_init(void (*spi_isr)(void));
static void _spi_host_cs_high(void);
static void _spi_host_cs_low(void);
static void _spi_host_cs_init(void)
{

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

    gpio_init_io(SSV_CS_PIN,GPIO_OUTPUT);
    gpio_write_io(SSV_CS_PIN, 1); //Default high
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
}


static void _spi_host_irq_pin_init(void (*spi_isr)(void))
{

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);

    drv_int_init(SPI_INTERUPT_GPIO);
    drv_edge_set(RISING); /* rising trigger */
    drv_user_isr_set(spi_isr); /* register interrupt callback function */
    drv_enable_set(FALSE);

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
}

static void _spi_host_cs_high(void)
{
    gpio_cs_enable(SSV_CS_PIN, true);
}

static void _spi_host_cs_low(void)
{
    gpio_cs_enable(SSV_CS_PIN, false);
}

bool spi_is_truly_isr()
{
    return TRUE;
}

bool spi_host_init(void (*spi_isr)(void))
{

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    if(spi_isr == NULL)
    {
        SDRV_ERROR("%s(): spi_isr is a null pointer\r\n",__FUNCTION__);
        return FALSE;
    }
    

    _spi_host_cs_init();
    _spi_host_cs_high();
    _spi_host_irq_pin_init(spi_isr);

    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);

    atcspib200_reg_p spi;
	spi = (atcspib200_reg_p) CPE_SPI2_BASE;

	atcspib200_reset(spi, ATCSPIB200_SPI_RESET);
	atcspib200_if_set(spi,ATCSPIB200_ADDR_3BYTE, 7, ATCSPIB200_MERGE, ATCSPIB200_NORM_LINE, ATCSPIB200_MSB, ATCSPIB200_MASTER, ATCSPIB200_CPP_0);

	atcspib200_ctrl_set(spi, 2, 2, 0, 0);
	atcspib200_timing_set(spi, 1, 1, 0);  

	return TRUE;
}

bool spi_ht_write(UINT8 *buf, UINT32 sizeToTransfer)
{
	int i, j, k;
	atcspib200_reg_p dev_p;

	if (NULL == buf)
		return FALSE;
	j = (sizeToTransfer % 4);
    k = (sizeToTransfer / 4);
    
	dev_p =	(atcspib200_reg_p) CPE_SPI2_BASE; 
	
	if (atcspib200_reset(dev_p, ATCSPIB200_RXF_RESET | ATCSPIB200_TXF_RESET) == FAIL)
        return FALSE;

    atcspib200_intr_enable(dev_p, 0);
        
	if (sizeToTransfer <= 512)	
	{		
	    atcspib200_if_set(dev_p,ATCSPIB200_ADDR_3BYTE, 7, ATCSPIB200_MERGE, ATCSPIB200_NORM_LINE, ATCSPIB200_MSB, ATCSPIB200_MASTER, ATCSPIB200_CPP_0);

        atcspib200_data_ctrl(dev_p, 0, 0, ATCSPIB200_ADDR_SINGLE, ATCSPIB200_DATA_SINGLE, ATCSPIB200_WRonly, 0, 0, sizeToTransfer, 0, 0);
		if(sizeToTransfer > 3)
        { 
			for (i=0; (i<sizeToTransfer)&&((i+4)<=sizeToTransfer); i+=4)
			{
			    if (i == 0)
                {         
                    dev_p->RegDPort = (buf[i + 3]<< 24) | (buf[i + 2] << 16)| (buf[i + 1] << 8)|buf[i]; 
                    atcspib200_cmd(dev_p, 0);
                }
                else
                {
            		 while (ATCSPIB200_GET_FIELD(dev_p->St, ST, TXFEM) == 0);
                     dev_p->RegDPort = (buf[i + 3]<< 24) | (buf[i + 2] << 16)| (buf[i + 1] << 8)|buf[i];                    
                }
			}
        }
        
        if (j != 0)
		{   			    
			switch (j)
			{
				case 1:
					dev_p->RegDPort = buf[sizeToTransfer-1];
					break;
				case 2:
                    dev_p->RegDPort = (buf[sizeToTransfer - 1] << 8) | buf[sizeToTransfer-2];
					break;					
				case 3:
				    dev_p->RegDPort = (buf[sizeToTransfer - 1] << 16) | (buf[sizeToTransfer - 2] << 8) | buf[sizeToTransfer -3];
					break;						
				default:
					break;
			 }
		}
        
        if(sizeToTransfer < 4)
            atcspib200_cmd(dev_p, 0);
               
        if (atcspib200_polling_spibsy(dev_p) == FAIL)
        {
           printf("====512===========busy\r\n");
        }

	    return TRUE;
	}
	else	
    {    
	    atcspib200_if_set(dev_p,ATCSPIB200_ADDR_3BYTE, 31, ATCSPIB200_MERGE, ATCSPIB200_NORM_LINE, ATCSPIB200_MSB, ATCSPIB200_MASTER, ATCSPIB200_CPP_0);
        atcspib200_ctrl_set(dev_p, 2, 2, 0, 0);
        atcspib200_timing_set(dev_p, 1, 1, 0);  
        atcspib200_data_ctrl(dev_p, 0, 0, ATCSPIB200_ADDR_SINGLE, ATCSPIB200_DATA_SINGLE, ATCSPIB200_WRonly, 0, 0, k, 0, 0);

        for (j=0,i=0; j<k; j++,i+=4)
        {        
            if (j == 0)
            {         
                dev_p->RegDPort = (buf[i]<< 24) | (buf[i + 1] << 16)| (buf[i + 2] << 8)| buf[i+3]; 
                atcspib200_cmd(dev_p, 0);
            }
            else
            {
                 while (ATCSPIB200_GET_FIELD(dev_p->St, ST, TXFEM) == 0);
                 dev_p->RegDPort = (buf[i]<< 24) | (buf[i + 1] << 16)| (buf[i + 2] << 8)| buf[i+3];                     
       		}
	    }   
        if (atcspib200_polling_spibsy(dev_p) == FAIL)
        {
           printf("===============spi busy\r\n");
        }
        
        while (atcspib200_polling_spibsy(dev_p) == FAIL);
     }
}


bool spi_ht_read(UINT32 *rbuf, UINT32 sizeToTransfer)
{
	int i,j,k;
    int wait_cnt=0;
	atcspib200_reg_p spi;
    j = sizeToTransfer/4;
    spi = (atcspib200_reg_p) CPE_SPI2_BASE;
    
    if (sizeToTransfer <= 512)
    {
        k = sizeToTransfer;
        atcspib200_if_set(spi,ATCSPIB200_ADDR_3BYTE, 7, ATCSPIB200_MERGE, ATCSPIB200_NORM_LINE, ATCSPIB200_MSB, ATCSPIB200_MASTER, ATCSPIB200_CPP_0);
    }
    else 
    {
        k = (sizeToTransfer/4);
        atcspib200_if_set(spi,ATCSPIB200_ADDR_3BYTE, 31, ATCSPIB200_MERGE, ATCSPIB200_NORM_LINE, ATCSPIB200_MSB, ATCSPIB200_MASTER, ATCSPIB200_CPP_0);
    }
    
    // wait prior transfer finish
	while (ATCSPIB200_GET_FIELD(spi->St, ST, SPIBSY));
    
    atcspib200_data_ctrl(spi, 0, 0, ATCSPIB200_ADDR_SINGLE, ATCSPIB200_DATA_SINGLE, ATCSPIB200_RDonly, 0, 0, 0, 0, k);

	if (atcspib200_reset(spi, ATCSPIB200_RXF_RESET) == FAIL)
        return FALSE;
    
    atcspib200_cmd(spi, 0);
    
    for (i=0; i<j; i++)
    {
        wait_cnt=0;
        while((++wait_cnt < 50000)&&(ATCSPIB200_GET_FIELD(spi->St, ST, RXFVE) == 0));
        if (wait_cnt == 50000)
            return FALSE;
        else
        {
            if (sizeToTransfer<=512)
                *(rbuf++) = spi->RegDPort; 
            else
            {
                *rbuf = spi->RegDPort; 
                *rbuf =((*rbuf >> 24)&0XFF)| ((*rbuf >> 16)&0XFF)<<8 | ((*rbuf >> 8)&0XFF)<<16 | (*rbuf&0XFF) << 24 ;
                 rbuf++;
            }
        }
    }

	if (sizeToTransfer & 3)
		*(rbuf++) = spi->RegDPort; 
    
	return TRUE;
}

bool spi_host_readwrite(u8 *buf, u32 sizeToTransfer, u32 *sizeToTransfered, u8 options, bool IsRead)
{

    bool ret=TRUE;
    int i;
    
    i = (sizeToTransfer%4);
    
    if(options & SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE)
    {
        _spi_host_cs_low();

    }
    if(options & SPI_TRANSFER_OPTIONS_CPU_POLLING_MODE)
    {
        if(TRUE==IsRead){
            ret = spi_ht_read(buf, sizeToTransfer);
        }else{
            if(sizeToTransfer <= 2048){
               ret = spi_ht_write(buf, sizeToTransfer);
            }else{
            
               ret = spi_ht_write(buf, 2048);               
               ret = spi_ht_write(buf + 2048, sizeToTransfer-2048);
            }         
        }
    }
    else
    {
        if(TRUE==IsRead){
            ret = spi_ht_read(buf, sizeToTransfer);
        }else{
            if(sizeToTransfer <= 2048){
               ret = spi_ht_write(buf, sizeToTransfer);
            }else{            
               ret = spi_ht_write(buf, 2048);               
               ret = spi_ht_write(buf + 2048, sizeToTransfer-2048);
            }         
        }        
    }
    if(options & SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE)
    {
        _spi_host_cs_high();
     
    }
    return ret;
}

void spi_host_irq_enable(bool enable)
{
    gpio_isr_enable(SPI_INTERUPT_GPIO, enable);
}


