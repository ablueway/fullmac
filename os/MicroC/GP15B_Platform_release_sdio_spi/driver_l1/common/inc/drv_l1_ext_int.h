#ifndef __drv_l1_EXT_INT_H__
#define __drv_l1_EXT_INT_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

//external interrupt driver
typedef enum
{
	EXTA,
	EXTB,
	EXTC
}EXTABC_ENUM;

typedef enum
{
	FALLING = 0,
	RISING
}EXTABC_EDGE_ENUM;


#define EXTC_INT 0x100
#define EXTB_INT 0x80
#define EXTA_INT 0x40

#define EXTC_POL 0x20
#define EXTB_POL 0x10
#define EXTA_POL 0x08

#define EXTC_IEN 0x04
#define EXTB_IEN 0x02
#define EXTA_IEN 0x01


/*
Function Name	drv_l1_ext_int_init
Description	Initiate External interrupt
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_ext_int_init(void)
Parameters	None
Return Values	None
*/
extern void drv_l1_ext_int_init(void);
/*
Function Name	drv_l1_extabc_int_clr
Description	Clear interrupt flag of EXTA or EXTB or EXTC
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_extabc_int_clr(INT8U ext_src)
Parameters	EXTA
			EXTB
Return Values	None
*/
extern void drv_l1_extabc_int_clr(INT8U ext_src);
/*
Function Name	drv_l1_extabc_edge_set
Description	External interrupt A or B or C input edge selection
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_extabc_edge_set(INT8U ext_src, INT8U edge_type)
Parameters	ext_src: EXTA
					EXTB
					EXTC
	edge_type: 1 : Rising edge trigger
				0 : Falling edge trigger
Return Values	None
*/
extern void drv_l1_extabc_edge_set(INT8U ext_src, INT8U edge_type);
/*
Function Name	drv_l1_extabc_enable_set
Description	Enable/disable EXTA or EXTB or EXTC interrupt
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_extabc_enable_set(INT8U ext_src, BOOLEAN status)
Parameters	ext_src: EXTA
					EXTB
					EXTC
			status: TRUE: Enable
				       FALSE: Disable
Return Values	None
*/
extern void drv_l1_extabc_enable_set(INT8U ext_src, BOOLEAN status);
/*
API Name	drv_l1_extabc_user_isr_set
Description	Set ISR of EXTA/EXTB/EXTC
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_extabc_user_isr_set(INT8U ext_src,void (*user_isr)(void))
Parameters	ext_src: EXTA
					EXTB
					EXTC
			*user_isr: Assigned ISR pointer
Return Values	None
*/
extern void drv_l1_extabc_user_isr_set(INT8U ext_src,void (*user_isr)(void));
/*
Function Name	drv_l1_extabc_user_isr_clr
Description	Clear ISR of EXTA/EXTB/EXTC
Header File	drv_l1_ext_int.h
Syntax	void drv_l1_extabc_user_isr_clr(INT8U ext_src)
Parameters	ext_src: EXTA
					EXTB
					EXTC
Return Values	None
*/
extern void drv_l1_extabc_user_isr_clr(INT8U ext_src);

#endif		// __drv_l1_EXT_INT_H__