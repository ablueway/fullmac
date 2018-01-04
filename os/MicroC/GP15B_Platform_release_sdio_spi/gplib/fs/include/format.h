#ifndef	__FORMAT_H__
#define __FORMAT_H__

#define format_printf(x)   
#define format_printf1(x,y)   

/***************************************************************************/
#define C_MBR_ZeroSec_Offset			0x000
#define C_MBR_Type_Addr					0x1c2
#define C_MBR_SecOffset_Addr			0x1c6
#define C_MBR_SecNumber_Addr			0x1ca

#define C_DBR_First_Byte				0xeb
#define C_DBR_FAT16_Second_Byte			0x3c
#define C_DBR_FAT32_Second_Byte			0x58
#define C_DBR_Third_Byte				0x90

#define MBR_FAT16_Type	        		0x06
#define MBR_FAT32_Type	        		0x0B
#define MBR_EXFAT_Type					0x07

#define DBR_nbyte_Addr					0x0b
#define DBR_nsector_Addr				0x0d
#define DBR_nreserved_Addr				0x0e
#define DBR_nfat_Addr					0x10
#define DBR_ndirent_Addr				0x11
#define DBR_nsize_Addr					0x13
#define DBR_mdesc_Addr					0x15
#define DBR_nfsect_Addr					0x16
#define DBR_hidden_Addr					0x1c			
#define DBR_huge_Addr					0x20
#define DBR_xnfsect_Addr				0x24
#define DBR_xflags_Addr		    		0x28
#define DBR_xfsversion_Addr		   		0x2a
#define DBR_xrootclst_Addr				0x2c
#define DBR_xfsinfosec_Addr				0x30
#define DBR_xbackupsec_Addr		    	0x32
#define DBR_FAT16_VOL_ID				39
#define DBR_FAT16_VOL_LAB			    43
#define DBR_FAT32_VOL_ID				67
#define DBR_FAT32_VOL_LAB			    71

#define DBR_FAT16_FilSysType			54
#define DBR_FAT32_FilSysType			82

#define DBR_end_Addr					0x1fe


#define ExPartionOffSet				0x40
#define ExVolumeLongth				0x48
#define ExFatTableoffset			0x50
#define ExFatLenth					0x54
#define	ExClusterHeapOffset			0x58
#define ExClusterCount				0x5C
#define ExRootCluster				0x60
#define ExVolumeSerialNumber		0x64
#define ExFileSysRevision			0x68
#define ExVolumeFlags				0x6A
#define ExPerSectorShift			0x6C
#define ExClusterPerShift			0x6D
#define ExNumofFats					0x6E
#define ExDriveSelect				0x6F
#define ExPercentUse				0x70


#define FORMAT_UPCASE_LENGTH 			5836





struct DSKSZTOSECPERCLUS
{
 	 INT32S DiskSize;
   #if WITHEXFAT  == 1 
     INT16S  SecPerClusVal;
   #else
	 INT8S  SecPerClusVal;
   #endif 
};

#endif
