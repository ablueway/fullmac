
#ifndef __CONSOLE_H
#define __CONSOLE_H

#define GETCH()      				console_fgetchar()
#define STRCMP(s, t)				gp_strcmp((INT8S *)(s), (INT8S *)(t))
#define STRLEN(s)					gp_strlen((INT8S *)(s))
#define MEMCPY(dest, src, Len) 		gp_memcpy((INT8S *)(dest), (INT8S *)(src), (INT32U)(Len))
#define MEMSET(dest, byte, Len) 	gp_memset((INT8S *)(dest), (INT8U)(byte), (INT32U)(Len))
#define MEMALLOC(siz)				gp_malloc_align((INT32U)(siz), (INT32U)(4))
#define FREE(ptr)					gp_free((void *)(ptr))

typedef void (*cmdHandler)(int argc, char *argv[]);

typedef struct cmd_s 
{
	const char  *cmd;
	cmdHandler   phandler;
	struct cmd_s *pnext;
} cmd_t;

extern void cmdRegister(cmd_t *bc);
extern void Cmd_Task(void *para);

#endif