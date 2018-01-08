# not in
ifeq ($(KBUILD_TOP),)
    ifneq ($(KBUILD_EXTMOD),)
    KBUILD_DIR := $(KBUILD_EXTMOD)
    else
    KBUILD_DIR := $(PWD)
    endif
KBUILD_TOP := $(KBUILD_DIR)/../../
endif

# KBUILD_TOP = /home/aaron/svn/host/ssv6xxx
include $(KBUILD_TOP)/config.mak

KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/smac/Module.symvers

# not in
ifeq ($(DRV_OPT), HUW_DRV)
KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/hci_wrapper/Module.symvers
endif

KBUILD_EXTRA_SYMBOLS += $(KBUILD_TOP)/ssvdevice/Module.symvers

KMODULE_NAME=ssv6200_sdio
KERN_SRCS := sdio.c

# KBUILD_TOP = /home/aaron/svn/host/ssv6xxx
include $(KBUILD_TOP)/rules.mak
