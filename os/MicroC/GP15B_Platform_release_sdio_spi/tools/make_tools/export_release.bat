if "%1" == "" goto ERROR

echo Copy application
xcopy "..\..\application\inc" 									"%1\application\inc" /s /i /y
xcopy "..\..\application\msg_manager" 					"%1\application\msg_manager" /s /i /y
xcopy "..\..\application\multimedia_platform" 	"%1\application\multimedia_platform" /s /i /y
xcopy "..\..\application\task_audio_dac" 				"%1\application\task_audio_dac" /s /i /y
xcopy "..\..\application\task_audio_decoder" 		"%1\application\task_audio_decoder" /s /i /y
xcopy "..\..\application\task_audio_record" 		"%1\application\task_audio_record" /s /i /y
xcopy "..\..\application\task_avi_encoder" 			"%1\application\task_avi_encoder" /s /i /y
xcopy "..\..\application\task_filesrv" 					"%1\application\task_filesrv" /s /i /y
xcopy "..\..\application\task_image" 						"%1\application\task_image" /s /i /y
xcopy "..\..\application\task_video_decoder" 		"%1\application\task_video_decoder" /s /i /y
xcopy "..\..\application\task_video_encoder" 		"%1\application\task_video_encoder" /s /i /y
xcopy "..\..\application\usbd" 									"%1\application\usbd" /s /i /y

echo Copy board_config
xcopy "..\..\board_config" 											"%1\board_config" /s /i /y

echo Copy driver1
xcopy "..\..\driver_l1\cdsp" 										"%1\driver_l1\cdsp" /s /i /y
xcopy "..\..\driver_l1\common" 									"%1\driver_l1\common" /s /i /y
xcopy "..\..\driver_l1\inc" 										"%1\driver_l1\inc" /s /i /y
xcopy "..\..\driver_l1\jpeg\inc" 								"%1\driver_l1\jpeg\inc" /s /i /y
xcopy "..\..\driver_l1\mipi" 										"%1\driver_l1\mipi" /s /i /y
xcopy "..\..\driver_l1\scaler\inc" 							"%1\driver_l1\scaler\inc" /s /i /y
xcopy "..\..\driver_l1\sdc\inc" 								"%1\driver_l1\sdc\inc" /s /i /y
xcopy "..\..\driver_l1\spu\inc" 								"%1\driver_l1\spu\inc" /s /i /y
xcopy "..\..\driver_l1\usbd\inc" 								"%1\driver_l1\usbd\inc" /s /i /y


echo Copy driver_l2
xcopy "..\..\driver_l2\ad_key" 									"%1\driver_l2\ad_key" /s /i /y
xcopy "..\..\driver_l2\cdsp" 										"%1\driver_l2\cdsp" /s /i /y
xcopy "..\..\driver_l2\display" 								"%1\driver_l2\display" /s /i /y
xcopy "..\..\driver_l2\inc" 										"%1\driver_l2\inc" /s /i /y
xcopy "..\..\driver_l2\nand_flash" 							"%1\driver_l2\nand_flash" /s /i /y
xcopy "..\..\driver_l2\scaler" 									"%1\driver_l2\scaler" /s /i /y
xcopy "..\..\driver_l2\sccb" 										"%1\driver_l2\sccb" /s /i /y
xcopy "..\..\driver_l2\sd\inc" 									"%1\driver_l2\sd\inc" /s /i /y
xcopy "..\..\driver_l2\sensor" 									"%1\driver_l2\sensor" /s /i /y
xcopy "..\..\driver_l2\spi_flash" 							"%1\driver_l2\spi_flash" /s /i /y
xcopy "..\..\driver_l2\src" 										"%1\driver_l2\src" /s /i /y
xcopy "..\..\driver_l2\system" 									"%1\driver_l2\system" /s /i /y
xcopy "..\..\driver_l2\usbd\inc" 								"%1\driver_l2\usbd\inc" /s /i /y
xcopy "..\..\driver_l2\usbd\user" 							"%1\driver_l2\usbd\user" /s /i /y

echo Copy gplib
xcopy "..\..\gplib\bmp\inc" 										"%1\gplib\bmp\inc" /s /i /y
xcopy "..\..\gplib\calendar" 										"%1\gplib\calendar" /s /i /y

xcopy "..\..\gplib\fs\driver" 									"%1\gplib\fs\driver" /s /i /y
xcopy "..\..\gplib\fs\include" 									"%1\gplib\fs\include" /s /i /y
xcopy "..\..\gplib\fs\nls" 											"%1\gplib\fs\nls" /s /i /y
copy "..\..\gplib\fs\fs.h" 											"%1\gplib\fs\fs.h" 
copy "..\..\gplib\fs\fs_get_version.c" 					"%1\gplib\fs\fs_get_version.c" 
copy "..\..\gplib\fs\fs_os.c" 									"%1\gplib\fs\fs_os.c" 
copy "..\..\gplib\fs\fsglobalvar.c" 						"%1\gplib\fs\fsglobalvar.c" 
copy "..\..\gplib\fs\swap_byte.c" 							"%1\gplib\fs\swap_byte.c" 
copy "..\..\gplib\fs\vfs.h" 										"%1\gplib\fs\vfs.h" 

xcopy "..\..\gplib\gif\inc" 										"%1\gplib\gif\inc" /s /i /y
xcopy "..\..\gplib\inc" 												"%1\gplib\inc" /s /i /y
xcopy "..\..\gplib\jpeg\inc" 										"%1\gplib\jpeg\inc" /s /i /y
xcopy "..\..\gplib\liba" 												"%1\gplib\liba" /s /i /y
xcopy "..\..\gplib\mm" 													"%1\gplib\mm" /s /i /y
xcopy "..\..\gplib\print_string" 								"%1\gplib\print_string" /s /i /y
xcopy "..\..\gplib\src" 												"%1\gplib\src" /s /i /y

echo Copy gpstdlib
xcopy "..\..\gpstdlib" 													"%1\gpstdlib" /s /i /y

echo Copy liba
xcopy "..\..\liba" 															"%1\liba" /s /i /y

echo Copy os
xcopy "..\..\os\inc" 														"%1\os\inc" /s /i /y

echo Copy project
xcopy "..\..\project\GP326033" 													"%1\project\GP326033" /s /i /y
del "%1\project\GP326033\GP326033_platform_demo.mcp"


echo Copy tools
xcopy "..\..\tools"															"%1\tools" /s /i /y

goto OK
:ERROR
echo Using %0 [dest_dir] to copy to destination dir
:OK