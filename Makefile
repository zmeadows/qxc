# Compiler
CC = clang

# Project name
PROJECT = qxc

# Directories
OBJDIR = obj
SRCDIR = src

# Libraries
MYCFLAGS = -Wall \
		   -Wextra \
		   -Wconversion \
		   -Werror \
		   -Wstrict-prototypes \
		   -Wshadow \
		   -Wpointer-arith \
		   -Wcast-qual \
		   -Wmissing-prototypes \
		   -Wfloat-equal \
		   -Wunreachable-code \
		   -Wswitch-default \
		   -Winline \
		   -fno-omit-frame-pointer \
		   -fstrict-aliasing \
		   -pedantic \
		   -D_DEFAULT_SOURCE \
		   -g \
		   -Og \
		   -std=c11 \

MYLIBS   = -lm

CPPCHECK = cppcheck

# Files and folders
SRCS    = $(shell find $(SRCDIR) -type f -name '*.c')
HEADERS = $(shell find $(SRCDIR) -type f -name "*.h")
SRCDIRS = $(shell find $(SRCDIR) -type d | sed 's/$(SRCDIR)/./g' )
OBJS    = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Targets
$(PROJECT): buildrepo $(OBJS)
	$(CC) -o $@ $(OBJS) $(MYCFLAGS) $(MYLIBS)
	@$(call call-cppcheck)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ $< -c $(MYCFLAGS)

clean:
	rm -rf $(PROJECT)
	rm -rf $(OBJDIR)

buildrepo:
	@$(call make-repo)

# Create obj directory structure
define make-repo
	mkdir -p $(OBJDIR)
	for dir in $(SRCDIRS); \
	do \
		mkdir -p $(OBJDIR)/$$dir; \
	done
endef

define call-cppcheck
	cppcheck --suppress=missingIncludeSystem --enable=warning,performance,portability,information,missingInclude --std=c11 -q --inconclusive --std=posix  -I $(SRCDIR) $(SRCDIR)/*.c $(SRCDIR)/*.h
endef
