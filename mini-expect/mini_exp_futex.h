#ifndef     __MINI_EXP_FUTEX_H__
#define     __MINI_EXP_FUTEX_H__

#include "mini_exp_std.h"

extern  unsigned int  * Lock_key;
extern  unsigned int  * Lock_key2;
extern  unsigned int  * Go;
extern  unsigned int  * Go2;

#define MINI_EXP_WAIT_FOR_SETUP(key,val,timeout)        \
{                                                       \
    *Go = true;                                         \
    syscall(SYS_futex, key, FUTEX_WAIT, val, timeout);  \
    *Go = false;                                        \
}

#define MINI_EXP_WAIT_FOR_EXEC(key,val,timeout)         \
{                                                       \
    *Go2= true;                                         \
    syscall(SYS_futex, key, FUTEX_WAIT, val, timeout);  \
    *Go2= false;                                        \
}

#define MINI_EXP_WAIT_ONLY(key,val,timeout)             \
{                                                       \
    *Go = true;                                         \
    syscall(SYS_futex, key, FUTEX_WAIT, val, timeout);  \
}

#define MINI_EXP_WAKE(key,val)                          \
{                                                       \
    while(!(*Go))                                       \
    {                                                   \
        usleep(1*100*1000);                             \
    }                                                   \
    syscall(SYS_futex, key, FUTEX_WAKE, val);           \
}

#define MINI_EXP_WAKE2(key,val)                         \
{                                                       \
    while(!(*Go2))                                      \
    {                                                   \
        usleep(1*100*1000);                             \
    }                                                   \
    syscall(SYS_futex, key, FUTEX_WAKE, val);           \
}

#endif   /* __MINI_EXP_FUTEX_H__ */
