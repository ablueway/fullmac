#include "application.h"
//#include "wifi_abstraction_layer.h"
//#include "state_wifi.h"
#include "drv_l1_gpio.h"
#include "driver_l2_cfg.h"
#include "drv_l2_sdio.h"

void SDIO_Demo(void)
{
#if (defined _DRV_L2_SDIO) && (_DRV_L2_SDIO == 1)
	gpSDIOInfo_t* sdio;
	INT32U i;
	
	if(drvl2_sdio_init(0)<0)
	{
		DBG_PRINT("SDIO init fail\r\n");
		return ;
	}
	
	sdio = drvl2_sdio_get_info(0);
	DBG_PRINT("SDIO information:\r\n");
	for( i=0 ; i<sdio->num_info ; i++ )
	{
		if( sdio->info[i]==NULL || sdio->info[i][0]=='\0' )
			continue;
		DBG_PRINT("%s\r\n", sdio->info[i]);
	}
	DBG_PRINT("VID: 0x%04x\r\n", sdio->cis.vendor);
	DBG_PRINT("DID: 0x%04x\r\n", sdio->cis.device);
	/* ----- Enable SDIO IRQ ----- */
	drvl2_sdio_irq_en( sdio, 1 ); 
	/* ----- Receive IRQ ----- */
	drvl2_sdio_receive_irq( sdio, 1, 1);
#endif
}

void ssv_sdio_demo(void)
{
#if 0
	//RAY: Turn on WIFI
	gpio_write_io(WIFI_LDO_EN, DATA_LOW);
	OSTimeDly(1);

	gpio_write_io(WIFI_LDO_EN, DATA_HIGH);
	OSTimeDly(1);
	
	wal_init(0, NULL);
	
	if (wal_ap_mode_enable() != WAL_RET_SUCCESS)
	{
		DBG_PRINT("wifi init fail!!\r\n");
	}		
	else
	{
		DBG_PRINT("wifi init okl!!\r\n");			
		if(Wifi_State_Get() == WIFI_STATE_FLAG_DISCONNECT)
		{
			if(Wifi_Connect() == 0)
			{
				Wifi_State_Set(WIFI_STATE_FLAG_CONNECT);		
			}
		}
	}
#endif
}

void sdio_demo_task(void* para1)
{
	DBG_PRINT("Enter sdio_demo_task\r\n");
	SDIO_Demo();
	//ssv_sdio_demo();
	
	while(1)
	{
		OSTimeDly(100);
	}
	
}

