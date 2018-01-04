#ifndef __DRIVER_L2_CFG_H__
#define __DRIVER_L2_CFG_H__

	#define DRV_L2_ON			    	1
	#define DRV_L2_OFF			  		0

	#define _DRV_L2_SYSTEM_TIMER		DRV_L2_ON
	#define _DRV_L2_KEYSCAN	            DRV_L2_ON
	#define _DRV_L2_USBH                _DRV_L1_USBH
	#define _DRV_L2_USBH_UVC            _DRV_L1_USBH_UVC
	#define _DRV_L2_SPI                 DRV_L2_ON
	#define _DRV_L2_SDIO                DRV_L2_ON
	#define _DRV_L2_CDSP				DRV_L2_ON
	#define _DRV_L2_NAND	            DRV_L2_OFF
	#define _DRV_L2_DISP				DRV_L2_ON	
	#define _DRV_L2_SENSOR				DRV_L2_ON
	#define _DRV_L2_SCCB				DRV_L2_ON
	#define _DRV_L2_SCALER				DRV_L2_ON
	
	#define UVC_AUDIO_SUPPORT			1
	#define UAC_AUDIO_SAMPLE_RATE		22050		/* UAC sample rate */
#endif		// __DRIVER_L2_CFG_H__
