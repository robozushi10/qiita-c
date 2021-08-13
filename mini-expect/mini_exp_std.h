#ifndef     __MINI_EXP_STD_H__
#define     __MINI_EXP_STD_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>    
#include <termio.h>  /* for struct winsize */
#include <pty.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#ifdef  DEBUG
#define ASSERT  assert
#else
#define ASSERT  (void)
#endif

#define R               0
#define W               1

#endif   /* __MINI_EXP_STD_H__ */
