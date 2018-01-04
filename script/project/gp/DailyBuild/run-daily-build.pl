#!/usr/bin/perl
use File::Copy qw(move);
$daily_build_path = $ARGV[0];

#if($ARGV[2] eq '15')
#{
#  $compress_des_path ="$ARGV[1]-gp";
#}
#else
#{
#  $compress_des_path ="$ARGV[1]-gp19b";  
#}
$compress_des_path ="$ARGV[1]";    
$daily_build_gp_path ="c:\\daily-build-gp";
$compress_src_path="" ;
opendir(DIR,$daily_build_path) or die "Couldn't  open directory $daily_build_path $!";
@files=readdir(DIR);
closedir(DIR);
opendir(GPDIR,$compress_des_path);
@gpfiles=readdir(GPDIR);
closedir(GPDIR);

my $find = 0;

sub setChip
{
  open(inifile,"$work_dir\\script\\wifi.ini")or die " $work_dir/script/wifi.ini is not exist\n";
  open(inifileTemp,">$work_dir\\script\\wifi.ini.bak");
  print "$work_dir\\script\\wifi.ini\n";
  while($sline = <inifile>)
  {
       print $sline;
       if(($idx=index( $sline,"soc=SSV"))!=-1)
       {
          #if($chip_folder eq 'z')
          #{
            $temp = uc($chip_folder);
            print inifileTemp "soc=SSV$temp";           
          #}
       }
       else
       {
          print inifileTemp $sline;
       }
  }
  close(inifileTemp);
  close(inifile);
  unlink("$work_dir\\script\\wifi.ini");
  move "$work_dir\\script\\wifi.ini.bak","$work_dir\\script\\wifi.ini";
  
} 

sub process
{
        $a =index $file, '.7z';
        $temp1 = substr $file,0,$a;
        $temp2 = $temp1;
        $temp2 =~ s/iot-host/gp/;
        print $temp2;
        $compress_src_path =  "$daily_build_gp_path\\$temp2";
        if(-d  "$daily_build_gp_path\\$temp2")
        {
        }
        else
        {
           mkdir( "$daily_build_gp_path\\$temp2");
        }
        if(-e "$daily_build_path\\$file")
        {
           print "exist $daily_build_path\\$file "  ;
        }
        else
        {
          print "no $daily_build_path\\$file "  ;
          return 0;
        }
        system("c:\\Progra~1\\7-zip\\7z.exe x $daily_build_path\\$file -o$daily_build_gp_path\\$temp2");
       
        $work_dir = "$daily_build_gp_path\\$temp2\\$temp1";
        rename("$work_dir","$daily_build_gp_path\\$temp2\\$chip_folder");
        print 
        $work_dir = "$daily_build_gp_path\\$temp2\\$chip_folder";
        #system("xcopy autobuild.pl $daily_build_gp_path\\$s\\$s-$chip_folder\\script\\project\\gp\\ /Y" );
        setChip();
        #if(build($work_dir) == 0)
        #{
        #  return 0;
        #}$);
        system("perl $work_dir\\script\\project\\gp\\autobuild.pl $work_dir $ARGV[1]");
        print  "perl $work_dir\\script\\project\\gp\\autobuild.pl $work_dir $ARGV[1]\n";
       
        return 1;
}

print "$compress_des_path\n";

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
      	
      	$tempfile = $file;
        $tempfile =~ s/iot-host/gp/;
        foreach $gpfile(@gpfiles)
        {
          if($gpfile eq $tempfile)
          {
            $find = 1;
            break;
          }
          
        }
        
        if($find == 1)
        {
          
        }
        else
        {
         
          print("=========\n");
          print "  no find\n";
          print("=========\n");
          
          #$soc_val=1; 
          #$chip_folder ="6051z";
          #if(process()==0)        
          #{
          #  print "no zip\n";
          #}
          #else
          #{
            #$soc_val=0; 
            #$chip_folder ="6051q";
            #process();
            
            $chip_folder ="6030p";
            process();
            $file =~ s/iot-host/gp/;
            system("c:\\Progra~1\\7-zip\\7z.exe a $compress_des_path\\$file  $compress_src_path");
            
            system("RD $compress_src_path /S /Q");
          #  }
          } 
              
        }
}
