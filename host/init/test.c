/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#include <priv/ssv_types.h>
/************************************************************************************************************************************************************************
*
*   This main function just use in FREERTOS """"""SIM"""" platform
*   other platform need to call the function,ssv6xxx_dev_init to initialize wifi module
*   in the customer's task. 
*   Note: ssv6xxx_dev_init need to be call after os starting schedule(this function include some os commands inside)
*
************************************************************************************************************************************************************************/
s32 main(s32 argc, char *argv[])
{

    return 0;
}

