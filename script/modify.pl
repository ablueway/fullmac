#!/usr/local/bin/perl
my $my_top = '.';
use File::Copy qw(copy);
use File::Copy qw(move);
use POSIX qw(strftime);
sub trim($)
{
  my $string = shift;
  $string =~ s/^\s+//; # •h¿Y
  $string =~ s/\s+$//; # •hß¿
  return $string;
}
open(inifile,"$my_top/script/wifi.ini")or die " $my_top/script/wifi.ini is not exist\n";;

my $mac_ini = 'mac=';
my $config_soc_str='CONFIG_SSV6051Z';
my $macstr="";
my $soc_val= '';   
my @mac_arry;
while($sline = <inifile>)
{
   chomp $sline;
   if(($idx=index( $sline,$mac_ini))!=-1)
   { 
      $macstr = substr $sline,$idx+length($mac_ini);
      @mac_arry = split('0x', $sline);
      $cnt=0;
      foreach my $val (@mac_arry[1..6]) 
      {
        $mac_array[$cnt]=substr $val,0,2;
        $cnt++;
      }
   }
  
    if(($idx=index( $sline,"soc=SSV"))!=-1)
    {
    	$soc_val = substr $sline,4,8;
    	print "$soc_val \n";
    }

  
}
close(inifile);

my $set_mac = 0;
my $src_path  = "$my_top/host/init.c";
my $des_path  = "$my_top/host/init.c.bak";
my $src_mac = 'u8 config_mac[] ={ ';
if((length(macstr)!=0))
{ 
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
        chomp($src_mac);
        my @values = split('0x', $sline); 
        $values[6]= substr $values[6],0,2;
        my $cnt=0;
        foreach my $val (@values[1..6]) 
        {
          if($mac_array[$cnt] ne (substr $val,0,2))
          {
            $set_mac = 1;
            break;
          }
          
          $cnt =$cnt+1; 
        }
        if($set_mac == 1)
        {
            print write_file "$src_mac $macstr };\n";
        }
        else
        {
          break;
        }
     }  		 
  }
  
  close(src);
  close(write_file);
  if($set_mac == 1)
  {
    unlink($src_path);
    move $des_path,$src_path;
  }
  else
  {
    unlink($des_path);
  }
}
#config.h
my $define_str='#define CONFIG_CHIP_ID';
my $config_src_path  = "$my_top/host/include/priv/host_config.h";
my $config_des_path  = "$my_top/host/include/priv/host_config_bak.h";
open(src,$config_src_path);
open(write_file,">$config_des_path");
my $set_soc = 1;
while($sline = <src>)
{
 
   if(($idx=index( $sline,$define_str))==-1)
   { 
     print write_file $sline;
   }
   else
   {

      my $soc = substr $sline,length("$define_str"),length($sline);
      print "soc=$soc\n";
      print "soc_val =$soc_val\n";
      if(trim($soc)eq $soc_val)
      {
           $set_soc = 0;
           break ;
      }
      else
      {
         $set_soc = 1;
      }
      print write_file "$define_str     $soc_val\n";
      
   }
}

close(src);
close(write_file);
if($set_soc )
{
  unlink($config_src_path);
  rename($config_des_path,$config_src_path);
}
else
{
  unlink($config_des_path);
}


system("perl $my_top/script/ver_info.pl $my_top/host/version.c $ARGV[0]" );

