#ifndef DRV_L2_NAND_EXT_H
#define DRV_L2_NAND_EXT_H

/************* Common for GPL326XXB  ************************/
#include "project.h"

#if _OPERATING_SYSTEM != _OS_NONE
#include "os.h"
#endif


#if (defined _OPERATING_SYSTEM) && (_OPERATING_SYSTEM != _OS_NONE)
extern  OS_EVENT * gNand_sem;
extern  OS_EVENT * gNand_PHY_sem;
#endif

#ifndef NAND_READ_ALLOW
#define NAND_READ_ALLOW	    0x1 //partition mode
#endif  //NAND_READ_ALLOW

#ifndef NAND_WRITE_ALLOW
#define NAND_WRITE_ALLOW	0x2 //partition mode
#endif //NAND_WRITE_ALLOW

#define HEADER_OFFSET_APP_FLAG 					0x1c0	/*4 bytes*/
#define HEADER_OFFSET_APP_START 				0x1c4	/*1 bytes*/
#define HEADER_OFFSET_APP_SIZE  				0x1c5	/*2 bytes*/
#define HEADER_OFFSET_APP_PRENT 				0x1c7	/*1 bytes*/


#define HEADER_OFFSET_USER_SETTING_SIZE_BLK		0x1d0	/*4 bytes*/
#define HEADER_OFFSET_PARTITION_NUM				0x1d7	/*1 bytes*/ 	//0,1,2
#define HEADER_OFFSET_NAND_PART0_SIZE_BLK   	0x1d8   /*4 bytes*/		//size = 0:auto	
#define HEADER_OFFSET_NAND_PART1_SIZE_BLK		0x1dc   /*4 bytes*/		//size = 0:auto
#define HEADER_OFFSET_NAND_PART0_ATTRIBUTE   	0x1e3   /*1 bytes*/		//1,2,3
#define HEADER_OFFSET_NAND_PART1_ATTRIBUTE   	0x1e7   /*1 bytes*/		//1,2,3

#define HEADER_BANK_BLOCKS   					0x1e8	/*4 bytes*/	
#define HEADER_RECYCLE_BLOCKS					0x1ec	/*4 bytes*/	

#define HEADER_NAND_MB_SIZE                     0x1f8   /*4 bytes*/
#define HEADER_MAX_MCLK_MHz                     0x1fe   /*1 byte*/
#define HEADER_OFFSET_WP_PIN_ID                 0x1ff   /*1 byte*/

#define GPL6_BOOTAREA_BCH_SET_PAGE_NUMS		4	
#define GPL6_BOOTAREA_BCH_CHG_CYCLE		    (GPL6_BOOTAREA_BCH_SET_PAGE_NUMS*2)

#ifndef _NF_STATUS
#define _NF_STATUS
typedef enum
{
	NF_OK 										= 0x0000,
	
	NF_APP_ILL_TAG						= 0xff00,
	NF_APP_PART_NO_EXIST				= 0xff01,
	
	NF_UNKNOW_TYPE						= 0xff1f,
	NF_USER_CONFIG_ERR				= 0xff2f,
	NF_TOO_MANY_BADBLK				= 0xff3f,
	NF_READ_MAPTAB_FAIL				= 0xff4f,
	NF_SET_BAD_BLK_FAIL				= 0xff5f,
	NF_NO_SWAP_BLOCK					= 0xff6f,
	NF_BEYOND_NAND_SIZE				= 0xff7f,
	NF_READ_ECC_ERR						= 0xff8f,
	NF_READ_BIT_ERR_TOO_MUCH	= 0xff9f,
	NF_ILLEGAL_BLK_MORE_2			= 0xffaf,
	NF_BANK_RECYCLE_NUM_ERR		= 0xffbf,
	NF_EXCHAGE_BLK_ID_ERR			= 0xffcf,
	NF_DIRECT_READ_UNSUPPORT	= 0xffdf,
	NF_DMA_ALIGN_ADDR_NEED		= 0xfff0,
	NF_APP_LOCKED							= 0xfff1,
	NF_NO_BUFFERPTR						= 0xfff2,
	NF_FIX_ILLEGAL_BLK_ERROR	= 0xfff3,
	NF_FORMAT_POWER_LOSE			= 0xfff4,
	NF_NAND_PROTECTED					= 0xfff5,
	NF_FIRST_ALL_BANK_OK			= 0xffef
}NF_STATUS;
#endif

#if !defined(_NF_APP_PART_INFO)
#define _NF_APP_PART_INFO
typedef struct
{
	UINT32 part_size; //sector
	UINT32 checkSum;
	UINT32 startSector; //sector
	UINT32 imageSize; //sector
	UINT32 destAddress;
	UINT8  type;
	UINT8  version; 
}NF_APP_PART_INFO;
#endif

#ifndef _NF_APP_KIND
#define _NF_APP_KIND
typedef enum {
    APP_RUNTIME_CODE   	=   0,
    APP_RESOURCE     	=   1,
    APP_FASTBOOT_BIN 	=   2,
    APP_QUICK_IMAGE     =   3,
    APP_HIBRATE_IMAGE   =   4,
    APP_IMAGE_FLAG   	=   5,
    APP_SECOND_BTLDR   	=   6,
    APP_KIND_MIN		= APP_RUNTIME_CODE,
    APP_KIND_MAX		= APP_SECOND_BTLDR
}NF_APP_KIND;
#endif

extern void Nand_OS_LOCK(void);
extern void Nand_OS_UNLOCK(void);

extern INT32U Nand_part0_size_Get(void);
extern INT32U NandAppByteSizeGet(void);
extern INT32U NandBootAreaByteSizeGet(void);
extern INT32U CalculateFATArea(void);
extern INT16U GetBadFlagFromNand(INT16U wPhysicBlkNum);
extern INT32U nand_block_checkbad(INT32U block_id, INT8U* data_buf);

//APP-Zone API
/*
* Function Name :  NandBootEnableWrite
*
* Syntax :   void   NandBootEnableWrite(void);
*
* Purpose :    to enable the wirte fuction of App area.
*
* Parameters : <IN> none
*                    <OUT> none
* Return :   - None
*	   
* Note :  After this function is called, erase/write function is allow in App area.
*
*/
extern void   NandBootEnableWrite(void);
/*
* Function Name :  NandBootDisableWrite
*
* Syntax :   void   NandBootDisableWrite(void);
*
* Purpose :    to disable the wirte fuction of App area.
*
* Parameters : <IN> none
*                    <OUT> none
* Return :   - None
*	   
* Note :  After this function is called, erase/write function is not allowed in App area unless NandBootEnableWrite() is called.
*
*/
extern void   NandBootDisableWrite(void);
/*
* Function Name :  NandBootFormat
*
* Syntax :  INT32U NandBootFormat(INT16U format_type);
*
* Purpose :   to  format the Nand flash.
*
* Parameters : <IN> format_type
*                           0x01: Erase the blocks except original bad blocks and user-defined bad blocks
*                           0x10: Erase the blocks except original bad blocks
*                   <OUT> none
* Return :   0: Success
*               Others: Fail
*	   
* Note :  This function can be used only for App area. Before calling this function, 
*		user must call NandBootEnableWrite() first to set App are as writeable.
*
*/
extern INT32U NandBootFormat(INT16U format_type);
/*
* Function Name :  NandBootFlush
*
* Syntax : INT32U NandBootFlush(void);
*
* Purpose :   to make sure the related information is updated and stored in Nand.
*
* Parameters : <IN> void
*                   <OUT> none
* Return :   0: Success
*               Others: Fail
*	   
* Note : Before stopping writing App area, disabling Nand flash or turning off the power, 
*	     user must call this function to prevent from losing some information. 
*
*/
extern INT32U NandBootFlush(void);
/*
* Function Name :  NandBootInit
*
* Syntax : INT32U NandBootInit(void);
*
* Purpose :   to initialize Nand flash app area.
*
* Parameters : <IN> void
*                   <OUT> none
* Return :    0: Success. maptable is found.
*		   -1: Fail. maptable is no found.
*	   
* Note : This function can be used only for App area. If the maptable is not found, 
*           user has to call NandBootFormat() to generate maptable and then call this function again. 
*
*/
extern INT32U NandBootInit(void);
/*
* Function Name :  NandBootWriteSector
*
* Syntax : INT32U NandBootWriteSector(INT32U wWriteLBA, INT16U wLen, INT32U DataBufAddr);
*
* Purpose :   to write App area.
*
* Parameters : <IN> wWriteLBA: start address, it's present by logical sector
*                              wLen: write sector number
*                              DataBufAddr: target address in working memory
*                   <OUT> none
* Return :   0: Success
*               Others: Fail
*	   
* Note : 
*
*/
extern INT32U NandBootWriteSector(INT32U wWriteLBA, INT16U wLen, INT32U DataBufAddr);
/*
* Function Name :  NandBootReadSector
*
* Syntax : INT32U NandBootReadSector(INT32U wReadLBA, INT16U wLen, INT32U DataBufAddr);
*
* Purpose :   to write App area.
*
* Parameters : <IN> wReadLBA: start address, it's present by logical sector
*                              wLen: read sector number
*                              DataBufAddr: target address in working memory
*                   <OUT> none
* Return :   0: Success
*               Others: Fail
*	   
* Note : 
*
*/
extern INT32U NandBootReadSector(INT32U wReadLBA, INT16U wLen, INT32U DataBufAddr);

//Data-Zone API
/*
* Function Name :  DrvNand_initial
*
* Syntax : INT16U DrvNand_initial(void);
*
* Purpose :  build maptab & bank infomation.
*
* Parameters : <IN> void
*              <OUT> none
* Return : 
*             - NAND_OK  if successful; 
*             NF_UNKNOW_TYPE if nand initial fail or page size bigger than driver support
*			  NF_NO_BUFFERPTR if no buffer was allocated.
*			  NF_FORMAT_POWER_LOSE if lose power when format data area
*			  NF_BCH_MODE_N_MATCH if read bank info fail with all bch mode.
* Note :
*
*/
extern INT16U DrvNand_initial(void);
/*
* Function Name :  DrvNand_lowlevelformat
*
* Syntax : INT16U DrvNand_lowlevelformat(void);
*
* Purpose :  to format nand data area with low level.
*
* Parameters : <IN> void
*                   
*              <OUT> none
* Return :     NF_OK if successful; 
*              NF_UNKNOW_TYPE if nand initial fail or page size bigger than driver support
*			   NF_NO_BUFFERPTR if no buffer was allocated.
*			   NF_NAND_PROTECTED if it is set write-protected attribute when erase nand data area
* Note :
*
*/
extern INT16U DrvNand_lowlevelformat(void);
/*
* Function Name :  DrvNand_get_Size
*
* Syntax : INT32U DrvNand_get_Size(void);
*
* Purpose :  to get ths size of Data area.
*
* Parameters : <IN> void
*                     <OUT> none
*              
* Return :     -Size of Data area. The unit is sector(512 bytes).
*
* Note :
*
*/
extern INT32U DrvNand_get_Size(void);
/*
* Function Name :  DrvNand_write_sector
*
* Syntax : INT16U DrvNand_write_sector(INT32U wWriteLBA, INT16U wLen, INT32U DataBufAddr)
*
* Purpose :  to write data to nand data area with sectors..
*
* Parameters : <IN> wWriteLBA:start sector address to write data area.
*                   wLen: data length to read , unit: sector
*                   DataBufAddr: buffer used to store read back data from nand.
*              <OUT> none
* Return :     NF_OK if successful; 
*              0xffff if nand data area has not been initilized or current nvram type is nand rom.
*			   NF_DMA_ALIGN_ADDR_NEED if dma address is not aligned.
*              NF_BEYOND_NAND_SIZE if address to write is overlapped the nand actual size of data area.
* Note :
*
*/
extern INT16U DrvNand_write_sector(INT32U , INT16U , INT32U );
/*
* Function Name :  DrvNand_read_sector
*
* Syntax : INT16U DrvNand_read_sector(INT32U wReadLBA, INT16U wLen, INT32U DataBufAddr);
*
* Purpose :  to read nand data area with sectors..
*
* Parameters : <IN> wReadLBA:start sector address to read data area.
*                   wLen: data length to read , unit: sector
*                   DataBufAddr: buffer used to store read back data from nand.
*              <OUT> none
* Return :    -NF_OK if successful; 
*                0xffff if nand data area has not been initilized or current nvram type is nand rom.
*			   NF_DMA_ALIGN_ADDR_NEED if dma address is not aligned.
*              NF_BEYOND_NAND_SIZE if address to read is overlapped the nand actual size of data area.
* Note :
*
*/
extern INT16U DrvNand_read_sector(INT32U , INT16U , INT32U );
/*
* Function Name :  DrvNand_flush_allblk
*
* Syntax : INT16U	DrvNand_flush_allblk(void);
*
* Purpose :  to flush all block after writing data area.
*
* Parameters : <IN> void
*                   
*              <OUT> none
* Return :     0x00 if successful; 
*              0xffff if nand data area has not been initilized or current nvram type is nand rom.
*			   
* Note :
*
*/
extern INT16U DrvNand_flush_allblk(void);
/*
* Function Name :  nand_part0_para_set
*
* Syntax : void nand_part0_para_set(INT32U offset, INT32U size, INT16U mode);
*
* Purpose :  to set the configuration of Part0 of Data area which can be.
*
* Parameters : <IN> offset: start address
*                   		   size: size of Part0
*				  mode: 0x0: Not readable & not writeable
*						0x1: readable
*						0x2: writeable
*						0x3: readable & writeable
*                   <OUT> none
* Return :     The start address is logical address. The unit of offset and size is sector(512 bytes). 
*                  
*			   
* Note :
*
*/
extern void nand_part0_para_set(INT32U offset, INT32U size, INT16U mode);
extern void nand_part1_para_set(INT32U offset, INT32U size, INT16U mode);
extern void nand_part2_para_set(INT32U offset, INT32U size, INT16U mode);

extern void Nand_part0_Offset_Set(INT32U nand_fs_sector_offset);
extern void Nand_part0_mode_Set(INT16U mode);
extern void Nand_part0_size_Set(INT32U nand_fs_sector_size);
extern void Nand_part1_Offset_Set(INT32U nand_fs_sector_offset);
extern void Nand_part1_mode_Set(INT16U mode);
extern void Nand_part1_size_Set(INT32U nand_fs_sector_size);
/*
* Function Name :  Nand_part1_Offset_Get
*
* Syntax : INT32U Nand_part1_Offset_Get(void);
*
* Purpose :  to get the start address of Part1 of Data area which can be accessed by File system
*
* Parameters : <IN> void
*                   <OUT> none
* Return :     Start address of Part0 
*                  
*			   
* Note : Before calling this function, NandInfoAutoGet() must be called.
*
*/
extern INT32U Nand_part1_Offset_Get(void);
/*
* Function Name :  Nand_part1_mode_Get
*
* Syntax : INT32U Nand_part1_mode_Get(void);
*
* Purpose :  to get the read/write mode of Part1 of Data area
*
* Parameters : <IN> void
*                   <OUT> none
* Return :    0x0: Not readable & not writeable
*			0x1: readable
*			0x2: writeable
*			0x3: readable & writeable		   
* Note : Before calling this function, NandInfoAutoGet() must be called.
*
*/
extern INT32U Nand_part1_mode_Get(void);
/*
* Function Name :  Nand_part1_size_Get
*
* Syntax : INT32U Nand_part1_size_Get(void);
*
* Purpose :  to get the size  of Part1 of Data area
*
* Parameters : <IN> void
*                   <OUT> none
* Return :    return the real size of part1 of data area. Unit:sector.
	   
* Note : Before calling this function, NandInfoAutoGet() must be called.
*
*/
extern INT32U Nand_part1_size_Get(void);

extern INT32U Nand_part0_Offset_Get(void);
extern INT32U Nand_part0_mode_Get(void);

extern void   Nand_Partition_Num_Set(INT16U number);
/*
* Function Name :  Nand_Partition_Num_Get
*
* Syntax : INT16U Nand_Partition_Num_Get(void);
*
* Purpose :  to get the partition numbers  of Part1 of Data area
*
* Parameters : <IN> void
*                   <OUT> none
* Return :    return the real partition numbers of data area. 
	   
* Note : 
*
*/
extern INT16U Nand_Partition_Num_Get(void);
/*
* Function Name :  NandInfoAutoGet
*
* Syntax : INT32S NandInfoAutoGet(void);
*
* Purpose :   to get the configuration of Nand flash
*
* Parameters : <IN> void
*                   <OUT> none
* Return :    return the Configuration of Nand flash. 
	   
* Note : 
*
*/
extern INT32S NandInfoAutoGet(void);
extern INT32S NandAppInfoAutoGet(void);
extern INT32U DrvNand_bchtable_alloc(void); //not implemented
extern void DrvNand_bchtable_free(void); //not implemented

extern void FlushWorkbuffer(void);

extern void setSize4Partition(INT8U partitionIndex, INT32U sizeInSector);
extern INT32U getSize4Partition(INT8U partitionIndex);

extern void setMode4Partition(INT8U partitionIndex, INT8U mode);
extern INT8U getMode4Partition(INT8U partitionIndex);

extern INT32U nand_sector_nums_per_page_get(void);
extern INT32U nand_page_nums_per_block_get(void);

extern SINT32 NandAppInitParts(UINT16 *partTotal, UINT32 *partTotalSector);
extern SINT32 NandAppGetPartInfo(UINT16 whichPart, NF_APP_PART_INFO *partInfo);
extern SINT32 NandAppFindPart(UINT16 index, UINT8 type, NF_APP_PART_INFO *partInfo);
extern UINT32 NandAppGetTotalFormatStep(void);

#endif //DRV_L2_NAND_EXT_H
