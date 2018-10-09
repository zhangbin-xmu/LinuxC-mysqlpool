LDFLAGS += -lpthread
LDFLAGS += -lmysqlclient 


SRCS := main.c mysqlpool.c
	
	
OBJS := $(SRCS:%.c=%.o)
	
	
demo : $(OBJS) 
	$(CC) -o demo $(OBJS) $(LDFLAGS)


.PHONY : clean
clean :
	-rm $(OBJS) demo