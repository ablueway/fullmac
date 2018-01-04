/*
* MV platform RAM only 128K, so need to reduce code size.
* Modify as follows:
* 1. lwip use resource 0 . 
* 	 #define LWIP_PARAM_SET       0           
* 2. Iperf runninig server or client independent by compiling option.
* 	 #define HAVE_CLIENT          0
* 	 #define HAVE_SERVER          1
* 3. Disable CLI_HISTORY_ENABLE
* 	 #define CLI_HISTORY_ENABLE   0
* 4. Set SSV_TMR_MAX  10
*    #define SSV_TMR_MAX	      10
* 5. Create net_app_task only one.
* 6. Do not compiling SmartConfig module. 
*/

//#include <ssv_lib.h>
//#include <config.h>
//#include <log.h>
//#include <host_global.h>
//#include <rtos.h>
#include "malloc.h"
#include "sys.h"

typedef unsigned char   		bool;
#ifndef NULL
#define NULL                        (void *)0
#endif

#ifndef true
#define true                        1
#endif

#ifndef false
#define false                       0
#endif

#ifndef TRUE
#define TRUE                        1
#endif

#ifndef FALSE
#define FALSE                       0
#endif

#define OS_APIs
#define SSV_LDO_EN_PIN   1

extern bool sdio_if_load_fw(u8* fw_bin, u32 fw_bin_len);
#if 1
OS_APIs void *OS_MemAlloc( u32 size )
{
    
    void * ptr = NULL;

    ptr = (void *)mymalloc(0,size);   
    
    return ptr;
}

OS_APIs void __OS_MemFree( void *m )
{
    myfree(0,m);
}
#endif
void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    mymemcpy(pdest,psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    mymemset(pdest, byte, size); 
}

 

void platform_ldo_en_pin_init(void)
{

    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
#if 0    
#ifdef SSV_LDO_EN_PIN
       RCC->AHB1ENR|=1<<2;         //Ê¹ÄÜPORTCÊ±ÖÓ
//       RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
       GPIO_Set(GPIOC,1<<2,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_2M,GPIO_PUPD_NONE);    
#endif
#endif
}

void platform_ldo_en(bool en)
{
   if(en == 1)    
       GPIO_SetBits(GPIOC,GPIO_Pin_2);
   else 
      GPIO_ResetBits(GPIOC,GPIO_Pin_2);
       
#if 0    
#ifdef SSV_LDO_EN_PIN
       GPIO_Set(GPIOC,1<<2,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_2M,(en == 0) ? GPIO_PUPD_PD: GPIO_PUPD_PU);    
#endif
#endif
}
//=====================find fw to download=======================
#if 0
#include <firmware/ssv6200_uart_bin.h>
void platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin)); 
    ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
#else
bool platform_download_firmware(void)
{
    unsigned int fw_size, i;
 
   extern const unsigned char RES_WIFI_FW_START[];
   extern const unsigned char RES_WIFI_END[];
   unsigned char* fw = (unsigned char*)&RES_WIFI_FW_START;
   fw_size = ((unsigned int)&RES_WIFI_END) - ((unsigned int)&RES_WIFI_FW_START);
   printf("fw_size=%d,%x, fw=%x,%x\r\n",fw_size,*fw ,(unsigned int)fw,(unsigned int)&RES_WIFI_FW_START);
#if 0
   for(i=0; i<100; i++)
   {
       printf(" %02x ", *((u8 *)&RES_WIFI_FW_START-1+i));
       if (((i+1)%16)==0)
          printf("\r\n");
   }
   
   printf("\r\n");
   printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& \r\n");
   
   for(i=(fw_size-100); i<fw_size ; i++)
   {
       printf(" %02x ", *((u8 *)&RES_WIFI_FW_START-1+ i));
       if (((i+1)%16)==0)
          printf("\r\n");
   }
   
   printf("\r\n");
#endif   
   ssv6xxx_download_fw((u8 *)&RES_WIFI_FW_START-1,fw_size);//??? u8* bin
    return TRUE;

#endif
}
void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}


