#use Time::localtime;
use File::Copy qw(copy);
  #modify folder and file
  my $work_dir ;
  my $soc_val; 
  my $chip_folder;
  my $tempfolder;   
  sub modify{
   
    print "$work_dir\n";
    open(inifile,"$work_dir\\wifi.ini");
    
    my $mac_ini = 'mac=';
    my $config_soc_str='CONFIG_SSV6051Z';
    my $macstr="";                           
    while($sline = <inifile>)
    {
       chomp $sline;
       #print "$sline\n";
       if(($idx=index( $sline,$mac_ini))!=-1)
       { 
          $macstr = substr $sline,$idx+length($mac_ini);
          print "setup mac = $macstr\n";
       }
     
      
    }
    close(inifile);
    
    my $src_path  = "$work_dir\\host\\core\\host_apis.c";
    my $des_path  = "$work_dir\\host\\core\\host_apis_backup.c";
    print $src_path;
    print "\n"; 
    my $src_mac = "config_mac[]";
    open(src,$src_path);
    open(write_file,">$des_path");
    while($sline = <src>)
    {
      
       if(($idx=index( $sline,$src_mac))==-1)
       { 
         print write_file $sline;
       }
       else
       {
          print "$sline";
          chomp($src_mac);
          print write_file "u8 $src_mac =";
          $final = "{ $macstr}; \n";
          print write_file $final;
          #print $src_mac;
          #print $final;
         
       }
    		
    }
    
    close(src);
    close(write_file);
    unlink($src_path);
    rename($des_path,$src_path);
    
    #config.h
    my $define_str='CONFIG_SSV6051Z';
    my $config_src_path  = "$work_dir\\host\\include\\priv\\host_config.h";
    my $config_des_path  = "$work_dir\\host\\include\\priv\\host_config_bak.h";
    open(src,$config_src_path);
    open(write_file,">$config_des_path");
    while($sline = <src>)
    {
     
       if(($idx=index( $sline,$define_str))==-1)
       { 
         print write_file $sline;
       }
       else
       {
          print "$sline";
          print write_file "#define $define_str     $soc_val\n";
          
       }
    }
    
    close(src);
    close(write_file);
    
    unlink($config_src_path);
    rename($config_des_path,$config_src_path);
    print  "$work_dir\\host\\os_wrapper\\LinuxSIM\n";
    if( -e "$work_dir\\host\\os_wrapper\\LinuxSIM")
    {
      system("rmdir/s/q $work_dir\\host\\os_wrapper\\LinuxSIM");
    }
    if(-e "$work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2")
    { 
        return 1;
    }
    else
    { 
        if( -e "$work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z" ) 
        {
          system("c:\\Progra~1\\7-zip\\7z.exe x $work_dir\\os\\microc\\gp3260xxa_15B_release_v0.0.2.7z -o$work_dir\\os\\microc\\ ");
          return 1;
           
         }
         else
         {
             system("rd $compress_src_path /S /Q");
             print   "$work_dir\n" ;
             return 0;
                
         }
    }
    
   
  
}
sub build
{
  if(modify()==0)
  {     
      return 0;
  }
  # print("C:\\Progra~1\\ARM\\ADSv1_2\\Bin\\cmdide.exe $work_dir\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp /b /q\"");
   system("C:\\Progra~1\\ARM\\ADSv1_2\\Bin\\cmdide.exe $work_dir\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp /r /b /q\"");
   return 1;
}

sub process
{
        
        
        $a =index $file, '.7z';
        $s = substr $file,0,$a;;
        $compress_src_path=  "$daily_build_gp_path\\$s";
        if(-d  "$daily_build_gp_path\\$s")
        {
         
        }
        else
        {
           mkdir( "$daily_build_gp_path\\$s");
        }
        
        system("c:\\Progra~1\\7-zip\\7z.exe x $daily_build_path\\$file -o$daily_build_gp_path\\$s");
       
        
        $work_dir = "$daily_build_gp_path\\$s\\$s";
        rename("$work_dir","$work_dir-$chip_folder");
        $work_dir = "$daily_build_gp_path\\$s\\$s-$chip_folder";
        
         
        if(build == 0)
        {
          return 0;
        }
        system("rd $work_dir\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr_Data\\Release\\ObjectCode /S /Q");
        return 1;
}


#$daily_build_path ="x:\\build\\iot-host";
#$compress_des_path ="x:\\build\\iot-host-gp";
$daily_build_path = $ARGV[0];
$compress_des_path =$ARGV[1];  
    
$daily_build_gp_path ="c:\\daily-build-gp";
#$compress_des_path="c:\\temp1";
$compress_src_path="" ;
opendir(DIR,$daily_build_path) or die "Couldn't  open directory $daily_build_path $!";
@files=readdir(DIR);
closedir(DIR);
opendir(GPDIR,$compress_des_path);
@gpfiles=readdir(GPDIR);
closedir(GPDIR);

my $find = 0;

foreach $file(@files)
{
      print "$file\n";
      $find = 0;
       if(-d "$daily_build_path\\$file")
      {
        
         print "$daily_build_path\\$file is directory!!\n"
         
      }
      else
      {
      
        foreach $gpfile(@gpfiles)
        {
          
         
          if($gpfile eq $file)
          {
            $find = 1;
            print "find= $find\n";
            break;
          }
          
        }
        
        if($find == 1)
        {
          
        }
        else
        {
          
          print("=========\n");
          print $gpfile;
          print "\n";
          
          $soc_val=1; 
          $chip_folder ="z";
          if(process()==0)        
          {
            #unlink("$tempfolder");
            print "no zip\n";
          }
          else
          {
            $soc_val=0; 
            $chip_folder ="q";
            
            process();
            
            #mkdir($work_dir);
            sleep 1;
            system("c:\\Progra~1\\7-zip\\7z.exe a $compress_des_path\\$file  $compress_src_path");
            sleep 1;
            system("rd $compress_src_path /S /Q");
            }
          }       
        }
}
