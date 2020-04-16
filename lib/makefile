# Makefile used to create the version of the FSE library we need for squeeze
# Otherwise, the source files have been removed

CFLAGS = -g -O3 -c
CC = gcc

SRCDIR = .
OBJDIR = ./obj
INCLUDES = .

HEADERS = \
$(SRCDIR)/error_public.h \
$(SRCDIR)/error_private.h \
$(SRCDIR)/mem.h \
$(SRCDIR)/bitstream.h \
$(SRCDIR)/fse.h

SOURCES = \
$(SRCDIR)/entropy_common.c \
$(SRCDIR)/fse_compress.c \
$(SRCDIR)/fse_decompress.c

OBJS = \
hist.o \
entropy_common.o \
fse_compress.o \
fse_decompress.o

libfse.a:	$(OBJS)
	ar rcs libfse.a $(OBJS)

hist.o: hist.c
	$(CC) -I $(INCLUDES) $(CFLAGS) hist.c

entropy_common.o : entropy_common.c
	$(CC) -I $(INCLUDES) $(CFLAGS) entropy_common.c

fse_compress.o : fse_compress.c
	$(CC) -I $(INCLUDES) $(CFLAGS) fse_compress.c

fse_decompress.o : fse_decompress.c
	$(CC) -I $(INCLUDES) $(CFLAGS) fse_decompress.c

fseU16.o : fseU16.c
	$(CC) -I $(INCLUDES) $(CFLAGS) fseU16.c
		
clean:
	-rm $(OBJS) libfse.a