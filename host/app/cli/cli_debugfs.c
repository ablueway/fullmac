/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define ICOMM_DEBUG_FS_NAME			"icomm_debug"
#define ICOMM_CMD_CLI_FILE_NAME		"cmd_cli"

#define KER_BUF_LEN 				(200)

char ker_buf[KER_BUF_LEN];
struct dentry *debugfs_root_folder;


#if 0
void cmd_iw(s32 argc, char *argv[])
{
	if (argc<2)
		return;

    if (strcmp(argv[1], "scan")==0) {
        if (argc >= 2)
		    _cmd_wifi_scan(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
	} else if (strcmp(argv[1], "join")==0) {
        if (argc >= 3)
            _cmd_wifi_join(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
    } else if (strcmp(argv[1], "leave")==0) {
        if (argc<4)
            _cmd_wifi_leave(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
    } else if (strcmp(argv[1], "list")==0) {
        if (argc == 2)
        {
            _cmd_wifi_list(argc - 2, &argv[2]);
        }
        else
	        LOG_PRINTF("Invalid arguments.\n");
	}
}
#endif
/* read file operation */
static ssize_t icomm_read(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position)
{
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, KER_BUF_LEN);
}
 
static ssize_t icomm_write(struct file *fp, const char __user *buf, size_t size, loff_t *position)
{

	char info[255];  
//	int port=0,value=0;  
	memset(info, 0x0, 255);  
	copy_from_user(info, buf, size);

	printk("user input: %s\n",info);  

	if (info[0]== 'j') {
		printk("join: cmd\n");	
		_cmd_wifi_join(argc - 2, &argv[2]);
	} 
	else if(info[0] == 'l') {  
		printk("leave: cmd\n");
        _cmd_wifi_leave(argc - 2, &argv[2]);
	}  
	else if(info[0] == 's') 
	{
		printk("scan: cmd\n");
		_cmd_wifi_scan(1, &info[2]);
	}
	
	return size;//simple_write_to_buffer(ker_buf, KER_BUF_LEN, position, user_buffer, count);
}

static const struct file_operations icomm_debugfs_cli_ops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = icomm_read,
	.write = icomm_write,
};


int __init icomm_debugfs_cli_init(void)
{
	debugfs_root_folder = debugfs_create_dir(ICOMM_DEBUG_FS_NAME, NULL);
	if (IS_ERR(debugfs_root_folder)) 
	{
		debugfs_root_folder = NULL;
	}

	debugfs_create_file(ICOMM_CMD_CLI_FILE_NAME, 0644, 
		debugfs_root_folder, NULL, &icomm_debugfs_cli_ops);
	return 0;
}

void __exit icomm_debugfs_cli_exit(void)
{
	if (!debugfs_root_folder) 
	{
		return;
	}
	debugfs_remove_recursive(debugfs_root_folder);
	debugfs_root_folder = NULL;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("aaron.chen <aaron.chen@icomm-semi.com>");
MODULE_DESCRIPTION("Icomm DebugFS functionality");

module_init(icomm_debugfs_cli_init);
module_exit(icomm_debugfs_cli_exit);
