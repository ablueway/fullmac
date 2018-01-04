#include "turnkey_filesrv_task.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define FileServTaskStackSize	1024	
#define C_FS_Q_MAX				8	//128
#define C_FS_MAX_MSG			8	//128
#define C_FS_MSG_LENGTH			128

#define C_FS_SCAN_Q_MAX			8	//10
#define C_FS_SCAN_MAX_MSG		8	//10
#define C_FS_SCAN_MSG_LENGTH	4

#define C_FILE_SCAN_COUNT		12		// you can scan 5 disk at one time

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct FileServ_s
{
	MSG_Q_ID fs_msg_q_id;
	MSG_Q_ID fs_scan_msg_q_id;
	INT32U gFileScanCount;
	STFileScanCtrl gstFileScanCtrl[C_FILE_SCAN_COUNT];	
} FileServ_t;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

 /**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void filesrv_task_entry(void *parm); 
static void FileSrvRead(P_TK_FILE_SERVICE_STRUCT para);

#if GPLIB_NVRAM_SPI_FLASH_EN == 1
static void FileSrvNVRAMAudioGPRSRead(P_TK_FILE_SERVICE_STRUCT para);
static void FileSrvNVRAMAudioAppRead(P_TK_FILE_SERVICE_STRUCT para);
static void FileSrvNVRAMAudioAppPackedRead(P_TK_FILE_SERVICE_STRUCT para);
static INT32S file_service_nvram_audio_gprs_read(INT16U sec_offset, INT32U buf_addr,INT16U sec_cnt);
static void FileSrvNvL3Read(P_TK_FILE_SERVICE_STRUCT para);
//static void FileSrvNvL3_Fast_Read(P_TK_FILE_SERVICE_STRUCT para);
static INT32S file_service_spi_l3_read(INT8U *src_name, INT32U buf_addr);
#endif

static void FileSrvScanFileInit(void);
static void FileSrvScanFileStart(STScanFilePara *para);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
MSG_Q_ID fs_msg_q_id;
MSG_Q_ID fs_scan_msg_q_id;
INT32U gFileScanCount;
STFileScanCtrl gstFileScanCtrl[C_FILE_SCAN_COUNT];
INT32U FileServTaskStack[FileServTaskStackSize];

INT32S filesrv_task_open(void)
{
	INT8U error;
	
	fs_msg_q_id = msgQCreate(C_FS_Q_MAX, C_FS_MAX_MSG, C_FS_MSG_LENGTH);
	if(fs_msg_q_id == NULL) {
		DBG_PRINT("Create file service message queue faile\r\n");
	}

	fs_scan_msg_q_id = msgQCreate(C_FS_SCAN_Q_MAX, C_FS_SCAN_MAX_MSG, C_FS_SCAN_MSG_LENGTH);
	if(fs_scan_msg_q_id == NULL){
		DBG_PRINT("Create file scan service message queue faile\r\n");
	}
		
    FileSrvScanFileInit();
    
	error = OSTaskCreate(filesrv_task_entry, (void *) 0, &FileServTaskStack[FileServTaskStackSize - 1], FS_PRIORITY); 
	if(error != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

static void filesrv_task_entry(void *parm)
{
	INT32U msg_id;
	INT32S status;
	INT8U* para;
	//INT8U  para[C_FS_MSG_LENGTH];

	para = (INT8U*) gp_malloc((INT32U)C_FS_MSG_LENGTH);
	while(1)
	{
		status = msgQReceive(fs_msg_q_id, &msg_id, (void *)para, C_FS_MSG_LENGTH);
		if(status != 0) {
			continue;
		}
		
		switch(msg_id)
		{		
			case MSG_FILESRV_FS_READ:
				FileSrvRead((P_TK_FILE_SERVICE_STRUCT)para);
				break;
				
		#if GPLIB_NVRAM_SPI_FLASH_EN == 1
			case MSG_FILESRV_NVRAM_AUDIO_GPRS_READ:
				FileSrvNVRAMAudioGPRSRead((P_TK_FILE_SERVICE_STRUCT)para);
				break;
				
			case MSG_FILESRV_NVRAM_AUDIO_APP_READ:
				FileSrvNVRAMAudioAppRead((P_TK_FILE_SERVICE_STRUCT)para);
				break;
				
			case MSG_FILESRV_NVRAM_AUDIO_APP_PACKED_READ:
				FileSrvNVRAMAudioAppPackedRead((P_TK_FILE_SERVICE_STRUCT)para);
				break;
				
			case MSG_FILESRV_SPI_L3_READ:
				FileSrvNvL3Read((P_TK_FILE_SERVICE_STRUCT)para);
				break;				
		#endif
			case MSG_FILESRV_FLUSH:
				OSQFlush(((P_TK_FILE_SERVICE_STRUCT) para)->result_queue);
				OSQPost(((P_TK_FILE_SERVICE_STRUCT) para)->result_queue, (void *) C_FILE_SERVICE_FLUSH_DONE);
				break;
				
			case MSG_FILESRV_SCAN:
				FileSrvScanFileStart((STScanFilePara *)para);
				msgQSend(fs_scan_msg_q_id, MSG_FILESRV_SCAN_CONTINUE, NULL, 0, MSG_PRI_NORMAL);
				break;
				
			default:
				break;
		}
	}
	gp_free(para);
}

void file_scan_task_entry(void *parm)
{
	INT32U msg_id;
	INT32S status;
	INT8U  para[C_FS_SCAN_MSG_LENGTH];

	while(1)
	{
		status = msgQReceive(fs_scan_msg_q_id, &msg_id, (void *)para, C_FS_SCAN_MSG_LENGTH);
		if(status != 0) {
			continue;
		}
		
		switch(msg_id)
		{
			case MSG_FILESRV_SCAN_CONTINUE:
				status = FileSrvScanFileContinue();
				if(status) {
					msgQSend(fs_scan_msg_q_id, MSG_FILESRV_SCAN_CONTINUE, NULL, 0, MSG_PRI_NORMAL);
				}
				break;
				
			default:
				break;
		}
	}
}

static void FileSrvRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;

	read_cnt = read(para->fd, para->buf_addr, para->buf_size);
	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

#if GPLIB_NVRAM_SPI_FLASH_EN == 1
static void FileSrvNVRAMAudioGPRSRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;

	read_cnt = file_service_nvram_audio_gprs_read(para->spi_para.sec_offset, para->buf_addr, para->spi_para.sec_cnt);
	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

static INT32S file_service_nvram_audio_gprs_read(INT16U sec_offset, INT32U buf_addr,INT16U sec_cnt)
{
	return sec_cnt*512;
}

static void FileSrvNVRAMAudioAppRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;
	
	nv_lseek(para->fd, para->data_offset, SEEK_SET);
		
	read_cnt = nv_read(para->fd, para->buf_addr, para->buf_size);
	if(read_cnt == 0) {
		read_cnt = para->buf_size;
	} else {
		read_cnt = -1;
	}
	
	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

static void FileSrvNVRAMAudioAppPackedRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt = 0;

	if (read_cnt == 0) {
		read_cnt = para->buf_size;
	}

	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

static void FileSrvNvL3Read(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;

	read_cnt = file_service_spi_l3_read(para->spi_para.src_name, para->buf_addr);
	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

static INT32S file_service_spi_l3_read(INT8U *src_name, INT32U buf_addr)
{
	INT32U handle = 0 ;
	INT32U size   = 0;

	handle = nv_open(src_name);
	if(handle == 0xffff) {
		return -1;
	}

	size = nv_lseek(handle,0,SEEK_END);
	nv_lseek(handle,0,SEEK_SET);

	if (nv_read(handle,(INT32U)buf_addr, size) != 0) {
		return -1;
	}

	return size;
}
#endif

static void FileSrvScanFileInit(void)
{
	INT32S i;

	gFileScanCount = 0;
    for(i = 0; i < C_FILE_SCAN_COUNT; i++) {
    	gstFileScanCtrl[i].flag = 0;
    }
}

// if scan file faile, what to do??
static void FileSrvScanFileStart(STScanFilePara *para)
{
	INT32S i;
	INT32S status;

	for(i = 0; i < C_FILE_SCAN_COUNT; i++) {
		if(!gstFileScanCtrl[i].flag) {
			break;
		}
	}
	
	if(i == C_FILE_SCAN_COUNT) {
		DBG_PRINT("no enough file service struct to use\r\n");
		return;
	}

#ifdef TK_FILESRV_DEBUG
	DBG_PRINT("start%d\t\t%s\r\n", i, para->pstFNodeInfo->pExtName ? para->pstFNodeInfo->pExtName : para->pstFNodeInfo->extname);
#endif

	gstFileScanCtrl[i].pstFNodeInfo = para->pstFNodeInfo;
	status = file_search_start(gstFileScanCtrl[i].pstFNodeInfo, &gstFileScanCtrl[i].stDiskFindControl);
	gstFileScanCtrl[i].flag = 1;
	if(para->result_queue) {
		OSQPost(para->result_queue, (void *) status);
	}
}

INT32S FileSrvScanFileContinue(void)
{
	INT32U i = 0;
	INT32S status;

	while(1)
	{
		if(gstFileScanCtrl[gFileScanCount].flag)
		{
			status = file_search_continue(gstFileScanCtrl[gFileScanCount].pstFNodeInfo, &gstFileScanCtrl[gFileScanCount].stDiskFindControl);
			if(status == 1)		// search complete
			{
				gstFileScanCtrl[gFileScanCount].flag = 0;		// free gstFileScanCtrl[i]
			#ifdef TK_FILESRV_DEBUG
				DBG_PRINT("end%d\r\n", gFileScanCount);
			#endif
			}
		#ifdef TK_FILESRV_DEBUG
			else
			{
				DBG_PRINT("c%d\r\n", gFileScanCount);
			}
		#endif

			gFileScanCount++;
			if(gFileScanCount == C_FILE_SCAN_COUNT)
			{
				gFileScanCount = 0;
			}
			return 1;
		}

		gFileScanCount++;
		if(gFileScanCount == C_FILE_SCAN_COUNT)
		{
			gFileScanCount = 0;
		}

		i++;
		if(i == C_FILE_SCAN_COUNT)
		{
			break;
		}
	}
	return 0;
}

