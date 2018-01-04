rem build version
subwcrev . > version.txt
perl .\script\modify.pl %1

rd /s/q %~dp0\host\platform\ak
rd /s/q %~dp0\host\platform\andes
rd /s/q %~dp0\host\platform\fh
rd /s/q %~dp0\host\platform\gm
rd /s/q %~dp0\host\platform\LinuxSIM
rd /s/q %~dp0\host\platform\mvsilicon

rem decompression

"C:\Program Files\7-Zip\7z.exe" x %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2.7z -o%~dp0\os\MicroC
"C:\Program Files\7-Zip\7z.exe" x %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\tools\make_tools.7z -o%~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\tools

rem create folder

md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\inc
md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src

md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\inc
md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\src

rem md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\inc
rem md %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\src

rem create  Symbolic Links

mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\inc\host %~dp0\host
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\inc\os %~dp0\os

mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\ap %~dp0\host\ap
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\core %~dp0\host\core
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\hal %~dp0\host\hal
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\drv %~dp0\host\drv
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\udhcp %~dp0\host\app\netapp\udhcp
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\lib %~dp0\host\lib
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\netmgr %~dp0\host\app\netmgr
mklink /h %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\init.c %~dp0\host\init.c
mklink /h %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src\version.c %~dp0\host\version.c



mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\inc\host %~dp0\host
mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\inc\os %~dp0\os

mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\src\iperf3.0 %~dp0\host\app\netapp\iperf3.0


rem mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\inc\host %~dp0\host
rem mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\inc\os %~dp0\os

rem mklink /d %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\src\tcpip %~dp0\host\tcpip


rem gpmake init

cd %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\tools\make_tools\

call init.bat

cd project\GP3260xxa

del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini
copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config_task_wifi_lib.ini %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini

gpmake

gpmake task_wifi_lib

del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini
copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config_task_wifi_lib_iperf3.ini %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini


gpmake

gpmake task_wifi_lib_iperf3


del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini
copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config_task_wifi_lib_lwip.ini %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\Config.ini


rem gpmake

rem gpmake task_wifi_lib_lwip

rem end gpmake, start remove .c file


del /f/q/s %~dp0\host\*.o

del /f/q %~dp0\host\lib\task_wifi_lib.a
del /f/q %~dp0\host\lib\task_wifi_lib_iperf3.a
rem del /f/q %~dp0\host\lib\task_wifi_lib_lwip.a

copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\make\task_wifi_lib.a %~dp0\host\lib\task_wifi_lib.a
copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\make\task_wifi_lib_iperf3.a %~dp0\host\lib\task_wifi_lib_iperf3.a
rem copy %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\make\task_wifi_lib_lwip.a %~dp0\host\lib\task_wifi_lib_lwip.a

del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\make\task_wifi_lib.a
del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\make\task_wifi_lib_iperf3.a
rem del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\make\task_wifi_lib_lwip.a

rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\inc
rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib\src

rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\inc
rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_iperf3\src

rem rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\inc
rem rd /s/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\application\task_wifi_lib_lwip\src

del /f/q %~dp0\host\version.c
del /f/q %~dp0\host\main.c
del /f/q %~dp0\host\ethermac.c
rem rd /s/q %~dp0\host\tcpip\lwip-1.4.0\doc
rem rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\api
rem rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\core
rem rd /s/q %~dp0\host\tcpip\lwip-1.4.0\src\netif
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\ports\icomm\*.c
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\CHANGELOG
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\COPYING
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\FILES
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\README
rem del /f/q %~dp0\host\tcpip\lwip-1.4.0\UPGRADING

del /f/q %~dp0\host\lib\*.c
del /f/q %~dp0\host\lib\apps\*.c
del /f/q %~dp0\host\core\*.c
del /f/q %~dp0\host\drv\*.c
del /f/q %~dp0\host\ap\*.c
del /f/q %~dp0\host\ap\common\*.c
rd /s/q %~dp0\host\ap\crypto
del /f/q %~dp0\host\app\netapp\iperf3.0\src\*.c
del /f/q %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp
rem del /f/q %~dp0\host\app\netapp\udhcp\*.c
del /f/q %~dp0\host\app\netmgr\*.c
del /f/q %~dp0\host\app\netmgr\SmartConfig\*.c
del /f/q %~dp0\host\app\netmgr\SmartConfig\iComm\core\*.c
del /f/q %~dp0\host\hal\SSV6030\*.*
del /f/q %~dp0\host\hal\*.*

rd /s/q %~dp0\host\os_wrapper\LinuxSIM
rd /s/q %~dp0\host\os_wrapper\FreeRTOS
rd /s/q %~dp0\host\os_wrapper\AKOS

rd /s/q %~dp0\host\platform\LinuxSIM

rd /s/q %~dp0\host\drv\ftdi-spi
rd /s/q %~dp0\host\drv\sdio_linux
rd /s/q %~dp0\host\drv\sdio_linux_hci
rd /s/q %~dp0\host\drv\sdio_win32

rd /s/q %~dp0\host\ap
rd /s/q %~dp0\host\core
rd /s/q %~dp0\host\include\priv\hw
rd /s/q %~dp0\host\hal\SSV6030\regs

rd /s/q %~dp0\os\LinuxSIM
rd /s/q %~dp0\os\FreeRTOS

rd /s/q %~dp0\script

rd /s/q %~dp0\host\app\netapp\iperf3.0

rd /s/q %~dp0\doc
rd /s/q %~dp0\.svn

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
rem del /f/q %~dp0\ssv6200-uart.bin

rem code warrior for axf

SET PATH=%PATH%;C:\Program Files\ARM\ADSv1_2\Bin
cmdide.exe  %~dp0\os\MicroC\gp3260xxa_15B_release_v0.0.2\project\GP3260xxa\gp3260xxa_platform_demo_release_wifi.mcp /r /b /q"

%~d0
cd %~dp0
subst x: /d

