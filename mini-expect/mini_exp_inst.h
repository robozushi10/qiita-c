#ifndef     __MINI_EXP_INST_H__
#define     __MINI_EXP_INST_H__

#include "mini_exp_std.h"
#include "mini_exp_futex.h"

#define     MINI_EXP_SET_MASTER_FD      0
#define     MINI_EXP_GET_MASTER_FD      1

#define     MINI_EXP_SET_SELECT_FDW     0
#define     MINI_EXP_GET_SELECT_FDW     1

int MINI_EXP_set_master_fd(int * p_master);
int MINI_EXP_get_master_fd(void);
int MINI_EXP_set_select_fdw(int * p_fdw);
int MINI_EXP_get_select_fdw(void);

#endif   /* __MINI_EXP_INST_H__ */

