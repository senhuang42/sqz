CXX=g++
CXXFLAGS=-pedantic -Wall -g -std=c++17 -O3

INCLUDES = .
INCLUDESFSE = ./lib
SRCDIR = .

HEADERS = \
$(SRCDIR)/lib/fse.h \
$(SRCDIR)/lib/error_public.h \
$(SRCDIR)/lib/error_private.h \
$(SRCDIR)/lib/mem.h \
$(SRCDIR)/lib/bitstream.h \
$(SRCDIR)/lib/hist.h \

SOURCES = \
$(SRCDIR)/lib/fse_compress.c \
$(SRCDIR)/lib/fse_decompress.c \
$(SRCDIR)/lib/entropy_common.c \
$(SRCDIR)/lib/hist.c \

# SQZ libraries
sqz: cli.o SubframeProcessor.o LPC.o decode.o FseEncoder.o encode.o lib/libfse.a
	$(CXX) $(CXXFLAGS) -o sqz cli.o  SubframeProcessor.o LPC.o decode.o FseEncoder.o encode.o lib/libfse.a

cli.o: cli.cpp encode.h

SubframeProcessor.o: SubframeProcessor.cpp SubframeProcessor.h

LPC.o: LPC.cpp LPC.h

decode.o: decode.cpp decode.h InBufferedBitFileStream.h

encode.o: encode.cpp encode.h OutBufferedBitFileStream.h MDP.h StreamHeader.h SubframeProcessor.h LPC.h

FseEncoder.o: FseEncoder.cpp FseEncoder.h lib/libfse.a

clean:
	-rm *.o sqz