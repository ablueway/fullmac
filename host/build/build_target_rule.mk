.PHONY: all clean

$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)
obj-m += $(KMODULE_NAME).o


all:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

#.PHONY: all clean

#$(KMODULE_NAME)-y += $(KERN_SRCS:.c=.o)
#obj-$(CONFIG_SSV6200_CORE) += $(KMODULE_NAME).o


#all:
#	@$(MAKE) -C /lib/modules/$(KVERSION)/build \
#	SUBDIRS=$(KBUILD_DIR) CONFIG_DEBUG_SECTION_MISMATCH=y \
#	modules

#clean:
#	@$(MAKE) -C /lib/modules/$(KVERSION)/build SUBDIRS=$(KBUILD_DIR) clean

