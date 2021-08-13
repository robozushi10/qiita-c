#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "priv_cfn_invoke.h"

int Invoke_from_stdin(void);
inline static lexer_state_t * access_lexer_state(void);
inline static void set_lexer_state(int new_state);
inline static int get_lexer_state(void);
inline static int get_lexer_prev_state(void);
static void save_arg_list(int * argc,ainfo_t * alist,char * word,e_atype type);
static void delete_arg_list(int * argc,ainfo_t * alist);
static void parser_start(void);
static e_token do_lexer(char * word,e_atype * type,int mode);
static void dbg_dump_args(int argc,ainfo_t * alist);
static void * conv_sym_2_addr(char * symbol);
static void dym_cast_args(int argc,ainfo_t * alist,void * dst[MAXARG]);
static int invoke(int argc,ainfo_t * alist);
static int invoke_by_argc(int (*func)(), void * al[MAXARG], int argc);

/* 外部提供I/F */
int
Invoke_from_stdin(void)
{
    parser_start();
    return 0;
}

inline
static lexer_state_t *
access_lexer_state(void)
{
    static lexer_state_t state =
                        {
                            .prev = NEUTRAL,
                            .cur  = NEUTRAL
                        };
    return &state;
}

/* lexer の状態を更新する */
inline
static void
set_lexer_state(int new_state)
{
    lexer_state_t * state = access_lexer_state();
    int             tmp   = NEUTRAL;

    tmp         = state->cur;
    state->cur  = new_state;
    state->prev = tmp;
}

/* 現在の lexer の状態を取り出す */
inline
static int
get_lexer_state(void)
{
    lexer_state_t * state = access_lexer_state();
    return state->cur; 
}

/* 一つ以前の lexer の状態を取り出す */
inline
static int
get_lexer_prev_state(void)
{
    lexer_state_t * state = access_lexer_state();
    return state->prev; 
}

/* 入力された引数情報を保存しておく */
static void
save_arg_list(int * argc, ainfo_t * alist, char * word, e_atype type)
{
    if((alist[*argc].sym = malloc(strlen(word) + 1)) == NULL)
    {
        WARN("out of arg memory [word=%s]\n", word);
        exit(44);
    }
    strcpy(alist[*argc].sym, word);
    alist[*argc].type = type;
    (*argc)++;
}

/* 入力された引数情報を削除する */
static void
delete_arg_list(int * argc, ainfo_t * alist)
{
    int i = 0;

    for(i = 0; i < *argc; i++)
    {
        free(alist[i].sym);
        alist[i].sym  = NULL;
        alist[i].type = TYPE_atype_unknown;
    }

    *argc = 0;
}

/* パーサスタート */
static void
parser_start(void)
{
    enum      { OK, NG };
    e_token   token  = TOK_token_unknown;
    e_atype   type   = TYPE_atype_unknown;
    int       retval = 0;
    int       argc   = 0;
    ainfo_t   alist[MAXARG+1];
    int       mode   = NORMAL_MODE;
    char      word[MAXWORD];

    /* ignoresig(); */

    while(1)
    {
        switch(token = do_lexer(word, &type, mode)) /* lexer(字句解析器)コール */
        {
        case TOK_WORD:  /* 単語の区切り */
            if(argc < MAXARG)
            {
                save_arg_list(&argc, alist, word, type);
            }
            else
            {
                WARN("Too many args\n");
                delete_arg_list(&argc, alist);
                mode = ABORT_MODE;
                break;
            }
            continue;
        case TOK_NL:    /* 改行 */
            if((argc == 0) && (token != TOK_NL))
            {
                WARN("Illegal sequence\n");
            }
            else if(argc > 0)
            {
                retval = invoke(argc, &alist[0]);
            }
            delete_arg_list(&argc, &alist[0]);
            continue;
        case TOK_BP:    /* 括弧 ) */
            if(argc == 0)
            {
                WARN("Illegal sequence\n");
                delete_arg_list(&argc, alist);
                mode = ABORT_MODE;
                break;
            }
            else
            {
                save_arg_list(&argc, &alist[0], word, type);
            }
            continue;
        case TOK_ERR:   /* トークン不正 */
            delete_arg_list(&argc, &alist[0]);
            mode = ABORT_MODE;
            break;
        case TOK_EOL:   /* トークンの終点 */
        default:
            delete_arg_list(&argc, &alist[0]);
            mode = NORMAL_MODE;
            break;
        }
        usleep(5 * 1000 * 100);
    }
}

/* 字句解析器の実行 */
static e_token 
do_lexer(char * word, e_atype * type, int mode)
{
    enum    { FALSE , TRUE };
    int     c          = 0;
    char  * w          = word;
    int     state      = NEUTRAL;
    int     prev_state = NEUTRAL;
    u8      in_arg     = FALSE; /* 引数ポイント中かどうか */
    u8      in_arg_top = TRUE;  /* 各引数の先頭をポイント中かどうか */

    while((c = getchar()) != EOF)
    {
        if(mode == NORMAL_MODE)
        {
            prev_state   = get_lexer_prev_state();

            switch(state = get_lexer_state())
            {
                case NEUTRAL: /* 初期状態 */
                    if(isalpha(c))
                    {
                        *w++  = c; 
                        set_lexer_state(INSYMBOL);
                        continue;
                    }
                    else if(c == '\n')
                    {
                        set_lexer_state(NEUTRAL);
                        return TOK_NL;
                    }
                    else if(isspace(c))
                    {
                        continue;
                    }
                    else
                    {
                        WARN("lexer error (c=%c)\n", c);
                        set_lexer_state(NEUTRAL);
                        return TOK_ERR;
                    }
                    set_lexer_state(NEUTRAL);
                    return TOK_ERR; /* not reached */
                case INSQUOTE: /* シングルクオート解析中(非サポート) */
                    continue; 
                case INDQUOTE: /* ダブルクオート解析中 */
                    switch(c)
                    {
                        case '"':
                            *type  = TYPE_8;
                            in_arg = FALSE;
                            set_lexer_state(INPAREN);
                            continue;
                        case '\n':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_EOL;
                        default:
                            *w++   = c;
                            in_arg = TRUE;
                            continue;
                    }
                    set_lexer_state(NEUTRAL); /* not reach */
                    return TOK_ERR;
                case INPAREN:   /* ( 内にいる場合 */
                    switch(c)
                    {
                        case '\n':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_EOL;
                        case '(': /* ネストは非サポート */ /* サポートする場合は，ネスト数を数えれば良いはず */
                            WARN("Syntax error\n");
                            in_arg = FALSE;
                            set_lexer_state(NEUTRAL);
                            return TOK_ERR;
                        case '"':
                            if(in_arg_top)
                            {
                                in_arg = TRUE;
                                set_lexer_state(INDQUOTE);
                                continue;
                            }
                            else
                            {
                                WARN("Syntax error\n");
                                set_lexer_state(NEUTRAL);
                                return TOK_ERR;
                            }
                        case ' ':
                        case '\t':
                            continue;
                        case ',':
                            if(in_arg)
                            {
                                *w         = '\0';
                                in_arg_top = TRUE;
                                set_lexer_state(INSRCHARG);
                                return TOK_WORD;
                            }
                            else if((!in_arg) && (prev_state == INDQUOTE))
                            {
                                *w         = '\0';
                                in_arg_top = TRUE;
                                set_lexer_state(INSRCHARG);
                                return TOK_WORD;
                            }
                            else
                            {
                                WARN("Syntax error\n");
                                set_lexer_state(NEUTRAL);
                                return TOK_ERR;
                            }
                        case ')':
                            *w = '\0';
                            set_lexer_state(NEUTRAL);
                            if((!in_arg) && (prev_state == INDQUOTE))
                            {
                                return TOK_WORD;
                            }
                            return (in_arg) ? TOK_WORD : TOK_NL;
                        default:
                            *w++       = c;
                            *type      = TYPE_32;
                            in_arg     = TRUE;
                            in_arg_top = FALSE;
                            continue;
                    }
                case INSYMBOL: /* 関数名(symbol)解析中 */
                    switch(c)
                    {
                        case '\n':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_EOL;
                        case ' ': case '\t':
                        case '(':
                            *w = '\0';
                            set_lexer_state(INPAREN);
                            return TOK_WORD;
                        case ')':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_ERR;
                        default:
                            *w++ = c;
                            continue;
                    }
                case INSRCHARG: /* 引数情報を解析中 */
                    switch(c)
                    {
                        case '\n':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_EOL;
                        case ',':
                        case ')':
                            WARN("Syntax error\n");
                            set_lexer_state(NEUTRAL);
                            return TOK_ERR;
                        case '"':
							set_lexer_state(INDQUOTE);
                            in_arg = TRUE;
                            continue;
                        case ' ':
                        case '\t':
                            continue;
                        default:
                            *w++   = c;
                            *type  = TYPE_32;
                            in_arg = TRUE;
							set_lexer_state(INPAREN);
                            continue;
                    }
            }
        }
        else if(mode == ABORT_MODE) /* 不正があった場合は，入力文字をすべて破棄する */
        {
            if(c == '\n')
            {
                set_lexer_state(NEUTRAL);
                break;
            }
        }
    }

    return TOK_EOL;
}


/* デバッグ関数. 引数情報の表示 */
static void
dbg_dump_args(int argc, ainfo_t * alist)
{
    int i = 0;

    for(i = 0; i < argc; i++)
    {
        if(alist[i].type == TYPE_8)
        {
            printf
            (
                "DEBUG: alist[%d] = \"%s\"\n",
                i, 
                ((char *)alist[i].sym) ? ((char *)alist[i].sym) : ""
            );
        }
        else
        {
            printf("DEBUG: alist[%d] = %s\n", i, (char *)alist[i].sym);
        }
    }
}

/* シンボル(関数)名からアドレスを取得する */
static void *
conv_sym_2_addr(char * symbol)
{
    int    i         = 0;
    int    (*func)() = NULL;

    for(i = 0; i < KEYNUM(g_fn_tbl); i++)
    {
        if(STRCMP(symbol, g_fn_tbl[i].sym))
        {
            func = g_fn_tbl[i].addr;
        }
    }

    return func;
}

/* 引数(alist)の型変換を行い，alに格納する */
static void
dym_cast_args(int argc, ainfo_t * alist, void * dst[MAXARG])
{
    int    i         = 0;

    for(i = 1; i < argc; i++)
    {
        dst[i] = (alist[i].type == TYPE_8)  ? (u8   *)alist[i].sym       :
                 (alist[i].type == TYPE_16) ? (u16  *)atoi(alist[i].sym) :
                 (alist[i].type == TYPE_32) ? (u32  *)atoi(alist[i].sym) :
                                              (void *)alist[i].sym;
    }
}


/*
 * 関数ポインタと関数の引数を取得し，引数の個数に応じた
 * 関数をコールする
 */
static int
invoke(int argc, ainfo_t * alist) 
{
    enum   {not_found = -1};
    int    ret       = 0;
    int    (*func)() = NULL;
    void * al[MAXARG];

    memset(al, 0, sizeof(void *) * MAXARG);

    dbg_dump_args(argc, alist); /* debug */

    /* シンボル(関数)名からアドレスを取得する */
    func = conv_sym_2_addr(alist[0].sym);
    if(func == NULL)
    {
        WARN("not found %s\n", alist[0].sym);
        return not_found;
    }

    /* 引数(alist)の型変換を行い，alに格納する */
    dym_cast_args (argc, alist, al);

    /* 引数の個数別に応じた関数をコールする */
    ret = invoke_by_argc(func, al, argc);

    return ret;
}

/* 引数の個数に応じた関数呼び出しを行う */
static int
invoke_by_argc(int (*func)(), void * al[MAXARG], int argc)
{
    int     ret = 0;

    switch(argc-1)
    {
        case 0:
            ret = func();
            break;
        case 1:
            ret = func(al[1]);
            break;
        case 2:
            ret = func(al[1], al[2]);
            break;
        case 3:
            ret = func(al[1], al[2], al[3]);
            break;
        case 4:
            ret = func(al[1], al[2], al[3], al[4]);
            break;
        case 5:
            ret = func(al[1], al[2], al[3], al[4], al[5]); 
            break;
        case 6:
            ret = func(al[1], al[2], al[3], al[4], al[5], al[6]); 
            break;
        case 7:
            ret = func(al[1], al[2], al[3], al[4], al[5], al[6], al[7]); 
            break;
        case 8:
            ret = func(al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8]); 
            break;
        default:
            WARN("too many arguments...\n");
            ret = -1;
            break;
    }

    return ret;
}

