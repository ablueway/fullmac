use File::Copy qw(copy);
use File::Copy qw(move);
$daily_build_path = $ARGV[0];
$daily_result_path ="d:\\dailyReport";
$daily_report_path = $ARGV[1];
$test_file_path = "";
$test_file ="";
$chip_id ="6030P";

opendir(DIR,$daily_build_path) or die "Couldn't  open directory $daily_build_path $!";
@files=readdir(DIR);
closedir(DIR);

foreach $gpfile(@files)
{				
		if(-d "$daily_build_path\\$gpfile")
		{}
		
		else
		{
					
	    if(-M "$daily_build_path\\$gpfile" <1)
	    { 
	    	
		      $test_7zfile_path = "$daily_build_path\\$gpfile";
		      
		      $test_file =  substr $gpfile,0, length($gpfile)-3;
		      print "$test_file\n";
	    		
	    }
  	}
}

system("c:\\Progra~1\\7-zip\\7z.exe x $test_7zfile_path -oc:\\daily-build-gp"); 
system("mkdir $daily_result_path\\$test_file"."-ch1");
#run sikuli
system("mkdir c:\\test");
$copy_path = "c:\\daily-build-gp\\".$test_file."\\".$chip_id;
#print "xcopy c:\\daily-build-gp\\".$copy_path."\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr_Data\\Release\\gp3260xxa_platform_demo_release_captureRaw_lincorr.axf c:\\test";
system("xcopy ".$copy_path."\\os\\MicroC\\gp3260xxa_15B_release_v0.0.2\\project\\GP3260xxa\\gp3260xxa_platform_demo_release_captureRaw_lincorr_Data\\Release\\gp3260xxa_platform_demo_release_captureRaw_lincorr.axf c:\\test" );
if($ARGV[2] eq "1")
	system("c:\\tools\\sikuli\\runIDE.cmd -r c:\\tools\\autoTest1.sikuli  >ch1.txt");
else if($ARGV[2] eq "6")
  system("c:\\tools\\sikuli\\runIDE.cmd -r c:\\tools\\autoTest6.sikuli  >ch6.txt");
else if($ARGV[2] eq "6")
  system("c:\\tools\\sikuli\\runIDE.cmd -r c:\\tools\\autoTest11.sikuli  >ch11.txt");
  
#delete test file
system("RD c:\\daily-build-gp\\$test_file /S /Q");
system("RD c:\\test /S /Q");

$test_file =  $test_file."-ch$ARGV[2]";
system("mkdir $daily_result_path\\$test_file");
opendir(ResDIR,$daily_result_path) or die "Couldn't  open directory $daily_result_path $!";
@Rfiles=readdir(ResDIR);
closedir(ResDIR);

foreach $resultFile(@Rfiles)
{
		$checkPath = "$daily_result_path\\$resultFile";
    if(-d $checkPath)
    { }
    else
    {
      system("move $checkPath $daily_result_path\\$test_file");
      sleep(10);
    }
}
system("move $daily_result_path\\$test_file v:\\");
