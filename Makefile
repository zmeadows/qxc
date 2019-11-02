# Compiler
CC = g++

# Project name
PROJECT = qxc

# Directories
OBJDIR = obj
SRCDIR = src

# Libraries
MYCFLAGS = -std=c++17 \
		   -Wall \
		   -Wextra \
		   -Wconversion \
		   -Werror \
		   -Wshadow \
		   -Wpointer-arith \
		   -Wcast-qual \
		   -Wfloat-equal \
		   -Wunreachable-code \
		   -Wswitch-default \
		   -Winline \
		   -fno-omit-frame-pointer \
		   -fstrict-aliasing \
		   -lstdc++ \
		   -pedantic \
		   -g \

MYLIBS   = -lm

CPPCHECK = cppcheck

# Files and folders
SRCS    = $(shell find $(SRCDIR) -type f -name '*.cpp')
HEADERS = $(shell find $(SRCDIR) -type f -name "*.h")
SRCDIRS = $(shell find $(SRCDIR) -type d | sed 's/$(SRCDIR)/./g' )
OBJS    = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Targets
$(PROJECT): buildrepo $(OBJS)
	$(CC) -o $@ $(OBJS) $(MYCFLAGS) $(MYLIBS)
	@$(call call-cppcheck)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/darray.h
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
	cppcheck --suppress=missingIncludeSystem --enable=warning,performance,portability,information,missingInclude --language=c++ --std=c++11 -q --inconclusive -I $(SRCDIR) $(SRCDIR)/*.cpp $(SRCDIR)/*.h
endef
