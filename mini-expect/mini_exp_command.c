#include "mini_exp_std.h"
#include "mini_exp_inst.h"
#include "mini_exp_futex.h"

#define MINI_EXP_SET_NEXT_CMD_ID    0
#define MINI_EXP_GET_NEXT_CMD_ID    1

typedef
struct  _mini_exp_cmd_t
{
    char    * match;
    char    * cmd;
    int       timeout;
}
    mini_exp_cmd_t;

static int mini_exp_request_next_cmd(char ** match, char ** cmd, int * timeout);
static void mini_exp_pass_through_word(char * str, int len);
static void mini_exp_replace_word(char * match, char * cmd, char * str, int len);

static  int
mini_exp_cntl_cmd_id(int opt)
{
    static  int id  =   0;
    int         ret =   -1;

    switch(opt)
    {
    case    MINI_EXP_SET_NEXT_CMD_ID:
        ++id;
        ret = 0;
        break;
    case    MINI_EXP_GET_NEXT_CMD_ID:
        ret = id;
        break;
    default:
        break;
    }

    return  ret;
}

static int
mini_exp_request_cmd_id(void)
{
    return  mini_exp_cntl_cmd_id(MINI_EXP_GET_NEXT_CMD_ID);
}

static  int
mini_exp_update_cmd_id(void)
{
    mini_exp_cntl_cmd_id(MINI_EXP_SET_NEXT_CMD_ID);
    return  0;
}


static int
mini_exp_request_next_cmd(char ** match, char ** cmd, int * timeout)
{
    static  mini_exp_cmd_t
    mini_exp_cmd_list[] =
    {
#if 0
        {"Name (ftp**.*****.***:robozushi10): ", "********\n", -1},
        {"Password:", "********\n", -1},
#if 0
        {"ftp> ", "cd /piyopiyo\n", -1},
        {"ftp> ", "dir\n", -1},
        {"ftp> ", "dir\n", -1},
        {"ftp> ", "dir\n", -1},
#endif
        {"ftp> ", "bye\n", -1},
#else   
        {"robozushi10@localhost's password: ", "********\n", -1},
        {"robozushi10@hogehoge:~[1]_% ", "pwd\n", -1},
        {"robozushi10@hogehoge:~[2]_% ", "cd /tmp\n", -1},
        {"robozushi10@hogehoge:/tmp[3]_% ", "pwd\n", -1},
        {"robozushi10@hogehoge:/tmp[4]_% ", "cat ~/.cshrc\n", -1},
        {"robozushi10@hogehoge:/tmp[5]_% ", "sleep 3\n", -1},
        {"robozushi10@hogehoge:/tmp[6]_% ", "exit 99\n", -1},
#endif
    };


    int     id  = mini_exp_request_cmd_id();

    if(id < sizeof(mini_exp_cmd_list)/sizeof(mini_exp_cmd_list[0]))
    {
        *match    = mini_exp_cmd_list[id].match;
        *cmd     = mini_exp_cmd_list[id].cmd;
        *timeout = mini_exp_cmd_list[id].timeout;
//      printf("\n[DEBUG] match(%s)\n",*match);
//      printf("[DEBUG] cmd(%s)", *cmd);
        return  0;
    }
    else
    {
        return  1; /* next cmd was not existed */
    }
    return  id;
}

    
static void
mini_exp_pass_through_word(char * str, int len)
{
    int     wc      =   0;

    wc = write(1, str, len);
    ASSERT(wc != -1);
}

static void
mini_exp_replace_word(char * match, char * cmd, char * str, int len)
{
    int     len_of_match     = strlen(match);
    int     len_of_cmd      = strlen(cmd);
    char  * buf             = malloc(len_of_match + 1); 
    int     wc              = 0;
    int     fd_of_master    = MINI_EXP_get_master_fd();
#if 0   /* testing */
    FILE  * fp = fdopen(fd_of_master, "w");
    setbuf(fp, NULL);
#endif  /* testing */

    while(len >= len_of_match)
    {   
        memset(buf, '\0', len_of_match + 1); 
        memcpy(buf, str, len_of_match);
        if(strncmp(buf, match, len_of_match) == 0)
        {
            /* you must do non block mode about master. */
            wc   = write(1, match, len_of_match);
            ASSERT(wc != -1);
            //usleep(1*100*1000);/* sleeping time is necessary (robozushi10) */
            wc   = write(fd_of_master, cmd, len_of_cmd);
            ASSERT(wc != -1);
            //fflush(fp);
            str += len_of_match;
            len -= len_of_match;
            mini_exp_update_cmd_id();
            //usleep(1*100*1000);/* sleeping time is necessary (robozushi10) */
        }
        else
        {
            wc = write(1, buf, 1); 
            ASSERT(wc == 1); 
            str++;
            len--;
        }
    }   

    write(1, str, len);
#if 0   /* testing */
    free(fp);
#endif  /* testing */
    free(buf);
}


void
MINI_EXP_output_mesg(char * str, int len)
{
    int     err     =   0;
    char  * match    = NULL;
    char  * cmd     = NULL;
    int     timeout = -1;

    err = mini_exp_request_next_cmd(&match, &cmd, &timeout);
    if(!err)
    {
        mini_exp_replace_word(match, cmd, str, len);
    }
    else
    {
        mini_exp_pass_through_word(str, len);
    }
}

