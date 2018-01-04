/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include "sdio.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <hctrl.h>
#include <ssv_hal.h>
#include <ssv_drv.h>



static int	s_hSDIO				= 0;		// where is ONLY one SDIO handle in this module!
static u16	s_cur_block_size	= SDIO_DEF_BLOCK_SIZE;
static OsMutex	s_SDIOMutex;
//static u32	s_write_data_cnt	= 1;
//static u8	s_func_focus	= -1;
//static bool s_sdio_data_mode = true; // Power on status is data mode.

#include "../../core/txrx_task.h"
static bool sim_end = false;
static bool sim_pause = true;
OsTaskHandle h_sim_rx_isr;
void sdio_sim_rx_isr(void *args)
{
    struct ssv6xxx_huw_cmd cmd;
    int status;
    u32 rxpktnum = 0;
    u32 sleep_time = 1;

    while(sim_end != true)
    {
        if (sim_pause != false)
        {
            //SDRV_INFO("[wifi-sim:sdio-hci]: delay due to sim_pause\n");
            OS_MsDelay(100);
            continue;
        }
        ssv6xxx_memset((void *)&cmd, 0, sizeof(cmd));
        rxpktnum = 0;
        cmd.out_data_len = sizeof(rxpktnum);
        cmd.out_data = (u8 *)&rxpktnum;
        status = ioctl(s_hSDIO, IOCTL_SSV6XXX_SDIO_RX_PKT_NUM, &cmd);

        if(rxpktnum != 0)
            TXRXTask_Isr(INT_RX,FALSE);

        //SDRV_INFO("[wifi-sim:sdio-hci]: delay due to send rx semaphore\n");
        OS_MsDelay(sleep_time);

    }
}
struct task_info_st g_sim_rx_isr_info[] =
{
	{ "sdio_simrxisr",   (OsMsgQ)0, 0, 	OS_RX_ISR_PRIO, RX_ISR_STACK_SIZE, NULL,  sdio_sim_rx_isr},
};

const char*	_err_str(u32 err);

bool	sdio_set_function_focus(int func);
bool	sdio_get_function_focus(u8 out_buf[1]);

const char* _err_str(u32 err)
{
    switch (err)
    {
        case 0: return "The operation completed successfully.";
        case 1: return "Incorrect function.";
        case 2: return "The system cannot find the file specified.";
        case 3: return "The system cannot find the path specified.";
		case 4: return "The system cannot open the file.";
		case 5: return "Access is denied.";
        case 6: return "The handle is invalid.";
        default: return "Other. Plz visit MSDN website.";
    }
}

u32	sdio_handle(void)
{
	return (u32)s_hSDIO;
}

#define	SDIO_CHECK_OPENED()		{ if (s_hSDIO == 0)	SDIO_FAIL_RET(0, "SDIO device is NOT opened!\n");	}



inline u16	sdio_get_cur_block_size(void)
{
	return s_cur_block_size;
}

u16		sdio_get_block_size(void)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return true;
}

// note : this func will automatically update 's_cur_block_size' value after success
bool	sdio_set_block_size(u16 size)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return true;
}

bool	sdio_set_function_focus(int func)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return true;
}

bool	sdio_get_function_focus(u8 out_buf[1])
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return true;
}

static bool _sdio_read_reg(u32 addr, u32 *data)
{
	struct ssv6xxx_huw_cmd cmd;
    u32 in_val, out_val;
	int	status;
	u32		r;

	SDIO_CHECK_OPENED();

	cmd.in_data_len = 4;
	cmd.out_data_len = 4;
    in_val = addr;
    cmd.in_data = (u8 *)&in_val;
	cmd.out_data = (u8 *)&out_val;

	//OS_MutexLock(s_SDIOMutex);
	status = ioctl(s_hSDIO, IOCTL_SSV6XXX_SDIO_READ_REG,&cmd);
	//OS_MutexUnLock(s_SDIOMutex);
	if (status < 0)
	{
		SDIO_ERROR("READ_REG, status = %d\n", status);
		return FALSE;
	}

    r = out_val;

	SDIO_TRACE("%-20s() : 0x%08x, 0x%08x\n", "READ_REG", addr, r);
    *data = r;
	return TRUE;
}


u32  sdio_read_reg(u32 addr)
{
//	struct ssv6xxx_huw_cmd cmd;
//    u32 in_val, out_val;
//	int	status;
	u32		r;
	if(_sdio_read_reg(addr,&r)==TRUE)

		SDIO_TRACE("%-20s() : 0x%08x, 0x%08x\n", "READ_REG", addr, r);

	return r;
}


bool	sdio_write_reg(u32 addr, u32 data)
{
	struct ssv6xxx_huw_cmd cmd;
    u32     in_val[2], out_val;

	int	status;

	SDIO_CHECK_OPENED();

    in_val[0] = addr;
    in_val[1] = data;
    cmd.in_data = (u8*)in_val;
	cmd.out_data = (u8*)&out_val;
    cmd.in_data_len = 8;
	cmd.out_data_len = 4;

	//OS_MutexLock(s_SDIOMutex);
	status = ioctl(s_hSDIO,IOCTL_SSV6XXX_SDIO_WRITE_REG,&cmd);
	//OS_MutexUnLock(s_SDIOMutex);
	if (status < 0 )
	{
		SDIO_ERROR("WRITE_REG\n");
		return false;
	}

	SDIO_TRACE("%-20s : 0x%08x, 0x%08x\n", "WRITE_REG", addr, data);
	return true;
}

void    sdio_set_data_mode(bool use_data_mode)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
}

//Only for SDIO register access
u8	sdio_read_byte(u8 func,u32 addr)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return 0;
}

//Only for SDIO register access
bool	sdio_write_byte(u8 func,u32 addr, u8 data)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
	return true;
}

s32	sdio_ask_rx_data_len(void)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
    return 0;
}

s32		sdio_read_data(u8 *dat, size_t size)
{
	int	status;


	SDIO_CHECK_OPENED();
	SDIO_TRACE("%s() <= : size = %d\n", __FUNCTION__, size);

	//SDIO_TRACE("out_buf = 0x%08x, out_len = %d\n", out_buf, out_len);
	//SDIO_TRACE("block_count = %d, block_size = %d\n", block_count, block_size);

	//OS_MutexLock(s_SDIOMutex);
    status = read(s_hSDIO, dat, size);

	//OS_MutexUnLock(s_SDIOMutex);
	if (status < 0)
	{
		// SDIO_FAIL_RET(-1, "READ_MULTI_BYTE, status = FALSE\n");
		return -1;
	}

	// SDIO_TRACE("%-20s : len = %d (0x%08x)\n", "READ_MULTI_BYTE", out_len, out_len);
    return status;
}

s32		sdio_read_dataEx(u8 *dat, size_t size)
{
	return read(s_hSDIO,dat,size);
}

#define SDOI_DUMMY_SIZE	(SDIO_DEF_BLOCK_SIZE>>SDIO_TX_ALLOC_SIZE_SHIFT)


s32	sdio_write_data(void *dat, size_t size)
{
    int	status;

    //s_write_data_cnt++;
	// LOG_PRINTF("sdio_write_data : # %d\n", s_write_data_cnt);

	//OS_MutexLock(s_SDIOMutex);
    status = write(s_hSDIO, dat, size);

	//OS_MutexUnLock(s_SDIOMutex);

	if (status < 0)
		SDIO_FAIL_RET(-1, "WRITE_MULTI_BYTE, status = FALSE\n");

    return status;
}

bool	sdio_get_multi_byte_io_port(u32 *port)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
	return true;
}

bool	sdio_set_multi_byte_io_port(u32 port)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
	return true;
}

bool	sdio_get_multi_byte_reg_io_port(u32 *port)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
	return true;
}

bool	sdio_set_multi_byte_reg_io_port(u32 port)
{
    SDIO_WARN("[wifi-sim:sdio-hci]: %s not support\n", __FUNCTION__);
	return true;
}

bool	sdio_open(void)
{
	SDIO_TRACE("%-20s%s : 0x%08x\n", __FUNCTION__, " <= ", s_hSDIO);

	if (s_hSDIO != 0)
	{
		SDIO_WARN("Try to open a already opened SDIO device!\n");
		return false;
	}

	s_hSDIO = open(SDIO_DEVICE_NAME, O_RDWR);
	if (s_hSDIO < 0)
	{
		s_hSDIO = 0;
		SDIO_FAIL_RET(0, "open(), error\n");
	}
    u32 ret = OS_SUCCESS;
    /*lint -save -e732*/
    ret = OS_TaskCreate(g_sim_rx_isr_info[0].task_func,
            g_sim_rx_isr_info[0].task_name,
			g_sim_rx_isr_info[0].stack_size<<4,
			g_sim_rx_isr_info[0].args,
            g_sim_rx_isr_info[0].prio,
            &h_sim_rx_isr);
     /*lint -restore*/

    if(ret != OS_SUCCESS)
        SDRV_INFO("Fail to open sim rx isr thread\n");
    else
        SDRV_INFO("Success to open sim rx isr thread\n");
	SDIO_TRACE("%-20s%s : 0x%08x\n", __FUNCTION__, " => ", s_hSDIO);
	return (s_hSDIO != 0);
}

bool	sdio_close(void)
{
	SDIO_CHECK_OPENED();
    sim_end = true;
    OS_TaskDelete(h_sim_rx_isr);

	if (close(s_hSDIO))
	{
		s_hSDIO = 0;
		SDIO_TRACE("%-20s :\n", __FUNCTION__);
		return true;
	}

	SDIO_WARN("sdio_close(0x%08x) fail!\n", s_hSDIO);
	return false;
}

bool	sdio_init(void)
{
//	s32		current_sdio_clock_rate = 0;
//	u32		n32;
//	u32		port_data, port_reg;
//	u8      u8data;

	SDIO_CHECK_OPENED();

	if (s_hSDIO == 0)
	{
		SDIO_ERROR("SDIO is NOT opened!\n", s_hSDIO);
		return false;
	}
	// mutex init
	OS_MutexInit(&s_SDIOMutex);
	LOG_PRINTF("<sdio init> : init sdio mutex ...\n");

	LOG_PRINTF("<sdio init> : success!! \n");
	return true;
}


bool sdio_ack_int(void)
{
    return false;
} // end of - sdio_ack_int -


bool sdio_read_sram(u32 addr, u8 *data, u32 size)
{
    return false;
} // end of - sdio_read_sram -

bool sdio_write_sram(u32 addr, u8 *data, u32 size)
{
    int status = 0;
    struct ssv6xxx_huw_cmd cmd;

    SDIO_CHECK_OPENED();

    cmd.in_data = (u8*)&addr;
    cmd.in_data_len = 4;
    cmd.out_data = data;
    cmd.out_data_len = size;

    status = ioctl(s_hSDIO,IOCTL_SSV6XXX_SDIO_WRITE_SRAM, &cmd);
    SDIO_TRACE("%-20s() : 0x%08x, 0x%08x\n", "WRITE_SRAM", addr, size);
    return true;
}// end of - sdio_write_sram -

s32 sdio_start(void)
{
    int status;
    struct ssv6xxx_huw_cmd cmd;
    ssv6xxx_memset((void *)&cmd, 0, sizeof(cmd));
    //printf("sdio_start\n");

  /*
    * Set tx interrupt threshold for EACA0 ~ EACA3 queue & low threshold
    */
    // Freddie ToDo: Use resource low instead of id threshold as interrupt source to reduce unecessary
    //Inital soc interrupt Bit13-Bit16(EDCA 0-3) Bit28(Tx resource low)
    //sdio_write_reg(ADR_SDIO_MASK, 0xf7fe1fff);

    //Setting Tx resource low ---ID[0x5] Page[0x15]
    #if 0
    sdio_write_reg(HAL_ADR_TX_LIMIT_INTR, 0x80000000 |
        HAL_TX_LOWTHRESHOLD_ID_TRIGGER << 16 |HAL_TX_LOWTHRESHOLD_PAGE_TRIGGER);
    #endif
    //sdio_write_reg(ADR_TX_LIMIT_INTR, 0x80020045);

    SDIO_CHECK_OPENED();
    ssv6xxx_memset((void *)&cmd, 0, sizeof(cmd));
    status = ioctl(s_hSDIO, IOCTL_SSV6XXX_SDIO_START, &cmd);

    SDIO_TRACE("%-20s()\n", "start");
    sim_pause = false;
    return status;
}

s32 sdio_stop(void)
{
    int status;
    struct ssv6xxx_huw_cmd cmd;
    //printf("sdio_stop\n");

    SDIO_CHECK_OPENED();
    ssv6xxx_memset((void *)&cmd, 0, sizeof(cmd));
    status = ioctl(s_hSDIO, IOCTL_SSV6XXX_SDIO_STOP, &cmd);
    SDIO_TRACE("%-20s()\n", "stop");
    sim_pause = true;

    return status;
}

u32 sdio_write_fw_to_sram(u8* fw_bin, u32 fw_bin_len, u32 block_size)
{
    u8   *fw_buffer = NULL;
    u8   *fw_ptr = fw_bin;
    u32  sram_addr = 0x00000000;
    u32  bin_len=fw_bin_len;
    u32  i=0,len=0, checksum=0, temp=0;
    u32  j=0;

    SDRV_TRACE("%s() => :%d\r\n",__FUNCTION__,__LINE__);
    fw_buffer = (u8 *)OS_MemAlloc(block_size);
    if (fw_buffer == NULL) {
        SDRV_FAIL("%s(): Failed to allocate buffer for firmware.\r\n",__FUNCTION__);
        return 0;
    }

    while(bin_len > 0){
        len=(bin_len >= block_size)?block_size:bin_len;
        bin_len -= len;
        if(len!=block_size){
            ssv6xxx_memset((void *)fw_buffer, 0xA5, block_size);
        }
        memcpy(fw_buffer,fw_ptr,len);
        fw_ptr += len;

        SDRV_DEBUG("%s(): read len=0x%x,sram_addr=0x%x\r\n",__FUNCTION__,len,sram_addr);

		if(FALSE== sdio_write_sram(sram_addr,fw_buffer,block_size)) goto ERROR;
        for (i = 0; i < (block_size)/4; i++) /* force 1024 bytes a set. */
        {
            temp = *((u32 *)(&fw_buffer[i*4]));
            checksum += temp;
        }
		sram_addr += block_size;
        j++;
        LOG_PRINTF("* ",__FUNCTION__);
        if(j%16==0)
            LOG_PRINTF("\r\n",__FUNCTION__);
    }// while(bin_len > 0){
    LOG_PRINTF("\r\n",__FUNCTION__);
    SDRV_DEBUG("%s(): checksum = 0x%x\r\n",__FUNCTION__, checksum);
    OS_MemFree(fw_buffer);
    SDRV_TRACE("%s() <= :%d\r\n",__FUNCTION__,__LINE__);
    return checksum;
ERROR:
    SDRV_INFO("\r\n",__FUNCTION__);
    OS_MemFree(fw_buffer);
    SDRV_ERROR("%s(): FAIL\r\n",__FUNCTION__);
    return 0;
}


void sdio_irq_enable(void)
{
    //simulate irq is turned on
    sim_pause = false;
}

void sdio_irq_disable(void)
{
    //simulate irq is turned off
    sim_pause = true;
}
