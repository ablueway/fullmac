######################################################
#
# Comple tools path assign
#
######################################################

OUTPUT_LIB_NAME = task_wifi_lib.a

SRC_TOP_PATH= X:

HOST_DIR                := $(SRC_TOP_PATH)/application/task_wifi_lib/inc/host
LWIP_DIR                := $(HOST_DIR)/tcpip/lwip-1.4.0
#IPERF_DIR               := $(HOST_DIR)/app/netapp/iperf3.0
NETMGR_DIR               := $(HOST_DIR)/app/netmgr
DHCP_DIR               := $(HOST_DIR)/app/netapp/udhcp

MAKETOOLS_PATH= $(SRC_TOP_PATH)/tools/make_tools

.SUFFIXES:

sinclude	$(MAKETOOLS_PATH)/Makefile.utl




######################################################
#
# ENDIAN select
#
######################################################

ENDIAN          := LITTLE
#ENDIAN         := BIG

ifeq "$(ENDIAN)" "LITTLE"
  ENDIAN        = -EL
  GOAL					= $(OUTPUT_LIB_NAME)
  CFLAGS_ENDIAN = -DLITTLE_ENDIAN
else
  ENDIAN        =
  GOAL					= $(OUTPUT_LIB_NAME)
  CFLAGS_ENDIAN = -DBIG_ENDIAN
endif

######################################################
#
# Set inlcude file path
#
######################################################

INCLUDE_DIR	= -I../inc \
              -I$(SRC_TOP_PATH)/driver_l1/common/inc \
						  -I$(SRC_TOP_PATH)/project/$(PROJECTNAME)/inc	\
						  -I$(SRC_TOP_PATH)/application/inc \
						  -I$(SRC_TOP_PATH)/gplib/inc \
						  -I$(SRC_TOP_PATH)/gpstdlib/inc \
						  -I$(SRC_TOP_PATH)/driver_l1/inc	\
						  -I$(SRC_TOP_PATH)/driver_l2/inc	\
						  -I$(SRC_TOP_PATH)/os/inc 		
						  
INCLUDE_DIR += 	-I$(HOST_DIR) \
          			-I$(HOST_DIR)/include \
	  						-I$(HOST_DIR)/include/priv \
          			-I$(HOST_DIR)/include/priv/hw

INCLUDE_DIR += 	-I$(HOST_DIR)/ap 

INCLUDE_DIR += 	-I$(HOST_DIR)/app \
           			-I$(HOST_DIR)/app/netapp \
           			-I$(HOST_DIR)/drv \
           			-I$(HOST_DIR)/hal \
           			-I$(HOST_DIR)/os_wrapper/MicroC \
           			-I$(HOST_DIR)/platform/gp \
                -I$(HOST_DIR)/netstack_wrapper/icomm_lwIP
           
INCLUDE_DIR += -I$(LWIP_DIR)/src/include/ \
           -I$(LWIP_DIR)/ports/icomm/include \
           -I$(LWIP_DIR)/src/include/ipv4 \
           -I$(LWIP_DIR)/src/include/lwip \
           -I$(LWIP_DIR)/src/include/netif \
           -I$(LWIP_DIR)/ports/icomm/include/arch

INCLUDE_DIR += 	-I$(IPERF_DIR) -I$(NETMGR_DIR) -I$(DHCP_DIR)
######################################################
#
# compile option
#
######################################################

CFLAGS_C        = \
								$(OPTIMAL_FLAG)	\
								-W -Wall \
								$(INCLUDE_DIR) \
								-D$(COMPILER)\
								-Ez
#								-D__GCC_COMPILE__	\
#								-fstrength-reduce	\
#								-fthread-jumps	\
#								-fverbose-asm	\
#								-fmessage-length=0	\
								
                

CFLAGS_S        =  $(INCLUDE_DIR)

CFLAGS_EXTRA	= 
CFLAGS_COMPILE	=

#CFLAGS_ARM	=  -msoft-float -mcpu=arm7tdmi $(CFLAGS_C) $(CFLAGS_EXTRA) $(CFLAGS_COMPILE)
CFLAGS_ARM	=  $(CFLAGS_C) $(CFLAGS_EXTRA) $(CFLAGS_COMPILE)
CFLAGS_ARM_SS = 


######################################################
#
# source code assign
#
######################################################
	
SOURCES =$(wildcard ../src/version.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/src/api/*.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/src/core/*.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/src/core/snmp/*.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/src/core/ipv4/*.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/src/netif/*.c)
#SOURCES +=$(wildcard ../src/tcpip/lwip-1.4.0/ports/icomm/*.c)								 
SOURCES +=$(wildcard ../src/lib/*.c)
SOURCES +=$(wildcard ../src/lib/apps/*.c)
SOURCES +=$(wildcard ../src/core/*.c)
SOURCES +=$(wildcard ../src/drv/*.c)
SOURCES +=$(wildcard ../src/drv/spi/*.c)
SOURCES +=$(wildcard ../src/drv/sdio/*.c)
SOURCES +=$(wildcard ../src/ap/*.c)
SOURCES +=$(wildcard ../src/ap/common/*.c)
SOURCES +=$(wildcard ../src/ap/crypto/*.c)
#SOURCES +=$(wildcard ../src/iperf3.0/src/*.c)
SOURCES +=$(wildcard ../src/netmgr/*.c)
SOURCES +=$(wildcard ../src/netmgr/SmartConfig/*.c)                                   
SOURCES +=$(wildcard ../src/netmgr/SmartConfig/iComm/core/*.c)
SOURCES +=$(wildcard ../src/hal/*.c)
SOURCES +=$(wildcard ../src/hal/SSV6030/*.c)

SOURCES_ASM = $(wildcard ../src/*.s)						
OBJS		= $(addprefix ./,$(SOURCES:.c=.o)) $(addprefix ./,$(SOURCES_ASM:.s=.o))

ASMS		= $(DEPENDS:.d=.s))
DISASMS		= $(DEPENDS:.d=.dis))


.PHONY:		all clean depends  


all:		$(GOAL)
	@echo "all made"

clean:
	@echo "remove all generated files"
	@call del ..\src\*.o
	@call del ..\src\ap\*.o
	@call del ..\src\ap\common\*.o
	@call del ..\src\ap\crypto\*.o
	@call del ..\src\core\*.o
	@call del ..\src\drv\*.o
	@call del ..\src\tcpip\lwip-1.4.0\src\api\*.o
	@call del ..\src\tcpip\lwip-1.4.0\src\core\*.o
	@call del ..\src\tcpip\lwip-1.4.0\src\core\snmp\*.o
	@call del ..\src\tcpip\lwip-1.4.0\src\core\ipv4\*.o
	@call del ..\src\tcpip\lwip-1.4.0\src\netif\*.o
	@call del ..\src\tcpip\lwip-1.4.0\ports\icomm\*.o 
	@call del ..\src\lib\*.o
	@call del ..\src\lib\apps\*.o
	@call del ..\src\netmgr\*.o
	@call del ..\src\udhcp\*.o
	@call del .\*.a

#$(DEPENDS)

depends: $(DEPENDS)
	@echo "dependency made"

$(OUTPUT_LIB_NAME) : $(OBJS)
	$(XAR) -cr $@ $(OBJS)

#ifneq ($(MAKECMDGOALS),clean)
#sinclude	$(DEPENDS)
#endif

#ARMO/%.d : %.c
#	@$(PERL) $(MAKETOOLS_PATH)/wecho.pl  \"generating dependency for $<\"
#	@$(MKDIR) -p $(dir $@)
#	@$(XCC) $(CFLAGS_ARM) -M -o $(MAKETOOLS_PATH)/depends.tmp $<
#	@$(PERL) $(MAKETOOLS_PATH)/makedep.pl $(MAKETOOLS_PATH)/depends.tmp $@


./%.o : %.c
	@call $(MAKETOOLS_PATH)/wecho.exe $< cross-compiling gcc\"
	@$(XCC) $(CFLAGS_ARM) -c -o $@ $<

./%.o : %.S
	@call $(MAKETOOLS_PATH)/wecho.exe $< cross-compiling asm\"
	@$(XAS) $(CFLAGS_ARM_S) $(CFLAGS_S) -o $@ $<



