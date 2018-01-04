$cmd
rmdir /s/q .\os\microc\Gpl_326xxb
c:\Progra~1\7-zip\7z.exe x .\os\microc\Gpl_326xxb.7z -o.\os\microc\
subwcrev . > version.txt
perl .\script\modify.pl %1
perl .\script\project\gp\gpconfig.pl GP_19B
cd script/project/gp
gp19b_build.bat 
cd ..\..\..                                                            