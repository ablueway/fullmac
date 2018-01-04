#!/usr/local/bin/perl
use File::Path qw(remove_tree);
use FindBin;
my $folderName = "";
my $desPath = 'z:\\releasebuild\\';
#my $desPath = 'c:\\';
sub buildDeleteCompress
{
  #build q version
  $actionFolder = @_[0];
  chdir("$desPath\\$folderName\\$actionFolder");
  system("gp_lib_release.bat");
  system("for /r ./ %a in (.) do rd /s /q \"%a\\.svn\"");
  system("For /R . %G IN (*.d) do del \"%G\"");
  deleteFile("$desPath\\$folderName\\$actionFolder");
  chdir("$desPath\\$folderName");
  compress("$actionFolder");
}
sub compress
{
  #compress
  $compressFolder = @_[0];
  system("c:\\Progra~1\\7-zip\\7z.exe a $desPath\\$folderName\\$compressFolder.7z   $desPath\\$folderName\\$compressFolder");
  
  remove_tree("$desPath\\$folderName\\$compressFolder");
  if(-e "$desPath\\$folderName\\$compressFolder")
  {
    system("RMDIR $desPath\\$folderName\\$compressFolder");
  }
}
sub deleteFile                                                                 
{
  #delete 
  $currentFolder =@_[0];
 
  opendir(DIR,$currentFolder) or die "Couldn't  open directory $currentFolder!";
  @files=readdir(DIR);
  closedir(DIR);
  foreach $file(@files)
  {
    print "$file\n";
     if(-d "$currentFolder\\$file")
     {
     }
     else
     {
      unlink("$currentFolder\\$file") ;
     }
     
  }
  #delet script
  remove_tree("$currentFolder\\script");

  #delet compress file
  unlink("$currentFolder\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z");
  unlink("$currentFolder\\os\\microc\\Gpl_326xxb.7z");
  
  remove_tree("$currentFolder\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr_Data");
  remove_tree("$currentFolder\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_wifi_Data");
  unlink("$currentFolder\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp");
                                                                                                                              
  #delete other driver interface
  remove_tree("$currentFolder\\host\\drv\\sdio_linux_hci");
  remove_tree("$currentFolder\\host\\drv\\sdio_linux");
  remove_tree("$currentFolder\\host\\drv\\sdio");
  remove_tree("$currentFolder\\host\\drv\\ftdi-spi");
}
#unzip dailybuild
print "$desPath\n";
#system("c:\\Progra~1\\7-zip\\7z.exe x C:\\test\\$ARGV[0] -o$desPath");
system("c:\\Progra~1\\7-zip\\7z.exe x z:\\dailybuild\\iot-host-gp\\$ARGV[0] -o$desPath");
$folderName = substr $ARGV[0],0,length($ARGV[0])-3;




if(-e "$desPath\\$folderName\\$folderName-q\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2")
{
  print "remove \gp3260xxa_15B_release_v0.0.2\n";
  remove_tree("$desPath\\$folderName\\$folderName-q\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2");
}
if(-e "$desPath\\$folderName\\$folderName-z\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2")
{
  remove_tree("$desPath\\$folderName\\$folderName-z\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2");
}

buildDeleteCompress("$folderName-q");
buildDeleteCompress("$folderName-z");



