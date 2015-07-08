CC = clang
TARGET = server
CFLAGS = -Wall -Wextra -g -I.
LFLAGS = -Wall -I. -lm
LINKER = clang -o
SRCDIR = ./src
OBJDIR = ./obj
BINDIR = ./bin
INCDIR = ./inc

.PHONY: clean

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(LINKER) $@ $(LFLAGS) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS)-c $< -o $@

clean:
	rm -f $(OBJECTS)
