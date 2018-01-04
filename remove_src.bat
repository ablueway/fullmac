
rd /s/q %~dp0\host\platform\andes
rd /s/q %~dp0\host\platform\fh
rd /s/q %~dp0\host\platform\gm
rd /s/q %~dp0\host\platform\gd
rd /s/q %~dp0\host\platform\ite
rd /s/q %~dp0\host\platform\LinuxSIM
rd /s/q %~dp0\host\platform\mvsilicon
rd /s/q %~dp0\host\platform\ak
rd /s/q %~dp0\host\platform\sample
rd /s/q %~dp0\host\platform\silan
rd /s/q %~dp0\host\platform\st
rd /s/q %~dp0\host\platform\tg

rd /s/q %~dp0\host\drv\ftdi-spi
rd /s/q %~dp0\host\drv\sdio_linux
rd /s/q %~dp0\host\drv\sdio_linux_hci
rd /s/q %~dp0\host\drv\sdio_win32

rd /s/q %~dp0\host\os_wrapper\FreeRTOS
rd /s/q %~dp0\host\os_wrapper\AKOS
rd /s/q %~dp0\host\os_wrapper\Nuttx

del /f/q %~dp0\host\version.c
del /f/q %~dp0\host\main.c
del /f/q %~dp0\host\ethermac.c

rd /s/q %~dp0\host\lib\apps
del /f/q %~dp0\host\lib\*.c
rd /s/q %~dp0\host\core
del /f/q %~dp0\host\hal\*.c
del /f/q %~dp0\host\hal\*.h
rd /s/q %~dp0\host\hal\SSV6030\regs
del /f/q %~dp0\host\hal\SSV6030\*.c
del /f/q %~dp0\host\hal\SSV6030\*.h
del /f/q %~dp0\host\drv\*.c
del /f/q %~dp0\host\drv\sdio\*.c
del /f/q %~dp0\host\drv\spi\*.c

rd /s/q %~dp0\host\ap
rd /s/q %~dp0\host\app\netapp\iperf3.0

::del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp
del /f/q %~dp0\host\app\netmgr\*.c
del /f/q %~dp0\host\app\netmgr\SmartConfig\*.c
del /f/q %~dp0\host\app\netmgr\SmartConfig\iComm\core\*.c

::rd /s/q %~dp0\host\include\priv\hw
rd /s/q %~dp0\host\mac


::rd /s/q %~dp0\host\tcpip\lwip-1.4.0\doc
::rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\api
::rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\core
::rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\netif
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\ports\icomm\*.c
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\CHANGELOG
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\COPYING
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\FILES
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\README
::del /f/q %~dp0\host\tcpip\lwip-1.4.0\UPGRADING


rd /s/q %~dp0\os\LinuxSIM
rd /s/q %~dp0\os\FreeRTOS

rd /s/q %~dp0\script


rd /s/q %~dp0\doc

del /f/q %~dp0\.cproject
del /f/q %~dp0\.gdbinit
del /f/q %~dp0\.project
del /f/q %~dp0\build_gp.bat
del /f/q %~dp0\build_gp.sh
del /f/q %~dp0\build_gp19.bat
del /f/q %~dp0\Makefile
del /f/q %~dp0\Makefile.release
del /f/q %~dp0\readme.txt
del /f/q %~dp0\release.sh




