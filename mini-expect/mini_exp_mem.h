#ifndef     __MINI_EXP_MEM_H__
#define     __MINI_EXP_MEM_H__

#include "mini_exp_std.h"
#include "mini_exp_futex.h"
#include "mini_exp_inst.h"
#include "mini_exp_mem.h"

void * MINI_EXP_get_shm_area(char * filename, int require_size);


#endif   /* __MINI_EXP_MEM_H__ */
