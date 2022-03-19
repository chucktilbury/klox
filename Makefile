
PROJDIR	=	$(realpath $(CURDIR))
BINDIR	=	$(PROJDIR)/bin
OBJDIR	=	$(PROJDIR)/obj
SRCDIR	=	$(PROJDIR)/src
INCDIR	=	$(PROJDIR)/include
make_dirs := $(shell mkdir -p $(OBJDIR) $(BINDIR))

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
			native.o \
			native_defs.o \
			table.o

OBJS 	=	$(foreach item, $(OBJLST), $(addprefix $(OBJDIR)/, $(item)))
SRCS	=	$(foreach item, $(OBJLST:.o=.c), $(addprefix $(SRCDIR)/, $(item)))

# Build configurations
#DTRACE	=	-DDEBUG_TRACE_EXECUTION
DPRINT	=	-DDEBUG_PRINT_CODE
#GPRINT	=	-DDEBUG_STRESS_GC
#GLOG	=	-DDEBUG_LOG_GC
DEBUG	=	-g3 -Og
#OPTO	=	-O3
INCS	= 	-I$(INCDIR)
COPTS	=	-Wall -Wextra -std=c99 \
			$(GPRINT) \
			$(GLOG) \
			$(DTRACE) \
			$(DPRINT) \
			$(OPTO) \
			$(INCS) \
			$(DEBUG)
LIBS	=	-lreadline

CC		=	gcc

all: $(TARGET)

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(COPTS) -c $< -o $@


$(TARGET): $(OBJS)
	$(CC) $(COPTS) $^ -o $@ $(LIBS)

clean:
	-$(RM) -r $(OBJDIR) $(BINDIR)

ech:
	@echo $(PROJDIR)"\n"$(OBJS)"\n"$(SRCS)"\n"

format:
	cd src; astyle --options=astyle.rc *.c *.h; mv *.bak ../obj