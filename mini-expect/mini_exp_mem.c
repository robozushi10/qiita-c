#include "mini_exp_std.h"
#include "mini_exp_mem.h"
#include "mini_exp_futex.h"
#include "mini_exp_inst.h"

void *
MINI_EXP_get_shm_area(char * filename, int require_size)
{
    char          * addr    =   NULL;
    size_t          size    =   0;
    off_t           off     =   0;
    int             fd      =   -1;
    int             wc      =   -1;
    int             ret     =   -1;
    struct  stat    fs;
    char            tmpc    =   '0';
    
    unlink(filename);   /* ignore return value (robozushi10) */

    fd  = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT(fd  != -1);

    wc  = write(fd, &tmpc, require_size);
    ASSERT(wc != -1);

    ret  = fstat(fd, &fs);
    ASSERT(ret != -1);

    size = fs.st_size;

    addr = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
    ASSERT((int)*addr != -1);

    ret  = close(fd);
    ASSERT(ret != -1);
    
    return  addr;
}

