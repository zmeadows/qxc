# Compiler
CC = gcc

# Project name
PROJECT = qxc

# Directories
OBJDIR = obj
SRCDIR = src

# Libraries
MYCFLAGS = -Wall -Wextra -pedantic -g -std=c11 -fno-omit-frame-pointer
MYLIBS   = -lm

# Files and folders
SRCS    = $(shell find $(SRCDIR) -type f -name '*.c')
HEADERS = $(shell find $(SRCDIR) -type f -name "*.h")
SRCDIRS = $(shell find $(SRCDIR) -type d | sed 's/$(SRCDIR)/./g' )
OBJS    = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Targets
$(PROJECT): buildrepo $(OBJS)
	$(CC) -o $@ $(OBJS) $(MYCFLAGS) $(MYLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ $< -c $(MYCFLAGS)

clean:
	rm $(PROJECT)
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

