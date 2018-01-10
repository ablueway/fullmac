/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/module.h>


#define ICOMM_DEBUG_FS_NAME			"icomm_debug"
#define ICOMM_CMD_CLI_FILE_NAME		"cmd_cli"

#define KER_BUF_LEN 				(200)

char ker_buf[KER_BUF_LEN];
struct dentry *debugfs_root_folder;


/* read file operation */
static ssize_t icomm_read(struct file *fp, char __user *user_buffer,
                                size_t count, loff_t *position)
{
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, KER_BUF_LEN);
}
 
static ssize_t icomm_write(struct file *fp, const char __user *user_buffer,
                                size_t count, loff_t *position)
{
		if (count > KER_BUF_LEN) 
		{
		    return -EINVAL;
		}  
        return simple_write_to_buffer(ker_buf, KER_BUF_LEN, position, user_buffer, count);
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
