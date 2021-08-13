#include <stdio.h>

void
hoge(int i)
{
    printf("i = %d at hoge()\n", i);
}

void
piyo(char * mesg, char * mesg2)
{
    printf("(%s , %s) at piyo()\n", mesg, mesg2);
}

int
foo(void)
{
    printf("I am foo\n");
    return 99;
}

static void
internal_func(int i, char * mesg)
{
    printf
    (
        "[%s:%d] i = %d, mesg = %s at internal_func[%p]\n",
        __FILE__,
        __LINE__,
        i,
        mesg,
        internal_func
    );
}

void
Ext_func(int i, char * mesg)
{
    printf("call internal_func\n");
    internal_func(i, mesg);
}

int
main()
{
    Invoke_from_stdin();
    /* not reached */
    hoge(0);
    piyo("a","b");
    foo();
    Ext_func(0,"");
    return 0;
}
