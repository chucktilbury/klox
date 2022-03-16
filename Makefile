
PROJDIR	=	$(realpath $(CURDIR))
BINDIR	=	$(PROJDIR)/bin
TMPDIR	=	$(PROJDIR)/tmp
SRCDIR	=	$(PROJDIR)/src
INCDIR	=	$(PROJDIR)/include
make_dirs := $(shell mkdir -p $(TMPDIR) $(BINDIR))

TARGET	=	$(BINDIR)/klox

OBJLST	=	main.o \
			chunk.o \
			debug.o \
			memory.o \
			value.o \
			vm.o \
			compiler.o \
			scanner.o \
			object.o \
			table.o

OBJS 	=	$(foreach item, $(OBJLST), $(addprefix $(TMPDIR)/, $(item)))
SRCS	=	$(foreach item, $(OBJLST:.o=.c), $(addprefix $(SRCDIR)/, $(item)))

#DTRACE	=	-DDEBUG_TRACE_EXECUTION
DPRINT	=	-DDEBUG_PRINT_CODE
#GPRINT	=	-DDEBUG_STRESS_GC
#GLOG	=	-DDEBUG_LOG_GC
DEBUG	=	-g3 -Og
#OPTO	=	-O3
PARAMS	=	-Wall -Wextra $(GPRINT) $(GLOG) $(DTRACE) $(DPRINT) $(OPTO) $(DEBUG)
LIBS	=	-lreadline

COPTS	=	$(PARAMS) -I $(INCDIR)
CC		=	gcc

all: $(TARGET)

$(TMPDIR)%.o: $(SRCDIR)%.c
	$(CC) $(COPTS) -c $< -o $@


$(TARGET): $(OBJS)
	$(CC) $(COPTS) $^ -o $@ $(LIBS)

clean:
	-$(RM) -r $(TMPDIR) $(BINDIR)

ech:
	@echo $(PROJDIR)"\n"$(OBJS)"\n"$(SRCS)"\n"
