$cmd
rmdir /s/q .\os\microc\gp3260xxa_15B_release_v0.0.2
c:\Progra~1\7-zip\7z.exe x .\os\microc\gp3260xxa_15B_release_v0.0.2.7z -o.\os\microc\
subwcrev . > version.txt
perl .\script\modify.pl %1
perl .\script\project\gp\gpconfig.pl GP_15B
cd script/project/gp
gp15b_build.bat 
cd ..\..\..                                                            