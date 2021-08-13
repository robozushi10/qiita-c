SRCS = test_driver.c
OBJS = $(SRCS:.c=.o)
MUSTSRC = cfn_invoke.c priv_cfn_invoke.h 

all: $(SRCS)
	gcc -c $(SRCS)
	./mk_sym_tbl.rb $(OBJS)
	gcc -g $(MUSTSRC) $(OBJS)
clean:
	rm -f *.o *~ a.out
