
#include "gplib.h"
#include "console.h"
#include "drv_l1_uart.h"

#define CMD_HISTORIES 4
#define CMD_BUF_SIZE  128

static cmd_t  *pcmds = NULL;
static UINT8  cmdBuffer[CMD_BUF_SIZE];
static UINT32 pos = 0;
static UINT32 logIn = 0;

static UINT32 histBuf[CMD_HISTORIES][CMD_BUF_SIZE];
static UINT32 histPos[CMD_HISTORIES];
static UINT32 histIns;
static UINT32 histOutput;
static UINT32 histInsWrap;
static UINT32 histOutputWrap;

typedef unsigned int size_t;

static void default_cmd_handler(int argc, char *argv[]);

static cmd_t default_cmd_list[] = 
{
	{"all",   default_cmd_handler,  NULL },
	{NULL,    NULL,   NULL}
};

static void default_cmd_help(void)
{
	cmd_t *curr;

	curr = pcmds->pnext;
	if (curr != NULL)
		DBG_PRINT("\r\nUsage:\r\n");
	while ( curr ) 
	{
		DBG_PRINT("    %s help\r\n", curr->cmd);
		curr = curr->pnext;
	}
}

static void default_cmd_handler(int argc, char *argv[])
{
	if (STRCMP(argv[1],"help") == 0)
    {
    	default_cmd_help();
    }
    else
    {
       	default_cmd_help();
    }
}

static void default_cmd_register(void)
{
	cmd_t *pcmd = &default_cmd_list[0];

	while (pcmd->cmd != NULL) 
	{
		cmdRegister(pcmd);
		pcmd += 1;
	}
}

static int console_fgetchar(void)
{
	INT8U input_ch = 0;
	
	while (drv_l1_uart1_data_get(&input_ch, 0) != STATUS_OK)
	{
		OSTimeDly(1);
	}
	return (int )(input_ch);
}

static int isaspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static char *strdup(const char *str)
{
	size_t siz;
	char *copy;
	
	siz = STRLEN(str) + 1;
	if ((copy = (char *)MEMALLOC(siz)) == NULL)
		return NULL;
	MEMCPY(copy, str, siz);
	
	return copy;
}

static int setargs(char *args, char **argv)
{
    int count = 0;
    
    while (isaspace(*args)) ++args;
    while (*args) 
    {
        if (argv) argv[count] = args;
        while (*args && !isaspace(*args)) ++args;
        if (argv && *args) *args++ = '\0';
        while (isaspace(*args)) ++args;
        count++;
    }
    return count;
}

static char **parsedargs(char *args, int *argc)
{
    char **argv = NULL;
    int argn = 0;
    
    if (args && *args)
    {
    	args = (char *)strdup(args);
    	if (args)
    	{
        	argn = setargs(args,NULL);
        	if (argn)
        	{
        		argv = (char **)MEMALLOC((argn+1) * sizeof(char *));
        		if (argv)
    			{
        			*argv++ = args;
        			argn = setargs(args,argv);
    			}
    		}
    	}
	}
    if (args && !argv) FREE(args);
    
    *argc = argn;
    return argv;
 }

static void freeparsedargs(char **argv)
{
    if (argv) 
    {
        FREE(argv[-1]);
        FREE(argv-1);
    } 
}

int parse_to_argv(char *input_str, char ***out_av)
{
    char **av;
    int ac;
    char *as = NULL;
    
    as = input_str;

    av = parsedargs(as,&ac);
	
    #if 0
    int i;
    printf("== %d\r\n",ac);
    for (i = 0; i < ac; i++)
        DBG_PRINT("[%s]\r\n",av[i]);
    #endif
	
    *out_av = av;
    
    return ac;
}

static void cmdProcess(
	UINT8 *cmd,
	UINT32 repeating
)
{
	cmd_t *bc = pcmds;
	UINT32 idx = 0;
	UINT32 copy = 0;
	int argc = 0;
    char **argv = NULL;  
    
	/*
	 * Strip the white space from the command.
	 */
	while ( cmd[idx] != '\0' ) 
	{
		if ( (cmd[idx] != ' ') &&
			 (cmd[idx] != '\t') &&
			 (cmd[idx] != '\r') &&
			 (cmd[idx] != '\n') ) 
		{
			break;
		}
		idx++;
	}

	if ( idx > 0 ) 
	{
		/* Reached a non-white space character, compact the string */
		while ( cmd[idx] != '\0' ) 
		{
			cmd[copy++] = cmd[idx++];
		}
		cmd[copy] = '\0';   
	}

	/*
	 * Index points to the end of the string, move backwards.
	 */
	idx = STRLEN(cmd);

	while ( idx > 0 ) 
	{
		idx--;
		if ( (cmd[idx] == ' ') || 
			 (cmd[idx] == '\t') ||
			 (cmd[idx] == '\r') ||
			 (cmd[idx] == '\n') ) 
		{
			cmd[idx] = '\0';
		}
		else 
		{
			break;
		}
	}

	/*
	 * Find the command.
	 */
	idx = 0;

	while ( cmd[idx] != '\0' ) 
	{
		if ( (cmd[idx] == ' ') ||
			 (cmd[idx] == '\t') ||
			 (cmd[idx] == '\r') ||
			 (cmd[idx] == '\n') ) 
		{
			break;
		}
		idx++;
	}
	
	argc = parse_to_argv((char *)cmd, &argv);
	
	cmd[idx] = '\0';
	
	if (argc > 0)
	{
		while ( bc ) 
		{
			// DBG_PRINT("check %s, %s\r\n", bc->cmd, (argc == 0) ? "" : argv[0]);
			
			if ( STRCMP(bc->cmd, argv[0]) == 0 ) 
			{
				(bc->phandler)(argc, argv);
				return;
			}
			bc = bc->pnext;
		}
	}
	
	DBG_PRINT("command `%s' not found, try `all help'\r\n", (argc == 0) ? "" : argv[0]);
	
	if (argv != NULL)
    {
        freeparsedargs(argv);
        argv = NULL;
    }
	
}

static UINT32 cmdIdxIncrease(
	UINT32 *pcmdIdx
)
{
	UINT32 localIdx;
	UINT32 ret = 0;

	localIdx = *pcmdIdx;
	localIdx++;
	if ( localIdx == CMD_HISTORIES ) 
	{
		localIdx = 0;
		ret = 1;
	}
	*pcmdIdx = localIdx;

	return ret;
}

static UINT32 cmdFlushCopy(
	UINT32 cursorPos,
	UINT8 *pcmdBuf,
	UINT8 *pcmdSrc,
	UINT32 cmdLen
)
{
	if ( cursorPos > 0 ) 
	{
		for ( ; cursorPos > 0; cursorPos-- ) 
		{
			DBG_PRINT("\b \b");
			cmdBuffer[cursorPos] = '\0';
		}
	}
	MEMCPY(pcmdBuf, pcmdSrc, cmdLen);

	return 0;
}

void cmdMonitor(void)
{
	UINT8 c;
	UINT32 repeating;
	UINT32 histDownArw;
	static UINT32 upArrowCnt;

	if ( !logIn ) 
	{
		//if ( c == '\r' ) 
		{
			logIn = TRUE;
			DBG_PRINT("\r\n\r\ncmd>");
		}
	}
	else 
	{
		c = (UINT8 )GETCH();
		
		switch ( c ) 
		{
		case '\b':
		case '\x7f':
			if ( pos > 0 ) 
			{
				DBG_PRINT("\b \b");
				pos--;
			}
			cmdBuffer[pos] = '\0';
			break;

		case '\r':  /* Process the command. */
			DBG_PRINT("\r\n");
			if ( pos ) 
			{
				/*
				 * Do not place the same last command into the history if the same.
				 */
				if ( STRCMP((UINT8 *)histBuf[histIns], cmdBuffer) ) 
				{
					if ( cmdIdxIncrease(&histIns) == 1 ) 
					{
						histInsWrap = 1;
					}
					MEMCPY(histBuf[histIns], cmdBuffer, CMD_BUF_SIZE);
					histPos[histIns] = pos;
				}
				histOutput = histIns;
				histOutputWrap = 0;
				upArrowCnt = 0;
				repeating = FALSE;
			} 
			if ( pos ) 
			{
				cmdProcess(cmdBuffer, repeating);
				pos = 0;
				MEMSET(cmdBuffer, 0, CMD_BUF_SIZE);
				DBG_PRINT("\r\n");
			}
			DBG_PRINT("cmd>");
			break;

		case '[': /* Non ASCII characters, arrow. */
			c = GETCH();
			switch ( c ) 
			{
			case 'A': /* Key: up arrow */
				if ( histOutputWrap == 1 ) 
				{
					if ( histOutput == histIns ) 
					{
						break;
					}
				}
				if ( histInsWrap == 0 ) 
				{
					if ( histOutput == 0 ) 
					{
						break;
					}
				}
				upArrowCnt++;
				cmdFlushCopy(
					pos,
					cmdBuffer,
					(UINT8 *)histBuf[histOutput],
					histPos[histOutput]
				);
				pos = histPos[histOutput];
				cmdBuffer[pos + 1] = '\0';
				DBG_PRINT((CHAR *)cmdBuffer);
				if ( histInsWrap == 1 ) 
				{
					if ( histOutput == 0 ) 
					{
						histOutput = CMD_HISTORIES - 1;
						histOutputWrap = 1;
					}
					else 
					{
						histOutput--;
					}
				}
				else 
				{
					if ( histOutput != 0 ) 
					{
						/* Note that when wrap around does not occur, the least
						 * of index is 1 because it is the first one to be
						 * written.
						 */
						histOutput--;
					}
					/* Nothing to do with histOutput == 0,
					 * because there is no more commands.
					 */
				}
				break;

			case 'B': /* Key: down arrow */
				if ( upArrowCnt <= 1 ) 
				{
					break;
				}
				upArrowCnt--;
				cmdIdxIncrease(&histOutput);
				histDownArw = histOutput;
				cmdIdxIncrease(&histDownArw);
				cmdFlushCopy(
					pos,
					cmdBuffer,
					(UINT8 *)histBuf[histDownArw],
					histPos[histDownArw]
				);
				pos = histPos[histDownArw];
				cmdBuffer[pos + 1] = '\0';
				DBG_PRINT((CHAR *)cmdBuffer);
				break;

			case 'C': /* Key: right arrow */
				break;
			case 'D': /* Key: left arrow */
				break;
			default:
				break;
			}
			break;

		default:
			if ( (pos < (CMD_BUF_SIZE - 1)) && (c >= ' ') && (c <= 'z') ) 
			{
				cmdBuffer[pos++] = c;
				cmdBuffer[pos] = '\0';
				DBG_PRINT((CHAR *)cmdBuffer + pos - 1);
			}
			if ( c == '\x7e' ) 
			{
				cmdBuffer[pos++] = c;
				cmdBuffer[pos] = '\0';
				DBG_PRINT((CHAR *)cmdBuffer + pos - 1);
			}
			break;
		}
	} /* else of if !logged_in */
}

void Cmd_Task(void *para)
{
	default_cmd_register();
	
	while ( 1 ) 
	{
		cmdMonitor();
	}
}


void cmdRegister(cmd_t *bc)
{
	cmd_t *prev;
	cmd_t *curr;

	bc->pnext = NULL;
	if ( pcmds == NULL ) 
	{
		pcmds = bc;
	}
	else 
	{
		prev = NULL;
		curr = pcmds;
		while ( curr ) 
		{
			/* The list is sorted by alphabetic order. */
			if ( STRCMP(bc->cmd, curr->cmd) <= 0 ) 
			{
				bc->pnext = curr;
				if ( prev ) 
				{
					prev->pnext = bc;
				}
				else 
				{
					pcmds = bc;
				}
				return;
			}
			prev = curr;
			curr = curr->pnext;
		}

		/* Last on the list. */

		prev->pnext = bc;
	} /* else boot_commands */
}