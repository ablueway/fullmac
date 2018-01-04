Update Log 
2769
----

Summary
=======
1. Changed struct cfg_host_cmd.
2. Added [-d delay] in ping host command.
3. Added SoC direct memory access host commands "r" and "w".
4. Enabled load bin images.
5. Fixed PC-Lint warnings and errors.
6. Remove l2_packet layer for EAPOL frame handling.
7. Unified host commands dump message and length checking.
8. Added MsgEvent validity check.
9. Added PBUF pointer verification 
10. Added ASSERT_RET to return value in failure condition if
   ASSERT_RET is changed to not holding but returning failure
   value.
Changed details:
================
cli_cmd_soc.c/wsimp_config.c/wsimp_config.h/cli_cmd.c
-----------------------------------------------------
    1. Changed dat in struct cfg_host_cmd. Use HOST_CMD_HDR_LEN instead of 
      sizeof(struct cfg_host_cmd) or sizeof(HDR_HostCmd).
    2. Use dat8, dat16, dat32 for u8, u16, and u32 type respectively.

host/netapp/ping/ping.c
-----------------------
    1. Added [-d delay] for ping host command. The "delay" is in ms unit.

default.cfg/cli_cmd_soc.c
-------------------------
    1. w addr value:
       Write "value to address "addr" in SoC. "addr" and "value" are both 
       in hexdecimal format without "0x". 
       For example, writing 0x00000003 to 0xC0000018 is
       w C0000018 3
    2. r [addr [size]]
       Read address "addr" from SoC with optional "size" of u32 words.
       If size is not given, read one words. If both "addr" and "size"
       are missed, execute last "r" command.
 
cli.h/queue.c/tasks.c/ieee802_11_defs.h/aes-internal-dec.c/aes-internal-enc.c/
md5-internal.c/sha1-internal.c/sha1-pbkdf2.c/sha1.c/driver_cabrio.c/
driver_cabrio.h/l2_packet_rtos.c/wpa.c/wpa.h/wpa_i.h/common.c/common.h/
eloop.h/os_rtos.h/wpa_debug.c/events.c/wpa_supplicant.c/wpa_supplicant.h/
wpas_glue.c/edca.c/edca.h/edca_queue.c/edca_queue.h/mlme.c/mlme.h/
soft_main.c/sta_info.c/sta_info.h/util.c/util.h/pbuf.c/soc_global.c/
bsp.c/cpu.c/cpu.h/irq.c/cli.h/drv_dma.c/drv_dma.h/drv_pkt_dma.c/
drv_edca.c/drv_mac.c/drv_mac.h/drv_mbox.c/drv_nav.c/drv_phy.h/
drv_rtc.c/drv_timer.h/drv_uart.c/FreeRTOSConfig.h/hdr80211.h/rtos.h/
ssv_pktdef.h/ssv_reg.h/wpa_supplicant_i.h/ssv_lib.c/ssv_lib.h/list.h/
queue.h/task.h/timers.h/port.c/portmacro.h/croitine.c/list.c/queue.c/
tasks.c/timers.c/rtos_glue.c/rtos.c
-----------------------------------------------------------------------------
    1. Fixed PC-Lint warnings and errors.

crypto_internals.c
------------------
    1. Not referenced.

wpa_supplicant.c
----------------
    1. Remove l2_packet layer for EAPOL frame handling.

cmd_engine.c
------------
    1. Unified host commands dump message.
    2. Changed dat in struct cfg_host_cmd. Use HOST_CMD_HDR_LEN instead of 
       sizeof(struct cfg_host_cmd) or sizeof(HDR_HostCmd).
    3. Use dat8, dat16, dat32 for u8, u16, and u32 type respectively.

msgevt.c
--------
    1. Added MsgEvent validity check.

pbuf.h
------
    1. Added PBUF pointer verification 

types.h
------
    1. Added ASSERT_RET to return value in failure condition if
       ASSERT_RET is changed to not holding but returning failure
       value.
