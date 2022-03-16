
CC	=	gcc
TARGET	=	cterp
OBJS	=	main.o \
			chunk.o \
			value.o \
			memory.o \
			vm.o \
			compiler.o \
			scanner.o \
			object.o \
			table.o \
			debug.o

#DTRACE	=	-DDEBUG_TRACE_EXECUTION
DPRINT	=	-DDEBUG_PRINT_CODE
#GPRINT	=	-DDEBUG_STRESS_GC
#GLOG	=	-DDEBUG_LOG_GC
DEBUG	=	-g3
#OPTO	=	-O3
PARAMS	=	-Wall -Wextra $(GPRINT) $(GLOG) $(DTRACE) $(DPRINT) $(OPTO) $(DEBUG)
LIBS	=	-lreadline

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(PARAMS) -o $(TARGET) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(PARAMS) -c $< -o $@

main.o: main.c common.h
chunk.o: chunk.c chunk.h common.h
value.o: value.c value.h common.h
memory.o: memory.c memory.h common.h
debug.o: debug.c debug.h common.h

clean:
	-rm -f $(OBJS) $(TARGET)