#ifndef  __CFN_INTERP_H__
#define  __CFN_INTERP_H__
/*=========*/
/* include */
/*=========*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

/*========*/
/* define */
/*========*/
#define MAXARG      8
#define MAXWORD     4096
#define KEYNUM(a)   (sizeof(a)/sizeof(*a))

#define STRCMP(a,b)  \
        ((strlen(a) == strlen(b)) && (strcmp(a, b) == 0))
    

#define WARN(fmt, args...)              \
{                                       \
    fprintf                             \
    (                                   \
        stderr,                         \
        "[%s:%d] "                      \
        fmt,                            \
        __FILE__,                       \
        __LINE__,                       \
        ## args                         \
    );                                  \
}


/*=========*/
/* typedef */
/*=========*/
typedef          char       s8;
typedef          short      s16;
typedef          int        s32;
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;

/*======*/
/* enum */
/*======*/
/* トークンの種類 */
typedef enum 
{
    TOK_ERR           = -1,     /* 構文規則エラー */
    TOK_WORD          =  0,     /* 単語           */
    TOK_FP            =  1,     /* (              */
    TOK_BP            =  2,     /* )              */
    TOK_NL            =  3,     /* 改行           */
    TOK_EOL           =  4,     /* EOF            */
    TOK_token_unknown = 0x7fffffff
}
    e_token;

/* lexer の位置(状態) */
enum lexer_state
{
    INVALID    = -1,
    NEUTRAL    =  0,
    INSQUOTE   =  1,  /* シングルクオート中 */
    INDQUOTE   =  2,  /* ダブルクオート中   */
    INPAREN    =  3,  /* () カッコ中        */
    INSYMBOL   =  4,  /* 関数名(symbol)中   */
    INSRCHARG  =  5,  /* 引数をスキャナ中   */
    IN_Unknown = 0x7fffffff
};

/* argument の型 */
typedef
enum _e_atype
{
    TYPE_8             = 0,  /* x86の場合. char      */
    TYPE_16            = 1,  /* x86の場合. short     */
    TYPE_32            = 2,  /* x86の場合. int, long */
    TYPE_atype_unknown = 0x7fffffff
}
    e_atype;

/* スキャナーモード(入力文字の通常，破棄) */
enum  _lexer_mode
{
    NORMAL_MODE = 0,
    ABORT_MODE  = 1
};

/*========*/
/* struct */
/*========*/
/* 関数テーブル管理構造体 */
typedef
struct _fn_tbl_t
{
    char * sym;     /* シンボル名 */
    void * addr;    /* アドレス   */
}
    fn_tbl_t;

/* 引数管理構造体 */
typedef
struct _ainfo_t
{
    char * sym;     /* 引数名            */
    int    type;    /* 引数の型. e_atype */
}
    ainfo_t;

/* スキャナーの状態 */
typedef
struct _lexer_state_t
{
    int prev;
    int cur;
}
    lexer_state_t;

/*========*/
/* extern */
/*========*/
#include "cfn.extern.h"

/*==========*/
/* function */
/*==========*/

static fn_tbl_t
g_fn_tbl[] =
{
#include "cfn.tbl.h"
};

#endif /* __CFN_INTERP_H__ */
