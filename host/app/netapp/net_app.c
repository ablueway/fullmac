#include <ssv_types.h>
#include <msgevt.h>
#include <log.h>
#include "net_app.h"
#include <os_wrapper.h>

#if NETAPP_SUPPORT

#if (CLI_ENABLE==1)
#define NET_APP_MAX_ARGS            CLI_ARG_SIZE 
#define NET_APP_ARGS_SIZE           (sizeof(char *)*NET_APP_MAX_ARGS)
#define NET_APP_BUF_SIZE            CLI_BUFFER_SIZE
#else
#define NET_APP_MAX_ARGS            20
#define NET_APP_ARGS_SIZE           (sizeof(char *)*NET_APP_MAX_ARGS)
#define NET_APP_BUF_SIZE            80
#endif

#if (PING_SUPPORT  == 1)   
extern void net_app_ping(s32, char **);
#endif
#ifdef __APP_TTCP__
extern void net_app_ttcp(s32, char **);
#endif

/* in */
#if (IPERF3_ENABLE == 1)
extern void net_app_iperf3(s32, char **);
#endif
/* in */
#if (CONFIG_BUS_LOOPBACK_TEST)
extern void bus_loopback(int argc, char **argv);
#endif

static struct task_info_st sg_net_app_task[] =
{
#if NETAPP1_STACK_SIZE   
    { "net_task0",  (OsMsgQ)0, 1, OS_NETAPP_TASK0_PRIO, NETAPP1_STACK_SIZE, NULL, net_app_task    },
#endif
#if NETAPP2_STACK_SIZE   
    { "net_task1",  (OsMsgQ)0, 1, OS_NETAPP_TASK1_PRIO, NETAPP2_STACK_SIZE, NULL, net_app_task    },
#endif
#if NETAPP3_STACK_SIZE
    { "net_task2",  (OsMsgQ)0, 1, OS_NETAPP_TASK2_PRIO, NETAPP3_STACK_SIZE, NULL, net_app_task    },
#endif        
#if NETAPP4_STACK_SIZE
    { "net_task3",  (OsMsgQ)0, 1, OS_NETAPP_TASK3_PRIO, NETAPP4_STACK_SIZE, NULL, net_app_task    },
#endif        
#if NETAPP5_STACK_SIZE
    { "net_task4",  (OsMsgQ)0, 1, OS_NETAPP_TASK4_PRIO, NETAPP4_STACK_SIZE, NULL, net_app_task    },
#endif        

};








extern u32 g_wsimp_flags;


static struct net_app_info_st sg_net_app_info[] =
{
#if PING_SUPPORT    
    { "ping",   net_app_ping ,TRUE},
#endif        
#ifdef __APP_TTCP__ //if you want ttcp, need add it to makefile
    { "ttcp",   net_app_ttcp ,FALSE},
#endif
#if (IPERF3_ENABLE== 1)
    { "iperf3",  net_app_iperf3,FALSE },
#endif
#if (CONFIG_BUS_LOOPBACK_TEST)
    { "bl",  bus_loopback,TRUE },
#endif        
};





void net_app_task(void *args)
{
    s32 argc, res, pid=(s32)args;
    MsgEvent *msg_evt;
    OsMsgQ mbox;
    char **argv;
    struct net_app_args_st *app_args;
    net_app_func app_func_t;

    /* Get app_info of this task */
    app_args = (net_app_args *)sg_net_app_task[pid].args;

    while(1)
    {
        mbox = sg_net_app_task[pid].qevt;
        res = msg_evt_fetch(mbox, &msg_evt);
        ASSERT(res==OS_SUCCESS);

        if (msg_evt->MsgType != MEVT_NET_APP_REQ) {
            msg_evt_free(msg_evt);
            argv[0] = (void *)0;
            continue;
        }
        app_func_t = (net_app_func)msg_evt->MsgData;
        argc = (s32)msg_evt->MsgData1;
        argv = (char **)msg_evt->MsgData2;
        msg_evt_free(msg_evt);
        if (app_func_t == NULL)
            continue;
        app_func_t(argc, argv);

        /**
                * argv[0]  means the task is in the idle state. So once
                * the job has completed, we shall set it to 0 to indicate
                * idle state. net_app_run() checks this value to decide
                * if the task is available or not.
                */
        app_args->app_info = NULL;
        argv[0] = (void *)0;
    }

}

s32 net_app_restrict(int func_idx)
{
    struct net_app_args_st *app_args;
    s32 i,len;

    //No need to restrict task
    if(!sg_net_app_info[func_idx].need_to_restrict)
        return 0;

   /* Restrict task just used one thread*/
    len = sizeof(sg_net_app_task)/sizeof(task_info);
    for(i=0; i<len; i++){
        app_args = (net_app_args *)sg_net_app_task[i].args;
        if (app_args->app_info == NULL)
            continue;

        if (app_args->app_info->app_func == sg_net_app_info[func_idx].app_func)
        {
            LOG_PRINTF("%s is running !!!\r\n",sg_net_app_info[func_idx].app_name);
            return -1;
        }
    }

    return 0;
}


s32 net_app_run(s32 argc, char *argv[])
{
    struct net_app_args_st *app_args;
    MsgEvent *msg_evt;
    char **sargv, *arg_pos;
    s32 i, j, k, len;

    /* Check to see if the net app has been registered */
    len = sizeof(sg_net_app_info)/sizeof(net_app_info);
    for(j=0; j<len; j++) {
        if (strcmp(argv[0],
            sg_net_app_info[j].app_name)==0)
            break;
    }
    if (j >= len)
        return -1;

    /* Restrict task just used one thread*/
    if(net_app_restrict(j)<0)
        return -1;


    /* Allocate an idle net_app task */
    len = sizeof(sg_net_app_task)/sizeof(task_info);
    for(i=0; i<len; i++) {
        app_args = (net_app_args *)sg_net_app_task[i].args;
        sargv = (char **)app_args->cmd_buf;
        arg_pos = (char *)app_args->cmd_buf;
        //if (sargv[0] == NULL)
        if (app_args->app_info == NULL)
            break;
    }
    if (i >= len)
        return -1;

    for(k=0, arg_pos+=NET_APP_ARGS_SIZE; k<argc; k++) {
        len = strlen(argv[k]);
        memcpy(arg_pos, argv[k], len);
        sargv[k] = arg_pos;
        arg_pos += len;
        *arg_pos++ = 0x00;
    }
    app_args->app_info = &sg_net_app_info[j];
    msg_evt = msg_evt_alloc();
    if(NULL!=msg_evt)
    {
        msg_evt->MsgType = MEVT_NET_APP_REQ;
        msg_evt->MsgData  = (u32)sg_net_app_info[j].app_func;
        msg_evt->MsgData1 = (u32)argc;
        msg_evt->MsgData2 = (u32)sargv;
        msg_evt_post(sg_net_app_task[i].qevt, msg_evt);
        return 0;
    }
    else
    {
        LOG_PRINTF("%s:msg alloc fail\r\n",__FUNCTION__);
        return -1;
    }
}



s32 net_app_show(void)
{
    struct net_app_args_st *app_args;
    u32 i;

    LOG_PRINTF("\r\n");
    LOG_PRINTF("  NetAppTask      status      AppName\r\n");
    LOG_PRINTF("  ----------      ------      ---------\r\n");
    for(i=0; i<sizeof(sg_net_app_task)/sizeof(task_info); i++) {
        app_args = (net_app_args *)sg_net_app_task[i].args;
        LOG_PRINTF("  %-10s      %-6s      %-16s\r\n",
            sg_net_app_task[i].task_name,
            ((app_args->app_info==NULL)? "idle": "run"),
            ((app_args->app_info==NULL)? "NULL": app_args->app_info->app_name)
        );
    }
    return 0;
}

void args_alloc_cmdline_buf(struct net_app_args_st *app_args)
{
	/* size(160) = NET_APP_BUF_SIZE(80) + NET_APP_ARGS_SIZE(80) */
	app_args->cmd_buf = (char *)MALLOC((NET_APP_BUF_SIZE + NET_APP_ARGS_SIZE));
	ASSERT(app_args->cmd_buf != NULL);
	ssv6xxx_memset((void *)app_args->cmd_buf, 0, (NET_APP_BUF_SIZE + NET_APP_ARGS_SIZE));
}

void net_app_task_init_args(u8 idx)
{
	/* Allocate memory for net_app_args */
	sg_net_app_task[idx].args = (void *)MALLOC(sizeof(net_app_args));
	ASSERT(sg_net_app_task[idx].args);

	/* Allocate cmdline buffer */
	args_alloc_cmdline_buf(sg_net_app_task[idx].args);

	/* Init app_info */
	((struct net_app_args_st *)sg_net_app_task[idx].args)->app_info = NULL;
}

s32 net_app_init(void)
{
	u8 idx = 0;
	for (idx = 0; idx < sizeof(sg_net_app_task)/sizeof(task_info); idx++) 
	{
		net_app_task_init_args(idx);
		
        if (OS_MsgQCreate(&sg_net_app_task[idx].qevt, 
			(s32)sg_net_app_task[idx].qlength) != OS_SUCCESS)
		{    
			return OS_FAILED;
		}

		OS_TaskCreate(sg_net_app_task[idx].task_func,
					  sg_net_app_task[idx].task_name,
					  sg_net_app_task[idx].stack_size << 4,
										(void *)idx,
					  sg_net_app_task[idx].prio,
										NULL);
    }
    return OS_SUCCESS;
}
#endif
