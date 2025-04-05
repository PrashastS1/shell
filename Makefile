# # directory definitions
# SRCDIR=.
# BUILTINS_SRCDIR=$(SRCDIR)/builtins
# SYMTAB_SRCDIR=$(SRCDIR)/symtab
# BUILD_DIR=$(SRCDIR)/build

# # compiler name and flags
# CC=gcc
# # LIBS=
# LIBS=-lreadline
# CFLAGS=-Wall -Wextra -g -I$(SRCDIR)
# LDFLAGS=-g

# # generate the lists of source and object files
# SRCS_BUILTINS=$(shell find $(SRCDIR)/builtins -name "*.c")

# SRCS_SYMTAB=$(SRCDIR)/symtab/symtab.c

# SRCS=main.c prompt.c node.c parser.c scanner.c source.c executor.c initsh.c  \
#      pattern.c strings.c wordexp.c shunt.c                         \
#      $(SRCS_BUILTINS) $(SRCS_SYMTAB)

# OBJS=$(SRCS:%.c=$(BUILD_DIR)/%.o)

# # output file name
# TARGET=shell

# # default target (when we call make with no arguments)
# .PHONY: all
# all: prep-build $(TARGET)

# prep-build:
# 	mkdir -p $(BUILD_DIR)/builtins
# 	mkdir -p $(BUILD_DIR)/symtab

# $(TARGET): $(OBJS)
# 	 $(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# # target to auto-generate header file dependencies for source files
# depend: .depend

# .depend: $(SRCS)
# 	$(RM) ./.depend
# 	$(CC) $(CFLAGS) -MM $^ > ./.depend;

# include .depend

# #compile C source files
# $(BUILD_DIR)/%.o : %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # clean target
# .PHONY: clean
# clean:
# 	$(RM) $(OBJS) $(TARGET) core .depend
# 	$(RM) -r $(BUILD_DIR)


#
#    Copyright 2020 (c)
#    Mohammed Isam [mohammed_isam1984@yahoo.com]
#
#    file: Makefile
#    This file is part of the "Let's Build a Linux Shell" tutorial.
#    (Modified for Homebrew Readline on macOS)
#
#    This tutorial is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This tutorial is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this tutorial.  If not, see <http://www.gnu.org/licenses/>.
#


# directory definitions
SRCDIR=.
BUILTINS_SRCDIR=$(SRCDIR)/builtins
SYMTAB_SRCDIR=$(SRCDIR)/symtab
BUILD_DIR=$(SRCDIR)/build

# --- Configuration for Homebrew Readline on macOS ---
# Check if brew command exists before running it
HOMEBREW_EXISTS := $(shell command -v brew 2> /dev/null)
ifeq ($(strip $(HOMEBREW_EXISTS)),)
    $(error "Homebrew ('brew' command) not found in PATH. Please install Homebrew or adjust READLINE_PATH manually.")
else
    # Get the prefix for readline installed via Homebrew
    READLINE_PATH := $(shell brew --prefix readline)
    ifeq ($(strip $(READLINE_PATH)),)
        $(error "Readline not found via 'brew --prefix readline'. Please run 'brew install readline'.")
    endif
endif
# --- End Homebrew Configuration ---


# compiler name and flags
CC=gcc
# Include paths: Project source dir AND Homebrew readline include dir
CFLAGS=-Wall -Wextra -g -I$(SRCDIR) -I$(READLINE_PATH)/include
# Linker flags: Debug symbols AND Homebrew readline library path
LDFLAGS=-g -L$(READLINE_PATH)/lib
# Libraries to link: Just readline for now
LIBS=-lreadline
# You might need -lncurses on some systems depending on readline compilation:
# LIBS=-lreadline -lncurses


# generate the lists of source and object files
SRCS_BUILTINS=$(shell find $(BUILTINS_SRCDIR) -maxdepth 1 -name "*.c") # Use -maxdepth 1 if needed
SRCS_SYMTAB=$(shell find $(SYMTAB_SRCDIR) -maxdepth 1 -name "*.c")   # Use find for consistency

SRCS=main.c prompt.c node.c parser.c scanner.c source.c executor.c initsh.c  \
     pattern.c strings.c wordexp.c shunt.c                         \
     $(SRCS_BUILTINS) $(SRCS_SYMTAB)

OBJS=$(SRCS:%.c=$(BUILD_DIR)/%.o)

# output file name
TARGET=shell

# default target (when we call make with no arguments)
.PHONY: all
all: prep-build $(TARGET)

# Create build subdirectories if they don't exist
prep-build:
	@mkdir -p $(BUILD_DIR)/$(BUILTINS_SRCDIR)
	@mkdir -p $(BUILD_DIR)/$(SYMTAB_SRCDIR)

# Linking rule: Combine all object files into the final target executable
# Uses LDFLAGS for library paths and LIBS for library names
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Optional: Rule to auto-generate header file dependencies
# (Can sometimes be complex to get right, especially with subdirs)
# depend: .depend

# .depend: $(SRCS)
#	$(RM) ./.depend
#	$(CC) $(CFLAGS) -MM $^ > ./.depend; # Note: -MM might not handle subdirs well without more logic

# include .depend

# Generic rule to compile C source files into object files in the build directory
# $< is the first prerequisite (the .c file)
# $@ is the target name (the .o file)
# CFLAGS includes the necessary -I flags
$(BUILD_DIR)/%.o : %.c
	@echo "Compiling $< ..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean target: Remove object files, executable, core dumps, dependency file
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	$(RM) $(BUILD_DIR)/*.o $(BUILD_DIR)/$(BUILTINS_SRCDIR)/*.o $(BUILD_DIR)/$(SYMTAB_SRCDIR)/*.o
	$(RM) $(TARGET) core .depend
	$(RM) -r $(BUILD_DIR)