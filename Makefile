LDFLAGS += -lpthread


SRCS := main.c threadpool.c
	
	
OBJS := $(SRCS:%.c=%.o)
	
	
demo : $(OBJS) 
	$(CC) -o demo $(OBJS) $(LDFLAGS)


.PHONY : clean
clean :
	-rm $(OBJS) demo