#ifndef __drv_l1_GPIO_H__
#define __drv_l1_GPIO_H__
 
#include "driver_l1.h"
#include "drv_l1_sfr.h"
#include "drv_l1_tools.h"

/*GPIO Register define*/
/* GPIO: 0xC0000000 */
#define GPIO_BASE_ADDR             	0xC0000000
#define EACH_GPIO_DATA_REG_OFFSET   0x00000020 
#define EACH_DIR_REG_OFFSET        	0x00000020
#define EACH_ATTRIB_REG_OFFSET     	0x00000020

#define IOA_DATA_ADDR              		(GPIO_BASE_ADDR+0x00)   /*0xC0000000*/ 
#define IOA_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x04)   /*0xC0000004*/ 
#define IOA_DIR_ADDR               		(GPIO_BASE_ADDR+0x08)   /*0xC0000008*/ 
#define IOA_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x0C)   /*0xC000000C*/ 
#define IOA_DRV	                        (GPIO_BASE_ADDR+0x10)   /*0xC0000010*/ 
                                                                               
#define IOB_DATA_ADDR              		(GPIO_BASE_ADDR+0x20)   /*0xC0000020*/ 
#define IOB_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x24)   /*0xC0000024*/ 
#define IOB_DIR_ADDR               		(GPIO_BASE_ADDR+0x28)   /*0xC0000028*/ 
#define IOB_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x2C)   /*0xC000002C*/ 
#define IOB_DRV	                        (GPIO_BASE_ADDR+0x30)   /*0xC0000030*/ 
                                                                               
#define IOC_DATA_ADDR              		(GPIO_BASE_ADDR+0x40)   /*0xC0000040*/ 
#define IOC_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x44)   /*0xC0000044*/ 
#define IOC_DIR_ADDR              	 	(GPIO_BASE_ADDR+0x48)   /*0xC0000048*/ 
#define IOC_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x4C)   /*0xC000004C*/ 
#define IOC_DRV	                        (GPIO_BASE_ADDR+0x50)   /*0xC0000050*/ 
                                                                               
#define IOD_DATA_ADDR              		(GPIO_BASE_ADDR+0x60)   /*0xC0000060*/ 
#define IOD_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x64)   /*0xC0000064*/ 
#define IOD_DIR_ADDR               		(GPIO_BASE_ADDR+0x68)   /*0xC0000068*/ 
#define IOD_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x6C)   /*0xC000006C*/ 
#define IOD_DRV	                        (GPIO_BASE_ADDR+0x70)   /*0xC0000070*/ 
                                                                               
#define IOE_DATA_ADDR              		(GPIO_BASE_ADDR+0x80)   /*0xC0000080*/ 
#define IOE_BUFFER_ADDR            		(GPIO_BASE_ADDR+0x84)   /*0xC0000084*/ 
#define IOE_DIR_ADDR               		(GPIO_BASE_ADDR+0x88)   /*0xC0000088*/ 
#define IOE_ATTRIB_ADDR            		(GPIO_BASE_ADDR+0x8C)   /*0xC000008C*/ 
#define IOE_DRV	                        (GPIO_BASE_ADDR+0x90)   /*0xC0000090*/ 
                                                                               
/* Attribution Register High/Low definition */
#define INPUT_NO_RESISTOR       1
#define OUTPUT_UNINVERT_CONTENT 1 
#define INPUT_WITH_RESISTOR     0
#define OUTPUT_INVERT_CONTENT   0


#define GPIO_FAIL               0
#define GPIO_OK                 1

#define EACH_REGISTER_GPIO_NUMS 16

#define LOWEST_BIT_MASK         0x00000001

/* Direction Register Input/Output definition */
#define GPIO_INPUT              0       /* IO in input */
#define GPIO_OUTPUT             1       /* IO in output */

/* Attribution Register High/Low definition */
#define ATTRIBUTE_HIGH          1
#define INPUT_NO_RESISTOR       1
#define OUTPUT_UNINVERT_CONTENT 1
#define ATTRIBUTE_LOW           0
#define INPUT_WITH_RESISTOR     0
#define OUTPUT_INVERT_CONTENT   0
#define DATA_HIGH               1
#define DATA_LOW                0

/* Direction Register Input/Output definition */
#define HAL_GPIO_INPUT          0   /* IO in input */
#define HAL_GPIO_OUTPUT         1   /* IO in output */



#define DRV_WriteReg32(addr,data)     ((*(volatile INT32U *)(addr)) = (INT32U)data)
#define DRV_Reg32(addr)               (*(volatile INT32U *)(addr))



/* Bearer type enum */
typedef enum 
{
	GPIO_SET_A=0,
	GPIO_SET_B,
	GPIO_SET_C,
	GPIO_SET_D,
	GPIO_SET_E,
	GPIO_SET_MAX
} GPIO_SET_ENUM;


#ifndef __GPIO_TYPEDEF__
#define __GPIO_TYPEDEF__

typedef enum {
    IO_A0=0,
    IO_A1 ,
    IO_A2 ,
    IO_A3 ,
    IO_A4 ,
    IO_A5 ,
    IO_A6 ,
    IO_A7 ,
    IO_A8 ,
    IO_A9 ,
    IO_A10,
    IO_A11,
    IO_A12,
    IO_A13,
    IO_A14,
    IO_A15,
    IO_B0 ,
    IO_B1 ,
    IO_B2 ,
    IO_B3 ,
    IO_B4 ,
    IO_B5 ,
    IO_B6 ,
    IO_B7 ,
    IO_B8 ,
    IO_B9 ,
    IO_B10,
    IO_B11,
    IO_B12,
    IO_B13,
    IO_B14,
    IO_B15,
    IO_C0 ,
    IO_C1 ,
    IO_C2 ,
    IO_C3 ,
    IO_C4 ,
    IO_C5 ,
    IO_C6 ,
    IO_C7 ,
    IO_C8 ,
    IO_C9 ,
    IO_C10,
    IO_C11,
    IO_C12,
    IO_C13,
    IO_C14,
    IO_C15,
    IO_D0 ,
    IO_D1 ,
    IO_D2 ,
    IO_D3 ,
    IO_D4 ,
    IO_D5 ,
    IO_D6 ,
    IO_D7 ,
    IO_D8 ,
    IO_D9 ,
    IO_D10,
    IO_D11,
    IO_D12,
    IO_D13,
    IO_D14,
    IO_D15,
    IO_E0 ,
    IO_E1 ,
    IO_E2 ,
    IO_E3 ,
    IO_E4 ,
    IO_E5 ,
    IO_E6 ,
    IO_E7 ,
    IO_E8 ,
    IO_E9 ,
    IO_E10,
    IO_E11,
    IO_E12,
    IO_E13,
    IO_E14,
    IO_E15
} GPIO_ENUM;

#endif  //__GPIO_TYPEDEF__


#ifndef _GPIO_MONITOR_DEF_
#define _GPIO_MONITOR_DEF_
typedef struct {
    INT8U KS_OUT74_EN   ;
    INT8U KS_OUT31_EN   ;
    INT8U KS_OUT0_EN    ;
    INT8U KS_EN         ;
    INT8U TS_all_en     ;
    INT8U SEN_EN        ;
    INT8U KEYN_IN2_EN   ;
    INT8U KSH_EN        ;
    INT8U ND_SHARE_EN   ;
    INT8U NAND_EN       ;
    INT8U UART_EN       ;
    INT8U TFTEN         ;
    INT8U TFT_MODE1     ;
    INT8U TFT_MODE0     ;
    INT8U EN_CF         ;
    INT8U EN_MS         ;
    INT8U EN_SD123      ;
    INT8U EN_SD         ;
    INT8U USEEXTB       ;
    INT8U USEEXTA       ;
    INT8U SPI1_EN       ;
    INT8U SPI0_EN       ;
    INT8U CCP_EN2       ;
    INT8U CCP_EN1       ;
    INT8U CCP_EN0       ;
    INT8U KEYC_EN       ;
    INT8U CS3_0_EN_i3   ;
    INT8U CS3_0_EN_i2   ;
    INT8U CS3_0_EN_i1   ;
    INT8U CS3_0_EN_i0   ;
    INT8U XD31_16_EN    ;
    INT8U BKOEB_EN      ;
    INT8U BKUBEB_EN     ;
    INT8U XA24_19EN3    ;
    INT8U XA24_19EN2    ;
    INT8U XA24_19EN1    ;
    INT8U XA24_19EN0    ;
    INT8U SDRAM_CKE_EN  ;
    INT8U SDRAM_CSB_EN  ;
    INT8U XA24_19EN5    ;
    INT8U XA24_19EN4    ;
    INT8U XA14_12_EN2   ;
    INT8U XA14_12_EN1   ;
    INT8U XA14_12_EN0   ;
    INT8U EPPEN         ;
    INT8U MONI_EN       ;
} GPIO_MONITOR;

#endif  //_GPIO_MONITOR_DEF_


#ifndef _GPIO_DRVING_DEF_
#define _GPIO_DRVING_DEF_

typedef enum {
    IOA_DRV_4mA=0x0,
    IOA_DRV_8mA=0x1,
    IOA_DRV_12mA=0x2,
    IOA_DRV_16mA=0x3,
/* IOB Driving Options */
    IOB_DRV_4mA=0x0,
    IOB_DRV_8mA=0x1,
    IOB_DRV_12mA=0x2,
    IOB_DRV_16mA=0x3,    
/* IOC Driving Options */    
    IOC_DRV_4mA=0x0,
    IOC_DRV_8mA=0x1,
    IOC_DRV_12mA=0x2,
    IOC_DRV_16mA=0x3,     
/* IOD Driving Options */
    IOD_DRV_4mA=0x0,
    IOD_DRV_8mA=0x1,
    IOD_DRV_12mA=0x2,
    IOD_DRV_16mA=0x3,
/* IOE Driving Options */    
    IOE_DRV_4mA=0x0,
    IOE_DRV_8mA=0x1,    
    IOE_DRV_12mA=0x2,
    IOE_DRV_16mA=0x3 
} IO_DRV_LEVEL;


#endif //_GPIO_DRVING_DEF_

#ifndef XD_PIN_ENUM_DEF
#define XD_PIN_ENUM_DEF
    typedef enum
    {
      XD_CS1=0x2,   /* IOF1 */
      XD_CS2=0x4,   /* IOF2 */
      XD_CS3=0x8    /* IOF3 */
    } XD_CS_ENUM;

    typedef enum
    {
      XD_IOC12_15,   
      XD_IOC6_9,   
      XD_IOG5_6_10_11
    } XD_CTRL_PIN_ENUM;

    typedef enum
    {
      XD_DATA_IOB13_8,   
      XD_DATA_IOD5_0,   
      XD_DATA_IOE5_0,
      XD_DATA_IOA13_8
    } XD_DATA0_5_ENUM;

    typedef enum
    {
      XD_DATA_IOB15_14,   
      XD_DATA_IOD7_6,   
      XD_DATA_IOE7_6,
      XD_DATA_IOA15_14 
    } XD_DATA6_7_ENUM;
    
    typedef enum
    {
        ID_NAND,
        ID_NAND1=ID_NAND,
        ID_XD,
        ID_NAND2=ID_XD,
        ID_NULL
    } NAND_XD_ENUM;

    typedef enum {
        XD_ONLY_MODE=0,
        XD_NAND_MODE=1,
        XD_NAND1_NAND2_MODE=2,
        XD_PAD_MODE_MAX,
        XD_PAD_MODE_NULL=0xFF
    } XD_PAD_MODE;    
#endif  //XD_PIN_ENUM_DEF
/*
Function Name	gpio_init
Description	Initialize GPIO
Header File	drv_l1_gpio.h
Syntax	void gpio_init(void)
Parameters	None
Return Values	None
*/
extern void gpio_init(void);
/*
Function Name	gpio_read_io
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_read_io(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Input status of the whole IO port which contains this IO
*/
extern BOOLEAN gpio_read_io(INT32U port);
/*
Function Name	gpio_write_io
Description	Set IO port output data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_io(INT32U port, BOOLEAN data)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_write_io(INT32U port, BOOLEAN data);
/*
Function Name	gpio_init_io
Description	General IO setting
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			direction: Value filled in direction and attribute bit, 1: set as 1, 0: set as 1
Return Values	1: Success
			0: Failed. The assigned port is not existed.
*/
extern BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction);
/*
Function Name	gpio_set_port_attribute
Description	Set IO port attribute
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			attribute: 1: set as 1, 0: set as 0
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute);
/*
Function Name	gpio_write_all_by_set
Description	Set IO port output value
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			write_data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data);
/*
Function Name	gpio_read_all_by_set
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	INT32U gpio_read_all_by_set(INT8U gpio_set_num)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Not 0xFFFFFFFF: Input status of this IO
			0xFFFFFFFF: gpio_set_num is out of range
*/
extern INT32U gpio_read_all_by_set(INT8U gpio_set_num);
/*
Function Name	gpio_get_dir
Description	Get IO port direction
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_get_dir(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Direction setting of the specified IO
*/
extern BOOLEAN gpio_get_dir(INT32U port);
/*
Function Name	gpio_drving_init_io
Description	Set GPIO driving capability
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			gpio_driving_level: IO drivering capability
Return Values	1: Success
			0: Failed
*/
extern BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level);
/*
Function Name	gpio_monitor
Description	Read monitor statues
Header File	drv_l1_gpio.h
Syntax	void gpio_monitor(GPIO_MONITOR *gpio_monitor)
Parameters	Pointer of structure which is used to store the monitor status
Return Values	None
*/
extern void gpio_monitor(GPIO_MONITOR *gpio_monitor);

/*
Function Name	gpio_drving_uninit
Description	Set GPIO driving capability to the lowest level
Header File	drv_l1_gpio.h
Syntax	void gpio_drving_uninit(void)
Parameters	None
Return Values	None
*/
extern void gpio_drving_uninit(void);

/*
Function Name	gpio_set_ice_en
Description	Set IOH[5:2] as JTAG pins or GPIOs
Header File	drv_l1_gpio.h
Syntax	void gpio_set_ice_en(BOOLEAN status)
Parameters	status: 1: BKUEB, 0: GPIOs
Return Values	None
*/
extern void gpio_set_ice_en(BOOLEAN status);
/*
Function Name	gpio_IOE_switch_config_from_HDMI_to_GPIO
Description	Set IOE from HDMI to GPIO
Header File	drv_l1_gpio.h
Syntax	void gpio_IOE_switch_config_from_HDMI_to_GPIO(void)
Parameters		None
Return Values	None
*/
extern void gpio_IOE_switch_config_from_HDMI_to_GPIO(void);


#endif /* __drv_l1_GPIO_H__ */
