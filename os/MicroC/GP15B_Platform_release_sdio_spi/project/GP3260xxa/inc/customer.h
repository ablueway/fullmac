#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__

#define CUSTOM_ON	1
#define CUSTOM_OFF	0

/*=== IC Version definition choices ===*/
//---------------------------------------------------------------------------
						   //
#define GP3260XXA					0x1000
#define GP31FXXXA					0x2000
#define MCU_VERSION             	GP3260XXA                               //
//---------------------------------------------------------------------------

/*=== Board ID Config ===*/
//---------------------------------------------------------------------------
#define BOARD_EMU_BASE          0x00001000                                 //
                                                                           //
#define BOARD_DVP_GPB51PG     (BOARD_EMU_BASE + 0x10)                    //
#define BOARD_EMU_GPB51PG     (BOARD_EMU_BASE + 0x20)                    //
#define BOARD_TYPE              BOARD_EMU_GPB51PG
//---------------------------------------------------------------------------

#define MJ_DECODE_SCALER                0  // SCALER_0
#define UNDEFINE_SCALER                 0  // SCALER_0  

#endif //__CUSTOMER_H__
