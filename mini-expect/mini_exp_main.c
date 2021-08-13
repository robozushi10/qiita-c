#include    "mini_exp_std.h"
#include    "mini_exp_inst.h"
#include    "mini_exp_futex.h"
#include    "mini_exp_command.h"
#include    "mini_exp_mem.h"

#define MINI_EXP_SHMFILE_KEY    "/dev/shm/mini_exp_key" 
#define MINI_EXP_SHMFILE_KEY2   "/dev/shm/mini_exp_key2"
#define MINI_EXP_SHMFILE_GO     "/dev/shm/mini_exp_go"  
#define MINI_EXP_SHMFILE_GO2    "/dev/shm/mini_exp_go2" 

typedef
struct _thread_arg
{
    int     thread_no;
    void  * data;
}
    thread_arg_t;

unsigned int  * Lock_key    = NULL;
unsigned int  * Lock_key2   = NULL;
unsigned int  * Go          = NULL;
unsigned int  * Go2         = NULL;

static int  mini_exp_initialize(void);
static int  mini_exp_getptymaster(char ** master_name,char * slave_name,struct termios * termp,struct winsize * winp);
static int  mini_exp_setup_stat_of_ptymaster(int master);
static int  mini_exp_getptyslave(char * slave_name);
static void mini_exp_spawn_cmd_child_proc(char * slave_name,int pp1[],int pp2[]);
static void mini_exp_spawn_cmd_parent_proc(int master,int pp1[],int pp2[]);
static void mini_exp_spawn_cmd(int argc,char * argv[0]);
static void mini_exp_setup_stat_of_pipe_for_select(int p1[]);
static void mini_exp_select_task(void * arg);
static void mini_exp_invoke_select(void);
static void mini_exp_expectl(int fd_of_master,int fdw_sel,int v,int v2);


static  int
mini_exp_setup_futex(void)
{
    Lock_key  = MINI_EXP_get_shm_area(MINI_EXP_SHMFILE_KEY , sizeof(size_t));
    Lock_key2 = MINI_EXP_get_shm_area(MINI_EXP_SHMFILE_KEY2, sizeof(size_t));
    Go        = MINI_EXP_get_shm_area(MINI_EXP_SHMFILE_GO  , sizeof(size_t));
    Go2       = MINI_EXP_get_shm_area(MINI_EXP_SHMFILE_GO2 , sizeof(size_t));
    *Go       = 0;
    *Go2      = 0;

    if((!Lock_key) || (!Lock_key2) || (!Go) || (!Go2))
    {
        fprintf(stderr, "ERROR: could not get shm area\n");
        fprintf(stderr, "ERROR: Lock_key (%p)\n", Lock_key);
        fprintf(stderr, "ERROR: Lock_key2(%p)\n", Lock_key2);
        fprintf(stderr, "ERROR: Go (%p)      \n", Go);
        fprintf(stderr, "ERROR: Go2(%p)      \n", Go2);
        return  -1;
    }

    return  0;
}


static int
mini_exp_initialize(void)
{
    int     err = -1;

    signal(SIGPIPE, SIG_IGN);

    err = mini_exp_setup_futex();
    if(err)
    {
        fprintf(stderr, "mini_exp_setup_futex()\n");
        return  err;
    }

    return  0;
}


static int
mini_exp_getptymaster
(
    char            **  master_name,
    char            *   slave_name,
    struct termios  *   termp,
    struct winsize  *   winp
)
{
    int             ret             = -1;
    static int      master          = -1;
    int             slave           = -1;

    master = MINI_EXP_get_master_fd();

    if(master < 0) /* master was not existed */
    {
        ret = openpty(&master, &slave, *master_name, NULL, NULL);
        ASSERT(ret == 0);
        MINI_EXP_set_master_fd(&master);
    }
    else
    {
        /* master was already existed */;
    }

    strcpy(slave_name, ttyname(slave));
    close(slave);

    fcntl(master, F_SETFD, 1);

    return  master;
}


static int
mini_exp_setup_stat_of_ptymaster(int master)
{
    int             cur_stat        = 0;
    int             err             = 0;

    cur_stat      = fcntl(master, F_GETFL);
    cur_stat     |= O_RDWR;
    cur_stat     |= O_NONBLOCK;
    err           = fcntl(master, F_SETFL, cur_stat);

    return  err;
}


static int
mini_exp_getptyslave(char * slave_name)
{
    int     slave   =   -1;

    slave = open(slave_name, O_RDWR);

    /* duplicate 0 onto 1 and 2 to prepare for stty */
    fcntl(0, F_DUPFD, 1); 
    fcntl(0, F_DUPFD, 2);

    /* If you want to save current terminal settings, you will call
     * ttytype(SET_TTYTYPE,slave,ttycopy,ttyinit,stty_args); */
    ioctl(0, TIOCSCTTY, NULL);

    return  slave;
}


void
mini_exp_spawn_cmd_child_proc
(
    char  * slave_name,
    int     pp1[],
    int     pp2[]
)
{
    int     slave       = -1;
    int     rc          = -1;
    int     wc          = -1;
    int     ret         = -1;
    char    sync_byte   = '0';
    int     v           = *Lock_key;
    
    ret = close(pp1[R]); ASSERT(ret != -1);
    ret = close(pp2[W]); ASSERT(ret != -1);
    
    setsid();

    fcntl(2, F_DUPFD, 3);
    
    ret = close(0); ASSERT(ret != -1);
    ret = close(1); ASSERT(ret != -1);
    ret = close(2); ASSERT(ret != -1);
    
    slave = mini_exp_getptyslave(slave_name);
    (void) slave; /* don't use slave (robozushi10) */
    
    signal(1, SIG_IGN);
    /* exp_console_set(); */
    /* for (i = 1; i < NSIG; i++)
       {
           signal(i, ignore[i] ? SIG_IGN : SIG_DFL);
       } */
    
    MINI_EXP_WAIT_FOR_SETUP(Lock_key, v, NULL);

    wc  = write(pp1[W], " ", 1); ASSERT(wc != -1);
    ret = close(pp1[W]);         ASSERT(ret != -1);

    MINI_EXP_WAKE(Lock_key, 1);
    
    while( ((rc = read(pp2[R], &sync_byte, 1)) < 0) && (errno == EINTR) )
    {
        /* empty */;
    }

    ret = close(pp2[R]);
    ASSERT(ret != -1);
}


static void
mini_exp_spawn_cmd_parent_proc(int master, int pp1[], int pp2[])
{
    int     rc  = -1;
    int     wc  = -1;
    int     v   = *Lock_key;
    char    buf[BUFSIZ];
    
    close(pp1[W]); 
    close(pp2[R]);

    fcntl(master, F_SETFD, 1);

    MINI_EXP_WAKE(Lock_key, 1);

    while
    (
        ((rc = read(pp1[R], buf, BUFSIZ)) == -1)
        &&
        (errno == EINTR)
    )
    {
        ; /* empty */
    }
    
    MINI_EXP_WAIT_FOR_SETUP(Lock_key, v, NULL);

    wc = write(pp2[W], " ", 1);
    ASSERT(wc != -1);

    close(pp1[R]);
    close(pp2[W]);
}


static void
mini_exp_spawn_cmd(int argc, char * argv[0])
{
    int             pp1[2];
    int             pp2[2];
    pid_t           pid             = -1;
    static  char  * master_name     = NULL;
    static  char    slave_name[]    = "/dev/ttyXX";
    int             master          = -1;
    int             ret             = -1;
    int             err             = 0;
    int             v               = *Lock_key;
    int             v2              = *Lock_key2;

    (void) v;

    master = mini_exp_getptymaster
             (
                 &master_name,
                 slave_name,
                 NULL,
                 NULL
             );
    
    ret = pipe(pp1); ASSERT(ret != -1);
    ret = pipe(pp2); ASSERT(ret != -1);

    err = mini_exp_setup_stat_of_ptymaster(master);
    if(err)
    {
        fprintf(stderr, "ERROR: ptymaster settings error\n");
        exit(40);
    }

    pid = fork(); ASSERT(pid != -1);

    if(pid > 0)
    {
        /* will do fcntl(master, F_SETFD, 1) */
        mini_exp_spawn_cmd_parent_proc(master, pp1, pp2);
        return;
    }
    else if(pid == 0)
    {
        /* invoke fcntl(0,F_DUPFD,1) and fcntl(0,F_DUPFD,2) */
        mini_exp_spawn_cmd_child_proc(slave_name, pp1, pp2);

        MINI_EXP_WAIT_FOR_EXEC(Lock_key2, v2, NULL);
        argv++;
        execvp(argv[0], argv);
    }
}


static void
mini_exp_setup_stat_of_pipe_for_select(int p1[])
{
    int             cur_stat        = 0;
    int             err             = -1;
    (void) err;

    cur_stat  = fcntl(p1[R], F_GETFL);
    cur_stat |= O_NONBLOCK;
    err       = fcntl(p1[R], F_SETFL, cur_stat);
    ASSERT(err >= 0);

    cur_stat  = fcntl(p1[W], F_GETFL);
    cur_stat |= O_NONBLOCK;
    err       = fcntl(p1[W], F_SETFL, cur_stat);
    ASSERT(err >= 0);
}


static void
mini_exp_select_task(void * arg)
{
    int             p1[2];
    int             ret             = -1;
    int             n               = 0;
    int             v               = 0;
    int             is_poll_master  = 0;
    int             fd_of_master    = -1;
    fd_set          readfds;
    fd_set          writefds;
    fd_set          exceptfds;
    char            buf[BUFSIZ];
    struct  timeval time;
    
    ret       = pipe(p1);
    ASSERT(ret != -1);/* C-R, P-W */
#if 0
    FDW_sel   = p1[W];
#else
    MINI_EXP_set_select_fdw(&p1[W]);
#endif

    mini_exp_setup_stat_of_pipe_for_select(p1);

    static  int  is_first        = true;

    while(1)
    {
        v               =   *Lock_key;
        fd_of_master    =   MINI_EXP_get_master_fd();
        if(is_poll_master)
        {
            for (;;)
            {
                FD_ZERO(&readfds);
                FD_ZERO(&exceptfds);
                FD_SET(p1[0], &readfds);
                FD_SET(fd_of_master , &readfds);
                FD_SET(fd_of_master , &exceptfds);
                time.tv_sec  = 1;
                time.tv_usec = 0;
                if(is_first)
                {
                    MINI_EXP_WAKE2(Lock_key2,1);
                    is_first = false;
                }
                usleep(1*10*1000);  /* ZANTEI */
                n = select(fd_of_master +1, &readfds, &writefds, &exceptfds, NULL);
                if (n > 0)
                {
                    break;
                }
                else if (n == 0)
                { 
                    ; /* timeout */
                }
                else if (n < 0)
                {
                    exit(999);
                }
            } /* end of for(;;) */
        }
        else
        {
            for (;;)
            {
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                FD_ZERO(&exceptfds);
                FD_SET(p1[0], &readfds);
                time.tv_sec  = 1;
                time.tv_usec = 0;
                MINI_EXP_WAKE(Lock_key, 1);

                if(p1[0] > fd_of_master )
                {
                    usleep(1*10*1000); /* ZANTEI */
                    n = select(p1[0]+1, &readfds, &writefds, &exceptfds, NULL);
                }
                else
                {
                    usleep(1*10*1000); /* ZANTEI */
                    n = select(fd_of_master +1, &readfds, &writefds, &exceptfds, NULL);
                }

                if (n > 0)
                {
                    break;
                }
                else if (n == 0)
                { 
                    ; /* timeout */
                }
                else if (n < 0) /* 返り値が負の数なら例外 */
                {
                    exit(250);
                }
            } /* end of for(;;) */
            n = read(p1[0], buf, BUFSIZ); /* none blocking */
        }

        if(*buf == '\0')
        {
            is_poll_master = (is_poll_master == false) ? true : false;
        }
    } /* end of while(1) */
}


static void
mini_exp_invoke_select(void)
{
    pthread_t       handle;
    thread_arg_t    targ;

    pthread_create
    (
        &handle,
        NULL,
        (void *)mini_exp_select_task,
        (void *)&targ
    );
    pthread_detach( handle );
}


static void
mini_exp_expectl(int fd_of_master, int fdw_sel, int v, int v2)
{
    int             rc              = -1;
    int             wc              = -1;
    char            buf[BUFSIZ];

    do
    {
        rc = read(fd_of_master , buf, BUFSIZ);
        if((rc == -1) && (errno != EINTR))
        {
            exit(0);
        }
#if 1
        MINI_EXP_output_mesg(buf, rc);
#else   /* do not input data mode (robozushi10) */
        wc = write(1, buf, rc); ASSERT(wc != -1);
#endif
        MINI_EXP_WAKE(Lock_key, 1);
        wc = write(fdw_sel, "\0", 1);
        ASSERT(wc != -1);
        MINI_EXP_WAIT_ONLY(Lock_key, v, NULL); /* BUGS */
    }
    while(1);
}

int
main(int argc, char * argv[])
{
    int             wc              = -1;
    int             err             = 0;
    int             fd_of_master    = -1;
    int             fdw_sel         = -1;

    // futex の初期化をする
    err = mini_exp_initialize();
    if(err)
    {
        fprintf(stderr, "ERROR: mini_exp_initialize()\n");
        exit(32);
    }

    int             v               = *Lock_key;
    int             v2              = *Lock_key2;

    // 図の「Script」スレッドを起動させる.
    mini_exp_invoke_select();

    // 図の「Script」「bash」間で、図の「Terminal」や「Pseudo pty master」「Pseudo pty slave」
    // への書き込みをタイミングを取るための制御変数 (futex) をセットアップする.
    MINI_EXP_WAIT_FOR_SETUP(Lock_key, v, NULL);

    // 図の「Pseudo terminal master」と「Pseudo terminal slave」を作成する.
    // また、「mini_expect で自動処理させたい実行ファイル」(図の「bash」に相当) を起動させる.
    mini_exp_spawn_cmd(argc, argv);

    // 図の「Script」のファイルディスクリプタを取得する.
    fdw_sel = MINI_EXP_get_select_fdw();
    wc = write(fdw_sel, "\0", 1);
    if(wc == -1)
    {
        perror("write(fdw_sel)");
        exit(59);
    }

    // 図の「Pseudo terminal master」のファイルディスクリプタを取得する.
    fd_of_master =  MINI_EXP_get_master_fd();

    MINI_EXP_WAIT_ONLY(Lock_key, v, NULL);

    // 図の「Script」と「Pseudo terminal master」とのやりとりをする
    mini_exp_expectl(fd_of_master, fdw_sel, v, v2);

    /* not reach (robozushi10) */
    int     status = 0;
    wait(&status);

    return 0;
}

