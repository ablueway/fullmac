#ifndef __TURNKEY_FILESRV_TASK_H__
#define __TURNKEY_FILESRV_TASK_H__
#include "application.h"
#include "msg_manager.h"

#define C_FILE_SERVICE_FLUSH_DONE		0xFF010263

typedef struct tk_audiobuf{
	INT32U status;
	INT32U ri;
	INT32U wi;
	INT32U length;
	INT8U  *audiobufptr; 	
	struct tk_audiobuf *next;
}STAVIAUDIOBufConfig;

typedef struct
{
	INT8S	flag;
	struct STFileNodeInfo *pstFNodeInfo;
	STDiskFindControl stDiskFindControl;
} STFileScanCtrl;

typedef struct {
	INT8U   src_id;
	INT8U   src_name[10];
	INT16U  sec_offset;
	INT16U  sec_cnt;
} TK_FILE_SERVICE_SPI_STRUCT, *P_TK_FILE_SERVICE_SPI_STRUCT;

typedef struct {
	INT16S fd;
	INT32U buf_addr;
	INT32U buf_size;
	TK_FILE_SERVICE_SPI_STRUCT spi_para;
	OS_EVENT *result_queue;
	INT32U data_offset;
	INT8U  main_channel;
	INT8U  *data_start_addr;
} TK_FILE_SERVICE_STRUCT, *P_TK_FILE_SERVICE_STRUCT;

typedef struct 
{
	INT16S disk;
	OS_EVENT *result_queue;
} STMountPara;

typedef struct 
{
	struct STFileNodeInfo *pstFNodeInfo;
	OS_EVENT *result_queue;
} STScanFilePara;

typedef struct
{
	INT8U	src_storage_id;
	INT8U	file_type;
	INT32U	file_idx;
	INT8U	dest_storage_id;
	INT8S	dest_name[32];
	INT32S	status;
} TK_FILE_COPY_STRUCT, *P_TK_FILE_COPY_STRUCT;

typedef struct
{
	f_pos	fpos;
	INT32U	file_idx;
	INT32S	status;
} TK_FILE_DELETE_STRUCT, *P_TK_FILE_DELETE_STRUCT;

typedef struct
{
	INT32U	*p_buffer;
	INT32U	size;
	INT8S	ext_name[4];
	INT8U	dest_storage_id;
	INT8S	dest_name[32];
	INT32S	status;
} TK_FILE_SAVE_STRUCT, *P_TK_FILE_SAVE_STRUCT;

extern MSG_Q_ID fs_msg_q_id;
extern MSG_Q_ID fs_scan_msg_q_id;
extern INT32S filesrv_task_open(void);
extern void file_scan_task_entry(void *parm);
extern INT32S FileSrvScanFileContinue(void);
#endif /*__TURNKEY_FILESRV_TASK_H__*/
