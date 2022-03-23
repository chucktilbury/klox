
PROJDIR	=	$(realpath $(CURDIR))
BINDIR	=	$(PROJDIR)/bin
OBJDIR	=	$(PROJDIR)/obj
SRCDIR	=	$(PROJDIR)/src
DOCDIR	=	$(PROJDIR)/docs
DOCOUTDIR	= $(DOCDIR)/out
INCDIR	=	$(PROJDIR)/include
make_dirs := $(shell mkdir -p $(OBJDIR) $(BINDIR))

TARGET	=	$(BINDIR)/klox
DOCTARG	=	$(DOCOUTDIR)/html/index.html

SLST	=	main.c \
			chunk.c \
			debug.c \
			memory.c \
			value.c \
			vm.c \
			compiler.c \
			scanner.c \
			object.c \
			native.c \
			native_defs.c \
			table.c \
			expressions.c \
			parser.c

HLST	=	common.h \
			chunk.h \
			debug.h \
			memory.h \
			value.h \
			vm.h \
			compiler.h \
			scanner.h \
			object.h \
			native.h \
			native_defs.h \
			table.h \
			expressions.h \
			parser.h


OBJS 	=	$(foreach item, $(SLST:.c=.o), $(addprefix $(OBJDIR)/, $(item)))
SRCS	=	$(foreach item, $(SLST), $(addprefix $(SRCDIR)/, $(item)))
HEADERS	=	$(foreach item, $(HLST), $(addprefix $(SRCDIR)/, $(item)))

# Build configurations
#DTRACE	=	-DDEBUG_TRACE_EXECUTION
DPRINT	=	-DDEBUG_PRINT_CODE
#GPRINT	=	-DDEBUG_STRESS_GC
#GLOG	=	-DDEBUG_LOG_GC
#NANB	=	-DNAN_BOXING
DEBUG	=	-g3 -Og
#OPTO	=	-Ofast
INCS	= 	-I$(INCDIR)
COPTS	=	-Wall -Wextra -std=c99 \
			$(GPRINT) \
			$(GLOG) \
			$(DTRACE) \
			$(DPRINT) \
			$(NANB) \
			$(OPTO) \
			$(INCS) \
			$(DEBUG)
LIBS	=	-lreadline

CC		=	gcc

.PHONY: clean-docs clean distclean docs read format

all: $(TARGET)

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(COPTS) -c $< -o $@


$(TARGET): $(OBJS)
	$(CC) $(COPTS) $^ -o $@ $(LIBS)

clean:
	-$(RM) -r $(OBJDIR) $(BINDIR)

clean-docs:
	-$(RM) -r $(DOCOUTDIR)

distclean: clean clean-docs

$(DOCTARG): $(SRCS) $(HEADERS)
	cd docs; \
	doxygen doxygen.cfg

docs: $(DOCTARG)

read: docs
	firefox $(DOCTARG)

format: $(SRCDIR)/astyle.rc $(SRCS) $(HEADERS)
	cd src; \
	astyle -v --options=astyle.rc $(SRCS) $(HEADERS); \
	mv -vf *.bak ../obj || true