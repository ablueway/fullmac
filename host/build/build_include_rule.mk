################################################################################
# 1. Define Project Include Path							   	   			   #	
################################################################################
EXTRA_CFLAGS += -I$(KBUILD_TOP)
EXTRA_CFLAGS += -I$(KBUILD_TOP)/include
EXTRA_CFLAGS += -I$(KBUILD_TOP)/include/smac
EXTRA_CFLAGS += -I$(KBUILD_TOP)/include/priv
EXTRA_CFLAGS += -I$(KBUILD_TOP)/include/priv/hw


EXTRA_CFLAGS += -I$(KBUILD_TOP)/ap
EXTRA_CFLAGS += -I$(KBUILD_TOP)/ap/common
EXTRA_CFLAGS += -I$(KBUILD_TOP)/ap/crypto

EXTRA_CFLAGS += -I$(KBUILD_TOP)/app/cli
EXTRA_CFLAGS += -I$(KBUILD_TOP)/app/cli/cmds
EXTRA_CFLAGS += -I$(KBUILD_TOP)/app/netmgr
EXTRA_CFLAGS += -I$(KBUILD_TOP)/app/netmgr/SmartConfig
EXTRA_CFLAGS += -I$(KBUILD_TOP)/app/netmgr/SmartConfig/iComm/core

EXTRA_CFLAGS += -I$(KBUILD_TOP)/core

EXTRA_CFLAGS += -I$(KBUILD_TOP)/hal
EXTRA_CFLAGS += -I$(KBUILD_TOP)/hal/SSV6006
EXTRA_CFLAGS += -I$(KBUILD_TOP)/hal/SSV6006/firmware/SSV6006C
EXTRA_CFLAGS += -I$(KBUILD_TOP)/hal/SSV6006/regs
EXTRA_CFLAGS += -I$(KBUILD_TOP)/hal/SSV6006/regs/SSV6006C

EXTRA_CFLAGS += -I$(KBUILD_TOP)/hif_wrapper
EXTRA_CFLAGS += -I$(KBUILD_TOP)/hif_wrapper/linux/usb
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/sdio
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/spi
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/usb

#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/sdio
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/spi
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/hwif/usb

EXTRA_CFLAGS += -I$(KBUILD_TOP)/init

EXTRA_CFLAGS += -I$(KBUILD_TOP)/net_wrapper/linux

EXTRA_CFLAGS += -I$(KBUILD_TOP)/os_wrapper/linux

EXTRA_CFLAGS += -I$(KBUILD_TOP)/platform/linux

#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/ports/icomm
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/ports/icomm/include/arch

#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/src/include/ipv4/lwip
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/src/include/ipv6/lwip
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/src/include/lwip
#EXTRA_CFLAGS += -I$(KBUILD_TOP)/tcpip/lwip-1.4.0/src/include/netif

