SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

CFLAGS += -std=c99 -pedantic -Wall

SRC = $(SRCDIR)/riff.c \
	  $(SRCDIR)/demo.c \
	  $(SRCDIR)/wav2pcm.c

INC = $(INCDIR)/riff.h \
	  $(INCDIR)/vector.h

OBJ = $(patsubst $(SRCDIR)%.c, obj%.o, $(SRC))

all: $(BINDIR)/demo $(BINDIR)/wav2pcm

$(BINDIR)/demo: $(OBJDIR)/riff.o $(OBJDIR)/demo.o
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BINDIR)/wav2pcm: $(OBJDIR)/riff.o $(OBJDIR)/wav2pcm.o
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INC)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ -I$(INCDIR) $(CFLAGS)

clean:
	$(RM) -r $(OBJDIR)
	$(RM) -r $(BINDIR)

