#include "mini_exp_std.h"
#include "mini_exp_inst.h"
#include "mini_exp_futex.h"
#include "mini_exp_mem.h"

static int mini_exp_cntl_master_fd(int opt, int * arg);
static int mini_exp_cntl_select_fdw(int opt, int * arg);


static int
mini_exp_cntl_master_fd(int opt, int * arg)
{
    static  int     _master = -1;
    int             retval  = -1;
    switch(opt)
    {
    case    MINI_EXP_SET_MASTER_FD:
        if(_master < 0)
        {
            _master = *arg;
        }
        retval  =   0;
        break;
    case    MINI_EXP_GET_MASTER_FD:
        retval  = _master;
        break;
    default:
        retval  =   -1;
        break;
    }

    return  retval;
}

static int
mini_exp_cntl_select_fdw(int opt, int * arg)
{
    static  int     _fdw_sel = -1;
    int             retval   = -1;

    switch(opt)
    {
    case    MINI_EXP_SET_SELECT_FDW:
        if(_fdw_sel < 0)
        {
            _fdw_sel = *arg;
        }
        retval  =   0;
        break;
    case    MINI_EXP_GET_SELECT_FDW:
        retval  = _fdw_sel;
        break;
    default:
        retval  =   -1;
        break;
    }

    return  retval;
}

int
MINI_EXP_set_master_fd(int * p_master)
{
    return  mini_exp_cntl_master_fd
            (
                MINI_EXP_SET_MASTER_FD,
                p_master
            );
}

int
MINI_EXP_get_master_fd(void)
{
    return mini_exp_cntl_master_fd(MINI_EXP_GET_MASTER_FD, NULL);
}

int
MINI_EXP_set_select_fdw(int * p_fdw)
{
    return  mini_exp_cntl_select_fdw
            (
                MINI_EXP_SET_SELECT_FDW,
                p_fdw
            );
}

int
MINI_EXP_get_select_fdw(void)
{
    return  mini_exp_cntl_select_fdw
            (
                MINI_EXP_GET_SELECT_FDW,
                NULL
            );
}

