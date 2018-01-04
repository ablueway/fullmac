#ifndef __GPLIB_PRINT_STRING_H__
#define __GPLIB_PRINT_STRING_H__


#include "gplib.h"

#if GPLIB_PRINT_STRING_EN == 1
/*
* Function Name :  print_string
*
* Syntax : void print_string(CHAR *fmt, ...)
*
* Purpose :  print debug message via uart or usb
*
* Parameters : <IN> *fmt, ...:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/
extern void print_string(CHAR *fmt, ...);
#endif
/*
* Function Name :  get_string
*
* Syntax : void get_string(CHAR *s)
*
* Purpose :  get character string from uart
*
* Parameters : <IN>  *s:  character string
*              <OUT> none
* Return : none
*
* Note :
*
*/
extern void get_string(CHAR *s);


#endif 		// __GPLIB_PRINT_STRING_H__