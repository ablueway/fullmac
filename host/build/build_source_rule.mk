KERN_SRCS := 

################################################################################
# 01. Define Include AP Source File Rule						   	   		   #	
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
# 02. Define Include APP Source File Rule						   	   		   #	
################################################################################
KERN_SRCS += ./app/netmgr/net_mgr.c

################################################################################
# 03. Define Include CORE Source File Rule						   	   		   #	
################################################################################
KERN_SRCS += ./core/host_apis.c
KERN_SRCS += ./core/mlme.c
KERN_SRCS += ./core/channel.c
KERN_SRCS += ./core/txrx_hdl.c
KERN_SRCS += ./core/txrx_task.c
KERN_SRCS += ./core/Regulatory.c
KERN_SRCS += ./core/host_cmd_engine.c
KERN_SRCS += ./core/recover.c
KERN_SRCS += ./core/txrx_task.c
KERN_SRCS += ./core/host_cmd_engine_rx.c
KERN_SRCS += ./core/host_cmd_engine_tx.c

################################################################################
# 04. Define Include DRV Source File Rule						   	   		   #	
################################################################################
KERN_SRCS += ./drv/ssv_drv.c
KERN_SRCS += ./drv/usb_linux/usb_if_impl.c

################################################################################
# 05. Define Include HAL Source File Rule						   	   		   #	
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
# 06. Define Include HWIF Source File Rule						   	   		   #	
################################################################################
KERN_SRCS += ./hwif/usb/usb.c

################################################################################
# 07. Define Include INIT Source File Rule						   		   	   #	
################################################################################
KERN_SRCS += ./init/init.c
KERN_SRCS += ./init/country_cfg.c
KERN_SRCS += ./init/ssv_frame.c
KERN_SRCS += ./init/version.c
KERN_SRCS += ./init/custom_cfg.c
KERN_SRCS += ./init/macaddr.c

################################################################################
# 08. Define Include LIB Source File Rule						   	   		   #	
################################################################################
KERN_SRCS += ./lib/apps/host_global.c
KERN_SRCS += ./lib/apps/host_log.c
KERN_SRCS += ./lib/apps/msgevt.c
KERN_SRCS += ./lib/apps/pbuf.c

KERN_SRCS += ./lib/ssv_timer.c
KERN_SRCS += ./lib/ssv_lib.c
KERN_SRCS += ./lib/efuse.c
KERN_SRCS += ./lib/ssv_msg.c

################################################################################
# 09. Define Include NET STACK WRAPPER Source File Rule						   #	
################################################################################
KERN_SRCS += ./netstack_wrapper/linux/netstack.c

################################################################################
# 10. Define Include OS WRAPPER Source File Rule						   	   #	
################################################################################
KERN_SRCS += ./os_wrapper/Linux/os.c

################################################################################
# 11. Define Include Platform Source File Rule						   		   #	
################################################################################
KERN_SRCS += ./platform/Linux/porting.c

################################################################################
# 12. Define Include TCPIP Source File Rule						   		   	   #	
################################################################################
