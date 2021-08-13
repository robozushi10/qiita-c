PROGRAM=mini_expect

SRCS= mini_exp_main.c       \
	  mini_exp_command.c 	\
	  mini_exp_inst.c 		\
	  mini_exp_mem.c

HDRS=mini_exp_command.h 	\
	 mini_exp_futex.h 		\
	 mini_exp_inst.h 		\
	 mini_exp_mem.h 		\
	 mini_exp_std.h

OBJS=$(SRCS:%.c=%.o)
CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lutil -lpthread
$(PROGRAM):$(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJS) $(LDLIBS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(PROGRAM)

