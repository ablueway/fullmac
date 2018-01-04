#include "application.h"
#include "board_config.h"
#include "drv_l1_uart.h"
#include "console.h"
#include "usbd.h"
#include "MultiMedia_Platform.h"

#define TaskStackSize 		1024
#define PRINT_OUTPUT_NONE	0x00 
#define PRINT_OUTPUT_UART	0x01
#define PRINT_OUTPUT_USB	0x02

//define the demo code
#define	USB_DEMO				0
#define NAND_DEMO				0
#define AUDDEC_DEMO				0
#define AUDENC_DEMO				0
#define VIDDEC_DEMO				0
#define VIDENC_DEMO				0
#define VIDENC_DEMO_2			0
#define DISPLAY_DEMO			0
#define IMADEC_DEMO				0
#define IMAENC_DEMO				0
#define	HDMI_DEMO				0
extern void set_print_output_type(INT32U type);
extern void Nand_Demo(void);
extern void audio_decode_simple_demo(void);
extern void audio_encode_simple_demo(void);
extern void video_decode_simple_demo(void);
extern void video_encode_simple_demo(void);
extern void display_demo(void);
extern void image_decode_simple_demo(void);
extern void image_encode_simple_demo(void);
extern void video_encode_wrap_path_simple_demo(void);
extern int wifi_demo(int argc, char *argv[]);

INT32U free_memory_start, free_memory_end;
INT32U MainTaskStack[TaskStackSize];
#if defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 1)
INT32U CmdTaskStack[TaskStackSize];
#endif


void Main_task_entry(void *para)
{
/* not in */
#if NAND_DEMO
	Nand_Demo();
#endif
/* not in */
#if USB_DEMO	/* USBD_MSDC_MODE = 1, USBD_UVC_MODE = 2, USBD_HID_KEYBOARD_MODE = 3, USBD_DETECT_MODE = 4 */
#if(USB_DEMO == USBD_MSDC_MODE)
	usbd_register_class(&usbd_msdc_ctl_blk);
#elif(USB_DEMO == USBD_UVC_MODE)
	usbd_register_class(&usbd_uvc_ctl_blk);
#elif(USB_DEMO == USBD_HID_KEYBOARD_MODE)
	usbd_register_class(&usbd_hid_keyboard_ctl_blk);		
#elif(USB_DEMO == USBD_DETECT_MODE)
	usbd_register_class(&usbd_det_ctl_blk);	
#endif	
	usbd_task_init();
#endif
/* not in */
#if AUDDEC_DEMO
	audio_decode_simple_demo();
#endif
/* not in */
#if AUDENC_DEMO
	audio_encode_simple_demo();
#endif
/* not in */
#if VIDDEC_DEMO
	video_decode_simple_demo();
#endif
/* not in */
#if VIDENC_DEMO
	video_encode_simple_demo();
#endif
/* not in */
#if VIDENC_DEMO_2
	video_encode_wrap_path_simple_demo();
#endif
/* not in */
#if DISPLAY_DEMO
	display_demo();
#endif
/* not in */
#if IMADEC_DEMO
	image_decode_simple_demo();
#endif
/* not in */
#if IMAENC_DEMO
	image_encode_simple_demo();
#endif
/* not in */
#if HDMI_DEMO
	hdmi_demo();
#endif

    wifi_demo(0,NULL);

/* in */
#if !defined(GPLIB_CONSOLE_EN) || (defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 0))
	while(1)
	{
		/* Sleep 100ms in main task loop */
		OSTimeDly(100);
	}
#endif
	while(1);
}

void Debug_UART_Port_Enable(void)
{
#if PRINT_UART_PORT == 0
	drv_l1_uart0_buad_rate_set(UART0_BAUD_RATE);
	drv_l1_uart0_tx_enable();
	drv_l1_uart0_rx_enable();
#elif PRINT_UART_PORT == 1
	drv_l1_uart1_buad_rate_set(UART0_BAUD_RATE);
	drv_l1_uart1_tx_enable();
	drv_l1_uart1_rx_enable();
#endif	
}

void Main(void *free_memory)
{
	// Initialize Operating System
	os_init();			
			
	//board config
	board_init();
	
	// Initialize drvier layer 1 modules
	drv_l1_init();
	
	// Initialize driver Layer 2 modules
	drv_l2_init();

	// Initiate Component Layer modules
	free_memory_start = ((INT32U) free_memory + 3) & (~0x3);	// Align to 4-bytes boundry
	free_memory_end = (INT32U) SDRAM_END_ADDR & ~(0x3);
	gplib_init(free_memory_start, free_memory_end);
		
	// Enable UART port for debug
	Debug_UART_Port_Enable();

	//Configure the output type of debug message, NONE, UART, USB or both
	set_print_output_type(PRINT_OUTPUT_UART | PRINT_OUTPUT_USB);
	
	// Initialize platform
	platform_entrance(free_memory);
	
	// Create a user-defined task
	OSTaskCreate(Main_task_entry, (void *) 0, &MainTaskStack[TaskStackSize - 1], MAIN_TASK_PRIORITY); 
	
#if defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 1)
	OSTaskCreate(Cmd_Task, (void *)0, &CmdTaskStack[TaskStackSize - 1], CONSOLE_TASK_PRIORITY);
#endif
	
	// Start running
	OSStart();
}
