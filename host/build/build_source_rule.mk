KERN_SRCS := 

################################################################################
# Define Include AP Source File Rule						   	   		       #	
################################################################################
#KERN_SRCS += ./ap/ap.c
#KERN_SRCS += ./ap/ap_config.c
#KERN_SRCS += ./ap/ap_drv_cmd.c
#KERN_SRCS += ./ap/ap_mlme.c
#KERN_SRCS += ./ap/ap_rx.c
#KERN_SRCS += ./ap/ap_sta_info.c
#KERN_SRCS += ./ap/ap_tx.c
#KERN_SRCS += ./ap/beacon.c
#KERN_SRCS += ./ap/hw_config.c
#KERN_SRCS += ./ap/ieee802_11_ht.c
#KERN_SRCS += ./ap/ieee802_11_mgmt.c
#KERN_SRCS += ./ap/neighbor_ap_list.c
#KERN_SRCS += ./ap/random.c
#KERN_SRCS += ./ap/testcase.c
#KERN_SRCS += ./ap/wpa_auth.c
#KERN_SRCS += ./ap/wpa_auth_ie.c
#KERN_SRCS += ./ap/wpa_debug.c
#KERN_SRCS += ./ap/wmm.c

################################################################################
# Define Include APP Source File Rule						   	   		       #	
################################################################################
KERN_SRCS += ./app/netmgr/net_mgr.c

################################################################################
# Define Include CORE Source File Rule						   	   		       #	
################################################################################
KERN_SRCS += ./core/host_apis.c
KERN_SRCS += ./core/mlme.c
KERN_SRCS += ./core/channel.c
KERN_SRCS += ./core/txrx_hdl.c
KERN_SRCS += ./core/txrx_hdl_tx.c
KERN_SRCS += ./core/txrx_hdl_rx.c
KERN_SRCS += ./core/txrx_task.c
KERN_SRCS += ./core/Regulatory.c
KERN_SRCS += ./core/host_cmd_engine.c
KERN_SRCS += ./core/host_cmd_engine_rx.c
KERN_SRCS += ./core/host_cmd_engine_tx.c
KERN_SRCS += ./core/recover.c
KERN_SRCS += ./core/txrx_task.c
KERN_SRCS += ./core/txrx_task_tx.c
KERN_SRCS += ./core/txrx_task_rx.c

################################################################################
# Define Include HAL Source File Rule						   	   		       #	
################################################################################
KERN_SRCS += ./hal/ssv_hal.c
KERN_SRCS += ./hal/SSV6006/ssv6006_hal.c
KERN_SRCS += ./hal/SSV6006/ssv6006_pkt.c
KERN_SRCS += ./hal/SSV6006/ssv6006_data_flow.c
KERN_SRCS += ./hal/SSV6006/ssv6006_decision_tbl.c
KERN_SRCS += ./hal/SSV6006/ssv6006_beacon.c
KERN_SRCS += ./hal/SSV6006/turismo_common.c
KERN_SRCS += ./hal/SSV6006/ssv6006_efuse.c

################################################################################
# Define Include HIF Wrapper Source File Rule						   	       #	
################################################################################
KERN_SRCS += ./hif_wrapper/ssv_drv.c
KERN_SRCS += ./hif_wrapper/linux/usb/usb.c

################################################################################
# Define Include INIT Source File Rule						   		   	       #	
################################################################################
KERN_SRCS += ./init/init.c
KERN_SRCS += ./init/country_cfg.c
KERN_SRCS += ./init/ssv_frame.c
KERN_SRCS += ./init/version.c
KERN_SRCS += ./init/custom_cfg.c
KERN_SRCS += ./init/macaddr.c

################################################################################
# Define Include LIB Source File Rule						   	   		       #	
################################################################################
KERN_SRCS += ./lib/apps/host_global.c
#KERN_SRCS += ./lib/apps/host_log.c
KERN_SRCS += ./lib/apps/msgevt.c
KERN_SRCS += ./lib/apps/pbuf.c

KERN_SRCS += ./lib/ssv_timer.c
KERN_SRCS += ./lib/ssv_lib.c
KERN_SRCS += ./lib/efuse.c
KERN_SRCS += ./lib/ssv_msg.c

################################################################################
# Define Include NET WRAPPER Source File Rule						   	       #	
################################################################################
KERN_SRCS += ./net_wrapper/linux/net.c

################################################################################
# Define Include OS WRAPPER Source File Rule						   	       #	
################################################################################
KERN_SRCS += ./os_wrapper/linux/os.c

################################################################################
# Define Include Platform Source File Rule						   		       #	
################################################################################
KERN_SRCS += ./platform/linux/porting.c