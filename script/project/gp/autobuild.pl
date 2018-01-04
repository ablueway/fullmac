#use Time::localtime;
use File::Copy qw(copy);
use File::Copy qw(move);
  #modify folder and file
  my $work_dir = $ARGV[0] ;
  my $soc_val; 
  my $chip_folder;
  my $tempfolder;
    
  sub modify{     
    print "perl $work_dir\\script\\modify.pl\n" ;
   
    my $dirpath = "$work_dir\\host\\os_wrapper";
    opendir (DIR, $dirpath) or die $!;
    while (my $file = readdir(DIR)) {

        # Use a regular expression to ignore files beginning with a period
        next if ($file =~ m/^\./ || $file=~"MicroC");
        
        system("rmdir/s/q $work_dir\\host\\os_wrapper\\$file");
    }

    closedir(DIR);

    chdir("$work_dir");
    system("perl .\\script\\modify.pl");
    #chdir("$work_dir\\script");
    #system("gp15b_build_q.bat");
    #if(-e "$work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2")
    #{ 
    #    return 1;
    #}
    if($ARGV[1] eq '19')
    {
      print "19\n";
      if( -e "$work_dir\\os\\microc\\Gpl_326xxb.7z") 
      {
        system("perl .\\script\\project\\gp\\gpconfig.pl GP_19B");
        system("c:\\Progra~1\\7-zip\\7z.exe x $work_dir\\os\\microc\\Gpl_326xxb.7z -o$work_dir\\os\\microc\\ ");
        print "$work_dir\\os\\microc\\Gpl_326xxb.7z"  ;
        return 1;
         
       }
       else
       {
           system("rd $compress_src_path /S /Q");
           print   "$work_dir\n" ;
           return 0;
              
       }
    }
    else
    { 
        if( -e "$work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z") 
        {
          system("perl .\\script\\project\\gp\\gpconfig.pl GP_15B");
          system("c:\\Progra~1\\7-zip\\7z.exe x $work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z -o$work_dir\\os\\microc\\ ");
          print "$work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z"  ;
          return 1;
           
         }
         else
         {
             system("rd $compress_src_path /S /Q");
             print   "$work_dir\n" ;
             return 0;
                
         }
    }
    return 1;
  
}



print "$work_dir\n";
if(modify()==0)
{    
    print "modify 0\n"  ;
    return 0;
}

 if($ARGV[1] eq '19')
 {
    system("cmdide.exe $work_dir\\os\\MicroC\\gpl_326xxb\\project\\gpl326xxb_wifi\\gpl326xxb_wifi_release.mcp /r /b /q\"");
    #system("rd $work_dir\\os\\\MicroC\\Gpl_326xxb\\project\\gpl326xxb_wifi\\gpl326xxb_wifi_release_Data\\Release\\ObjectCode /S /Q");
    unlink("$work_dir\\os\\\MicroC\\Gpl_326xxb\\project\\gpl326xxb_wifi\\gpl326xxb_wifi_release_Data\\Release\\ObjectCode");
 }
 else
 {
    system("cmdide.exe $work_dir\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp /r /b /q\"");
    system("rd $work_dir\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr_Data\\Release\\ObjectCode /S /Q");
 }
 return 1;


