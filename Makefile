# RedBull Kenrel Module TOP Directory


# KBUILD_TOP = /home/aaron/svn/host/ssv6xxx
KBUILD_TOP := $(PWD)
export KBUILD_TOP


KERNEL_MODULES += host


# KERNEL_MODULES = ssvdevice hci smac hwif/sdio hwif/usb
all: 
	@for dir in $(KERNEL_MODULES) ; do \
	    echo "############################################"; \
	    echo "# Building Kernel Module: $$dir.ko" ; \
	    echo "############################################"; \
	    $(MAKE) -C $$dir KBUILD_DIR=$(PWD)/$$dir ; \
	    echo ""; \
	    echo ""; \
	done 

clean:
	@for dir in $(KERNEL_MODULES) ; do \
	    $(MAKE) -C $$dir clean KBUILD_DIR=$(PWD)/$$dir ; \
	done
