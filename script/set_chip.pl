 use File::Copy qw(copy);
 use File::Copy qw(move);
  my $work_dir = '.';
  print qx(pwd);  
  
  open(inifile,"$work_dir/script/wifi.ini")or die " $work_dir/script/wifi.ini is not exist\n";
  open(inifileTemp,">$work_dir/script//wifi.ini.bak");
  print "$ARGV[0]";
  
  while($sline = <inifile>)
  {
       print $sline;
       if(($idx=index( $sline,"soc=SSV"))!=-1)
       {
            
            print inifileTemp "soc=SSV$ARGV[0]"; 
       }
       else
       {
          print inifileTemp $sline;
       }
  }
  close(inifileTemp);
  close(inifile);
  print "$work_dir/script/wifi.ini \n";
  $deletfile=   "$work_dir/script/wifi.ini ";
  #unlink $deletfile or warn "Could not unlink $file: $!";;
  unlink  "$work_dir/script/wifi.ini";
  move "$work_dir/script/wifi.ini.bak","$work_dir/script/wifi.ini";
  # qx("mv $work_dir/script/wifi.iniwifi.ini.bak $work_dir/script/wifi.ini ");

