CC = clang
TARGET = server
CFLAGS = -Wall -Wextra -g -I$(INCDIR)
LFLAGS = -lpthread 
LINKER = clang -o
SRCDIR = src
OBJDIR = obj
BINDIR = bin
INCDIR = inc

.PHONY: clean

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET): $(OBJECTS) 
	mkdir -p $(BINDIR)
	$(CC) $(LFLAGS) -o $@ $(OBJECTS) 


$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)	
	$(CC) -c $(CFLAGS) $< -o $@
	cscope -Rbq
	ctags -R


clean:
	rm -rf $(OBJECTS) $(OBJDIR) $(BINDIR)/$(TARGET) $(BINDIR)

	
