#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

void main(unsigned int argc,char *argv[])
{
	char c;
	int debugfs;
	char *usr_argv[argc]={0};

	debugfs = open("/sys/kernel/debug/icomm_debug/cmd_cli",O_WRONLY,S_IRUSR|S_IWUSR);
	printf("argc=%d argv=%s\n",argc,argv[0]);
	
        printf("debug fs handle = %d\n",debugfs);
	write(debugfs,argv[1],80);
}
