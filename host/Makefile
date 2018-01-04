KBUILD_TOP := $(PWD)
################################################################
# Include Project Feature Control
################################################################
include $(KBUILD_TOP)/build/build_redbull.mk
################################################################
# Include MakeFile Variables
################################################################
include $(KBUILD_TOP)/build/build_global_declare.mk

################################################################
# Include MakeFile Export Variables
################################################################
include $(KBUILD_TOP)/build/build_expose_rule.mk
#export KBUILD_TOP

################################################################
# Include Project Flag Rule 
################################################################
include $(KBUILD_TOP)/build/build_flag_rule.mk
#ccflags-y += -Werror
#ccflags-y += -g
################################################################
# Include Project Source Rule 
################################################################
include $(KBUILD_TOP)/build/build_source_rule.mk

#KERN_SRCS := ./hwif/usb/usb.c
#KERN_SRCS := ./../../test.c
################################################################
# Include Project include Rule 
################################################################
include $(KBUILD_TOP)/build/build_include_rule.mk


################################################################
# Include Makefile target Rule
################################################################
include $(KBUILD_TOP)/build/build_target_rule.mk










