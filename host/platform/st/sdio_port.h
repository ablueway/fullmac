#ifndef _SDIO_H_
#define _SDIO_H_
typedef unsigned char	      u8;
typedef unsigned int          u32;
typedef unsigned char   		bool;

typedef	unsigned int			size_t;

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


//output timing
// 0: cmd  [0]:positive [1]:negative
// 1: data [0]:positive [1]:negative
#define SDIO_DEF_OUTPUT_TIMING 		0 // 3

u8	 sdio_readb_cmd52(u32 addr);
bool sdio_writeb_cmd52(u32 addr, u8 data);
bool sdio_write_cmd53(u32 dataPort,void *dat, size_t size);
bool sdio_read_cmd53(u32 dataPort,u8 *dat, size_t size);
bool is_truly_isr(void);
bool sdio_host_detect_card(void);

void GPIO_Configuration(void); 
void EXTI_Configuration(void); 
void NVIC_Config(void); 


#endif /* _SDIO_H_ */

