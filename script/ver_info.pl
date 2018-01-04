#!/usr/bin/perl
use strict;


my $svn_status;
my $svn_filename;
my $svn_version;
my $sline;

my $svn_root = ".";
my $svn_root_version;
my $version_file = "$svn_root/version.txt";


sub get_version_linux {
    foreach (@_) {
        if($_ =~ m/^Last Changed Rev: (\d*)/) {
            return $1;
        }
    } 
    # file doesn't exist on svn
    return -1;
}


sub get_version_win {
   open(infile,"$version_file");
	 print $version_file;
   printf "\n\r";
    #foreach (@_) {
   while($sline = <infile>)
   { 		print $sline;
        if($sline =~ m/^Last committed at revision (\d*)/) {
        	 close(infile);
           print $1;
           return $1;
        }
    } 
    # file doesn't exist on svn
    close(infile);
    return -1;
}


printf("## script to generate version infomation header ##\n");

# step-0: get root svn number 

if($^O eq "linux")
{
 
  $svn_root_version = get_version_linux(qx(svn info $svn_root)); 
  
}
else
{  
   #$svn_root_version = get_version_win(qx("subwcrev $svn_root"));
   $svn_root_version = get_version_win();
}

if($svn_root_version == -1) 
{
    print "aa\n";
    if(-e "$version_file")
    {
      if(open(verfile,$version_file))
      {
        while($sline = <verfile>)
        {
             $svn_root_version = $sline;
             chomp $svn_root_version;
        }
        close(verfile);        
      }
    }
}
OUTPUT_HEADER:
# step-3: output header files
if (defined($ARGV[0])) {
    open HEADER, ">", $ARGV[0];
    select HEADER;
}
else {
    print "Error! Please specify source file\n";
}


printf("const char *ssv_version = { \"%s%d\"};\n\n",$ARGV[1], $svn_root_version);



use POSIX qw(strftime);
my $date = strftime "%Y/%m/%d/ %H:%M", localtime;
printf("const char *ssv_date 	= { \"$date\" };\n\n");

printf("const char *rlsversion 	= { \"6030.P1.1044\" };\n");
##

