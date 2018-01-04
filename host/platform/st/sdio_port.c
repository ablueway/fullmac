//#include "config.h"
//#include <log.h>
//#include <stdio.h>
#include <errno.h>
#include <string.h>
#if 1

#include "sdio_port.h"
#include "sdio_def.h"
#include "sdio_drv.h"

#define SDIO_ERROR printf
#define SDIO_TRACE 

#define SDIO_DEFAULT_BLOCK_SIZE  64 //256

static unsigned int sdio_block_size;
void (*sdio_int_isr)(void);
void sdio_trigger(int enable)
{   
    #if 0
    if (enable)
    {
 //       GpioSetRegBits(GPIO_A_DS,GPIOA12);
        GpioClrRegOneBit(GPIO_A_PU,GPIOA12);
        GpioClrRegOneBit(GPIO_A_PD,GPIOA12);
        GpioSetRegBits(GPIO_A_OUT, GPIOA12);
    }
    else
    {
 //       GpioSetRegBits(GPIO_A_DS,GPIOA12);
        GpioSetRegOneBit(GPIO_A_PU,GPIOA12);
        GpioSetRegOneBit(GPIO_A_PD,GPIOA12);
        GpioClrRegBits(GPIO_A_OUT,GPIOA12);
    }
    #endif
}

#if 0
void GPIO_Configuration(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIOF Periph clock enable */ 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//注意要打开SYSCFG时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 
    GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_100M;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

}
void EXTI_Configuration(void) 
{ 
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_Configuration();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
    
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0); 
    EXTI_InitStructure.EXTI_Line = EXTI_Line0; 
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; 
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE; 
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; 
//    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream3_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure); 

}
#endif

#if 0
void EXTIX_Init(void)
{
    RCC->AHB1ENR|=1<<0;   // Enable PORTA Clock
    GPIO_Set(GPIOA,PIN0,GPIO_MODE_IN,0,0,GPIO_PUPD_PD);
    Ex_NVIC_Config(GPIO_A,0,RTIR); 	 	// Rising trigger
    MY_NVIC_Init(0,2,EXTI0_IRQn,2);	
}
void EXTI_Configuration()
{

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream3_IRQn; 
//    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure); 
    EXTIX_Init();
}

#endif
bool sdio_isr_enable(bool enable)
{
//    printf("sdio_isr_enable %d \r\n", enable);
    
#if 1
    if(enable)
    {
        SDIO_ITConfig(SDIO_IT_SDIOIT,ENABLE);
    }
    else
    {
        SDIO_ClearITPendingBit(SDIO_IT_SDIOIT);
        SDIO_ITConfig(SDIO_IT_SDIOIT,DISABLE);
    }
//    printf("sdio_isr_enable : SDIO->STA %08x SDIO->MASK %08x SDIO->DCTRL %08x\r\n", SDIO->STA, SDIO->MASK,SDIO->DCTRL);
#endif
    #if 0
    if (enable)
    {
        EXTI->IMR |= 1;
    }
    else
    {
 //       EXTI_ClearITPendingBit(EXTI_Line0);
        EXTI->IMR &= 0xFFFFFFFE;
    }
    #endif
}


bool sdio_host_enable_isr(bool enable)
{  
    sdio_isr_enable (enable);
    return true;
}

bool sdio_set_block_size(unsigned int blksize)
{

    unsigned char blk[2];
    unsigned int resp;
    int ret;
    
    if (blksize == 0)
    {
        blksize = SDIO_DEFAULT_BLOCK_SIZE;
    }
    
    blk[0] = (blksize >> 0) & 0xff;
    blk[1] = (blksize >> 8) & 0xff;
    
	ret = SDIO_Write_Byte(0, (int)0x110, blk[0]);
    if (!ret)
    {
        ret = SDIO_Write_Byte(0, (int)0x111, blk[1]);
    }

    if (!ret)
    {
        sdio_block_size = blksize;
    }
    else
    {
        SDIO_ERROR("sdio_set_block_size(%x), ret = %d\r\n", blksize, ret);
        return false;
    }
    return true;
}

unsigned int sdio_get_block_size()
{
    return sdio_block_size;
}
bool is_truly_isr()
{
    return TRUE;
}

bool sdio_host_detect_card(void)
{
    return TRUE;
}

bool sdio_host_init(void (*sdio_isr)(void))
{
    int ret = 0;
    
    SD_Init();
    
    sdio_int_isr = sdio_isr;

  //  EXTI_Configuration();
    sdio_set_block_size(64);

    ret = SDIO_Write_Byte(0, 0x4, 0x3);
    
    if (0 != ret)
    {
        SDIO_ERROR("mmc_io_rw_direct_host ret = %d\r\n", ret);
        return false;
    }
    
    sdio_host_enable_isr(false);

    // output timing
    if (!sdio_writeb_cmd52(REG_OUTPUT_TIMING_REG, SDIO_DEF_OUTPUT_TIMING))
        SDIO_FAIL_RET(0, "sdio_write_byte(0x55, %d)\n", SDIO_DEF_OUTPUT_TIMING);
    LOG_PRINTF("output timing to %d (0x%08x)\n", SDIO_DEF_OUTPUT_TIMING, SDIO_DEF_OUTPUT_TIMING);

	SDIO_TRACE("%-20s : %s\n", "host_int", "ok");
  
    return true;
}

u8	sdio_readb_cmd52(u32 addr)
{
    u8 data;
    int ret;
    ret = SDIO_Read_Byte(1,addr,&data);
    if (ret != 0)
        printf("_sdio_drv_creg_read ret = %d\r\n", ret);
    
//	printf("%-20s : 0x%08x, 0x%02x\n", "READ_BYTE", addr, data);
    
    return data;
}

bool sdio_writeb_cmd52(u32 addr, u8 data)
{
    int ret;
    ret = SDIO_Write_Byte(1, addr,data);
    if (ret != 0)
    {
        printf("sdio_drv_creg_write ret = %d\r\n", ret);
        return 0;
    }
//	printf("%-20s : 0x%08x, 0x%02x\n", "WRITE_BYTE", addr, data);
    
	return true;
}

bool sdio_read_cmd53(u32 dataPort, u8 *dat, size_t size)
{
    u32 rx_blocks = 0, blksize = 0;
    int ret = 0;
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#if 1
    if (size > 64) {
        rx_blocks = (size + 64 - 1) / 64;
        blksize = 64;
        size = 64 * rx_blocks;
    } else {
        blksize = size;
        rx_blocks = 1;
    }
#endif
#if 0
    if (size > sdio_block_size) {
         rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
         blksize = sdio_block_size;
         size = sdio_block_size * rx_blocks;
     } else {
         blksize = size;
         rx_blocks = 1;
     }
#endif
    ret = SDIO_Read_Multi_Byte(1, dataPort, size, 0, dat);
    if (0 != ret)
    {
        SDIO_ERROR("sdio_drv_read error ret = % d \r\n", ret);
        return false;
    }
    
    SDIO_TRACE("%-20s : 0x%x, 0x%08x, 0x%02x\n", "READ_BLOCK", dataPort, dat, size);
    return true;    
}
bool sdio_write_cmd53(u32 dataPort, void *dat, size_t size)
{
  	unsigned int tx_blocks = 0, buflen = 0;
    int ret = 0;
    if (((unsigned int)dat & 3) || (size & 3))
    {
        SDIO_ERROR("data and len must be align 4 byte. data = 0x%08x,  size = %d\r\n", (unsigned int)dat, size);
        return false;
    }
#if 1
    if (size > 64) 
    {
        tx_blocks = (size + 64 - 1) / 64;
        buflen = 64;
        size = 64 * tx_blocks;
    } 
    else 
    {
        buflen = size;
        tx_blocks = 1;
    }
#endif
#if 0
    if (size > sdio_block_size) {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        buflen = sdio_block_size;
        size = sdio_block_size * tx_blocks;
    } else {
        buflen = size;
        tx_blocks = 1;
    }
#endif

    ret = SDIO_Write_Multi_Byte(1, dataPort, size, 0, dat);
    if (0 != ret)
    {        
            SDIO_ERROR("sdio_drv_write error \r\n");
            return false;
    }

    SDIO_TRACE("%-20s : 0x%x, 0x%p, 0x%02x\n", "WRITE_BLOCK", dataPort, dat, size);

    return true;
}
#endif

