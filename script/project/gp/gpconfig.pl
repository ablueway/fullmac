#!/usr/bin/perl
#platform
use File::Copy qw(copy);
use File::Copy qw(move);
my $src_path  = './host/platform/gp/porting.h';
my $des_path  = './host/platform/gp/porting_temp.h';
my $key_str = '#define GP_PLATFORM_SEL';
my $file_change = 0;
sub trim($)
{
  my $string = shift;
  $string =~ s/^\s+//; # ¥hÀY
  $string =~ s/\s+$//; # ¥h§À
  return $string;
}
if(length($ARGV[0])!=0)
{
  open(src,"$src_path");
  open(write_file,">$des_path");
  while($sline = <src>)
  {
     #print $sline;
     if(($idx=index( $sline,$key_str))==-1)
     { 
       print write_file $sline;
     }
     else
     {
        
        $temp = substr $sline,length($key_str),length($sline)-1;
        if(trim($temp) ne $ARGV[0])
        {
          print write_file "$key_str $ARGV[0]\n";
          $file_change = 1;
         
        }
        else
        {
          close(src);
          close(write_file);
          unlink($des_path);
          break;
          
        }
     }
  }
  if($file_change == 1)
  {
     close(src);
     close(write_file);
     unlink($src_path);
     move $des_path,$src_path;
  }         
            
}    