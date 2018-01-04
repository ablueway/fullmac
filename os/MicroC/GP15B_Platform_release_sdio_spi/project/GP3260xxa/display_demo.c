#include "application.h"
#include "drv_l1_uart.h"
#include "drv_l2_display.h"
#include "drv_l1_tv.h"
#include "drv_l1_tft.h"

#define DISPLAY_TV_EN			1
#define DISPLAY_TFT_EN			2
#define DISPLAY_DEMO_MODE		DISPLAY_TV_EN

void display_demo(void)
{
	INT32U i,j,frame_size,display_buffer;
	INT16U *fb,width, height;

	DBG_PRINT("display demo start\r\n");
	
	drv_l2_display_init();

#if DISPLAY_DEMO_MODE == DISPLAY_TV_EN
	drv_l2_display_start(DISDEV_TV_VGA,DISP_FMT_RGB565);
#endif

#if DISPLAY_DEMO_MODE == DISPLAY_TFT_EN
	drv_l2_display_start(DISDEV_TFT,DISP_FMT_RGB565);
#endif

#if DISPLAY_DEMO_MODE == DISPLAY_TV_EN
	drv_l2_display_get_size(DISDEV_TV_VGA,&width,&height);
	frame_size = (width*height*2);
	display_buffer = (INT32U) gp_malloc_align(frame_size, 32);
	fb = (UINT16 *)display_buffer;
	for (i = 0; i < height; i++) {
		 for (j = 0; j < width; j++) {
		    if(j/(width/5) == 0)
			     *fb++ = 0xf800;
		    if(j/(width/5) == 1)
			     *fb++ = 0x07e0;
		    if(j/(width/5) == 2)
			     *fb++ = 0x001f;
		    if(j/(width/5) == 3)
			     *fb++ = 0x0000;
		    if(j/(width/5) == 4)
			     *fb++ = 0xffff;					     					     						
		 }
	}
	drv_l2_display_update(DISDEV_TV_VGA,display_buffer);	
#endif

#if DISPLAY_DEMO_MODE == DISPLAY_TFT_EN
	drv_l2_display_get_size(DISDEV_TFT,&width,&height);
	frame_size = (width*height*2);
	display_buffer = (INT32U) gp_malloc_align(frame_size, 32);
	fb = (UINT16 *)display_buffer;
	for (i = 0; i < height; i++) {
		 for (j = 0; j < width; j++) {
		    if(j/(width/5) == 0)
			     *fb++ = 0xf800;
		    if(j/(width/5) == 1)
			     *fb++ = 0x07e0;
		    if(j/(width/5) == 2)
			     *fb++ = 0x001f;
		    if(j/(width/5) == 3)
			     *fb++ = 0x0000;
		    if(j/(width/5) == 4)
			     *fb++ = 0xffff;					     					     						
		 }
	}
	drv_l2_display_update(DISDEV_TFT,display_buffer);
#endif
	
	DBG_PRINT("display demo end\r\n");
	
	while(1)
		OSTimeDly(1);
}