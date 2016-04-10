CC = gcc
BASICOPTS = -O -D_7ZIP_ST 
CFLAGS = $(BASICOPTS) \
	-I. \
	-Icrc \
	-Ilzma \
	-Ivunzip \
	-Ivzip

TARGETDIR=build/bin

all: $(TARGETDIR)/vunzip $(TARGETDIR)/vzip

OBJS = \
	$(TARGETDIR)/LzFind.o \
	$(TARGETDIR)/allocation.o \
	$(TARGETDIR)/crc32.o
	
OBJS_vunzip = \
	$(TARGETDIR)/vunzip.o \
	$(TARGETDIR)/LzmaDec.o

OBJS_vzip = \
	$(TARGETDIR)/vzip.o \
	$(TARGETDIR)/LzmaEnc.o

$(TARGETDIR)/vunzip: $(TARGETDIR) $(OBJS_vunzip) $(OBJS) 
	$(LINK.c) $(CFLAGS) -o $@ $(OBJS_vunzip) $(OBJS) 

$(TARGETDIR)/vzip: $(TARGETDIR) $(OBJS) $(OBJS_vzip)
	$(LINK.c) $(CFLAGS) -o $@ $(OBJS) $(OBJS_vzip)

$(TARGETDIR)/vunzip.o: $(TARGETDIR) vunzip/main.c
	$(COMPILE.c) $(CFLAGS) -o $@ vunzip/main.c

$(TARGETDIR)/vzip.o: $(TARGETDIR) vzip/main.c
	$(COMPILE.c) $(CFLAGS) -o $@ vzip/main.c

$(TARGETDIR)/LzFind.o: $(TARGETDIR) lzma/LzFind.c
	$(COMPILE.c) $(CFLAGS) -o $@ lzma/LzFind.c

$(TARGETDIR)/LzmaDec.o: $(TARGETDIR) lzma/LzmaDec.c
	$(COMPILE.c) $(CFLAGS) -o $@ lzma/LzmaDec.c

$(TARGETDIR)/LzmaEnc.o: $(TARGETDIR) lzma/LzmaEnc.c
	$(COMPILE.c) $(CFLAGS) -o $@ lzma/LzmaEnc.c

$(TARGETDIR)/allocation.o: $(TARGETDIR) lzma/allocation.c
	$(COMPILE.c) $(CFLAGS) -o $@ lzma/allocation.c

$(TARGETDIR)/crc32.o: $(TARGETDIR) crc/crc32.c
	$(COMPILE.c) $(CFLAGS) -o $@ crc/crc32.c

clean:
	rm -f \
		$(TARGETDIR)/vunzip \
		$(TARGETDIR)/vzip \
		$(TARGETDIR)/vunzip.o \
		$(TARGETDIR)/vzip.o \
		$(TARGETDIR)/LzFind.o \
		$(TARGETDIR)/LzmaDec.o \
		$(TARGETDIR)/LzmaEnc.o \
		$(TARGETDIR)/allocation.o \
		$(TARGETDIR)/crc32.o
	rm -f -r $(TARGETDIR)

$(TARGETDIR):
	mkdir -p $(TARGETDIR)
