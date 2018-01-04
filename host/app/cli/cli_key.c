/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


//#ifdef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 1)
#include <termios.h>
#include <unistd.h>
#include <assert.h>

#include <ssv_ex_lib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <host_config.h>
#include <log.h>
 
 
#if (CLI_ENABLE==1)
 
#undef TERMIOSECHO
 
#define TERMIOSFLUSH
 
/* Changed __linux__ to __SSV_UNIX_SIM__ for CYGWIN compatibility */
struct termios backup_term;
char first = 0;


/*
 * kbhit()  a keyboard lookahead monitor
 *
 * returns the number of characters available to read
 */
int kbhit ( void )
{
    struct timeval tv;
    struct termios old_termios, new_termios;
    int            error;
    int            count = 0;
 
    tcgetattr( 0, &old_termios );
    new_termios              = old_termios;
    /*
     * raw mode
     */
    new_termios.c_lflag     &= ~ICANON;
    /*
     * disable echoing the char as it is typed
     */
    new_termios.c_lflag     &= ~ECHO;
    /*
     * minimum chars to wait for
     */
    new_termios.c_cc[VMIN]   = 1;
    /*
     * minimum wait time, 1 * 0.10s
     */
    new_termios.c_cc[VTIME]  = 1;
    error                    = tcsetattr( 0, TCSANOW, &new_termios );
    tv.tv_sec                = 0;
    tv.tv_usec               = 100;
    /*
     * insert a minimal delay
     */
    select( 1, NULL, NULL, NULL, &tv );
    #ifdef __CYGWIN__
    error                   += ioctl( 0, TIOCINQ, &count );
    #else
    error                   += ioctl( 0, FIONREAD, &count );
    #endif
    error                   += tcsetattr( 0, TCSANOW, &old_termios );    
    return( error == 0 ? count : -1 );
}  /* end of kbhit */
/**/
int getch( void ) 

{
      int c = 0;
 
      struct termios org_opts, new_opts;
      int res = 0;
 
     
      assert( res == 0 );

      if(!first){
        //  store old settings 
        res = tcgetattr( STDIN_FILENO, &org_opts );
     
        memcpy( &backup_term, &org_opts, sizeof(new_opts) );


            //- set new terminal parms 
        memcpy( &new_opts, &org_opts, sizeof(new_opts) );
            
#ifndef ECHOPRT
        new_opts.c_lflag &= ~( ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOKE | ICRNL );
#else
        new_opts.c_lflag &= ~( ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL );
#endif
        tcsetattr( STDIN_FILENO, TCSANOW, &new_opts );
        
        first = 1;
      }
 
      
      c = getchar();
 
          //  restore old settings 
      //res = tcsetattr( STDIN_FILENO, TCSANOW, &org_opts );
      //assert( res == 0 );
      return( c );
}

int reset_term(void)
{
//   struct timeval tv;
   tcsetattr( STDIN_FILENO, TCSANOW, &backup_term );
   return 1;
}
int get_term(void)
{
  tcgetattr( STDIN_FILENO, &backup_term );
}
#endif
#endif



