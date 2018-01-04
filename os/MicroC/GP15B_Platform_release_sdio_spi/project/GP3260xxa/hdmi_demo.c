#include "application.h"
#include "drv_l1_hdmi.h"
#include "drv_l1_sfr.h"
#include "drv_l1_interrupt.h"
#include "720P_GP420.h"

//extern RES_HDMI_DATA_START;

#define TFT_1280x720_MODE		6
#define TFT_SIZE_1280X720		(7<<16)

#define	PPU_YUYV_TYPE3					(3<<20)
#define	PPU_YUYV_TYPE2					(2<<20)
#define	PPU_YUYV_TYPE1					(1<<20)
#define	PPU_YUYV_TYPE0					(0<<20)

#define	PPU_RGBG_TYPE3					(3<<20)
#define	PPU_RGBG_TYPE2					(2<<20)
#define	PPU_RGBG_TYPE1					(1<<20)
#define	PPU_RGBG_TYPE0					(0<<20)

#define PPU_LB							(1<<19)
#define	PPU_YUYV_MODE					(1<<10)
#define	PPU_RGBG_MODE			        (0<<10)

#define TFT_SIZE_800X480                (5<<16)
#define TFT_SIZE_720x480				(4<<16)
#define TFT_SIZE_640X480                (1<<16)
#define TFT_SIZE_320X240                (0<<16)

#define	PPU_YUYV_RGBG_FORMAT_MODE		(1<<8)
#define	PPU_RGB565_MODE			        (0<<8)

#define	PPU_FRAME_BASE_MODE			    (1<<7)
#define	PPU_VGA_NONINTL_MODE			(0<<5)

#define	PPU_VGA_MODE					(1<<4)
#define	PPU_QVGA_MODE					(0<<4)

static int gp420conv_stop_flag = 0;

extern const INT8U ALIGN32 g_pc720P_YUV422_PATTERN[];

static void tft2_isr(void)
{
	if (R_PPU_IRQ_STATUS&0x80000) {
		//R_PPU_IRQ_STATUS  =  0x80000;		// clear interrupt flag	
		(*((volatile INT32U *) 0xC01B0000)) |= 0x200;	// reset

		if(gp420conv_stop_flag) {
			gp420conv_stop_flag = 0;
			(*((volatile INT32U *) 0xC01B0000)) = 0;
		} else {
		//	buff_get = ap_display_hdmi_queue_get(display_hdmi_dma_queue);
		//	old_buff_addr = (*((volatile INT32U *) 0xC01B0008));
		//	if (buff_get) {
		//		(*((volatile INT32U *) 0xC01B0008)) = buff_get;
		//		R_TV_FBI_ADDR = buff_get;

				//DBG_PRINT(".");
		//		ap_display_hdmi_queue_put(display_hdmi_isr_queue, old_buff_addr);
		//	} else {
		//		(*((volatile INT32U *) 0xC01B0008)) = old_buff_addr;
		//		R_TV_FBI_ADDR = old_buff_addr;

				//DBG_PRINT("x");
		//	}

			(*((volatile INT32U *) 0xC01B0000)) |= 0x100;	// start
		}
	}
	R_PPU_IRQ_STATUS  =  R_PPU_IRQ_STATUS;		// clear interrupt flag	
}

void hdmi_demo()
{
	
	R_PPU_IRQ_STATUS = 0x00002000 + 0x00000800;
	R_PPU_IRQ_EN &= ~(0x00002000 | 0x00000800);	// close TFT and TV interrupt
//	R_PPU_IRQ_EN |= 0x80000;				// bit19: HDMI enable
	
#if 0//YUV422
    (*((volatile INT32U *) 0xC01B0000)) = 0x0;
    (*((volatile INT32U *) 0xC01B0008)) = 0;//(INT32U)g_pc720P_YUV422_PATTERN;
#else//YUV420
    (*((volatile INT32U *) 0xC01B0000)) |= 0x200;
    (*((volatile INT32U *) 0xC01B0000)) |= 0x1;
    (*((volatile INT32U *) 0xC01B0004)) = 1280/4 - 1;
    (*((volatile INT32U *) 0xC01B0000)) |= 0x100;
    (*((volatile INT32U *) 0xC01B0008)) = (INT32U)g_lpby720P_GP420;
#endif

	R_PPU_ENABLE = (TFT_SIZE_1280X720|PPU_FRAME_BASE_MODE|PPU_YUYV_RGBG_FORMAT_MODE|PPU_YUYV_MODE|PPU_YUYV_TYPE3|(1<<24));  // YUYV mode
	R_PPU_MISC |= (1<<15);
	R_FREE_SIZE = 0x050002D0;// bit[31:16] = width , bit[15:0] = height
#if 0//YUV422    
    R_TV_FBI_ADDR = (INT32U)g_pc720P_YUV422_PATTERN;
#else//YUV420
	R_TV_FBI_ADDR = (INT32U)g_lpby720P_GP420;
#endif    
//    (*((volatile INT32U *) 0xC01B0000)) &= 0x01;
//    (*((volatile INT32U *) 0xC01B0008)) = (INT32U)g_pc720P_YUV422_PATTERN+0x10240;

	drvl1_hdmi_init(TFT_1280x720_MODE, 44100);
//	drvl1_hdmi_audio_ctrl(1);
//	drvl1_hdmi_dac_mute(1);
	vic_irq_register(VIC_PPU, tft2_isr);
	vic_irq_enable(VIC_PPU);		

	R_PPU_IRQ_STATUS = 0x00080000;
	R_PPU_IRQ_EN |= 0x00080000;					// bit19: HDMI enable
}

