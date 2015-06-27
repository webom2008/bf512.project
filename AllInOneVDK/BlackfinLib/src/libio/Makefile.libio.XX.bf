#/************************************************************************
#*
#* Makefile - LIBIO
#*
#* (c) Copyright 2001-2009 Analog Devices, Inc.  All rights reserved.
#*
#************************************************************************/

INSTALL_BUILD = yes

LIBNAME = libio${FX}${LIBCHIP}${MT}${Y}.dlb
ifeq ($(GEN_SYSCALL),yes)
COPY_OF_LIB = 
else
COPY_OF_LIB = libio${FX}${LIBCHIP}_${MT}${Y}.dlb
endif
LIBRARY_NAME = Host-based I/O Run-Time Library

include ../../common.mk

# Search Paths for Header Files
ifeq ($(INSTALL_BUILD), no)
LIBC         = ../../../../LIBC
INCLUDE_DIRS = -I ../../../include/internal      \
               -I ../../../include/platform/frio \
               -I $(LIBC)/include/platform/frio  \
               -I $(LIBC)/include/internal       \
               -I $(LIBC)/include/standard
VPATH        = ..:../../platform/frio
else
INCLUDE_DIRS = -I ../../../../include \
               -I ../../libc \
               -I ..
VPATH        = ..
endif

ASMOPTS = $(COMMON_ASM_OPTS)
COPTS   = $(COMMON_C_OPTS) $(INCLUDE_DIRS) -D_PRIMIO -D_ADI_LIBIO
ifeq ($(FX),fx)
COPTS  += -fixed-point-io
endif


# The basic primio interface itself objects

DOJS_BLKFN = \
	xprim.doj            \
	xprim_close.doj      \
	xprim_open.doj       \
	xprim_read.doj       \
	xprim_write.doj      \
	xprim_seek.doj       \
	xprim_remove.doj     \
	xprim_rename.doj     \
	closreg.doj

# Device IO objects

DOJS_DEVIO = \
	defiodev.doj         \
	devtab.doj           \
	devtab_entry.doj     \
	devIOtab.doj         \
	dev_dup.doj          \
	dev_init.doj         \
	dev_open.doj         \
	dev_read.doj         \
	dev_seek.doj         \
	dev_write.doj        \
	dev_close.doj        \
	primiolib.doj        \
	dev_init_dummy.doj

# Objects that are not sensitive to the size of type double:

DOJS_GENERAL = \
	clearerr.doj         \
	eof.doj              \
	fclose.doj           \
	ferror.doj           \
	fflush.doj           \
	fgetc.doj            \
	fgetpos.doj          \
	fgets.doj            \
	fopen.doj            \
	fputc.doj            \
	fputs.doj            \
	fread.doj            \
	freopen.doj          \
	fseek.doj            \
	fsetpos.doj          \
	ftell.doj            \
	fwrite.doj           \
	getc.doj             \
	getchar.doj          \
	gets.doj             \
	putc.doj             \
	putchar.doj          \
	puts.doj             \
	remove.doj           \
	rename.doj           \
	rewind.doj           \
	setbuf.doj           \
	setvbuf.doj          \
	ungetc.doj           \
	xdo_output.doj       \
	xdoopenfile.doj      \
	xflushwritebuf.doj   \
	xlong_to_string.doj  \
	xpad.doj             \
	xparse_field.doj     \
	xparse_format.doj    \
	xfiles.doj           \
	xhexdigits.doj       \
	xprim.doj            \
	xprim_close.doj      \
	xprim_open.doj       \
	xprim_read.doj       \
	xprim_write.doj      \
	xprim_write2.doj     \
	xprim_remove.doj     \
	xprim_rename.doj     \
	xprim_seek.doj       \
	xprint_fixed.doj     \
	xput_string.doj      \
	xreadbuf.doj         \
	xseek.doj            \
	xsnprintfputchar.doj \
	xsprintfputchar.doj  \
	xtheputchar.doj      \
	xlonglong_to_string.doj \
	widechar.doj

ifeq ($(FX), fx)
DOJS_GENERAL += \
	xprint_fx.doj
endif
 
# Objects that ARE sensitive to the size of type double (or call objects
# that are sensitive to the size of type double). 

DOJS_DOUBLE_SENSITIVE = \
	fprintf.doj         \
	fscanf.doj          \
	printf.doj          \
	scanf.doj           \
	snprintf.doj        \
	sprintf.doj         \
	sscanf.doj          \
	vfprintf.doj        \
	vprintf.doj         \
	vsnprintf.doj       \
	vsprintf.doj        \
	xprint_a_float.doj  \
	xprint_float.doj    \
	xprnt.doj           \
	xscan.doj

# Objects when DOUBLES_ARE_FLOAT:

DOBJS_32_SPECIFIC = ${DOJS_DOUBLE_SENSITIVE:.doj=_32.doj}

# Objects when DOUBLES_ARE_LONG_DOUBLE:

DOBJS_64_SPECIFIC = ${DOJS_DOUBLE_SENSITIVE:.doj=_64.doj}

# Object List:

DOJS = ${DOJS_GENERAL} ${DOBJS_32_SPECIFIC} ${DOBJS_64_SPECIFIC} ${DOJS_DEVIO}

.SUFFIXES:
.SUFFIXES: .c .asm .doj _32.doj _32.doj _64.doj _64.doj

.asm.doj:
		$(ASM) $(ASMOPTS) -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASMOPTS) -o $@ $<" ; false )

.c.doj:
		$(COMPILER) -c $(COPTS) -double-size-any -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(COPTS) -double-size-any -o $@ $<"  ; false )

.c_32.doj:
		$(COMPILER) -c $(COPTS) -double-size-32 -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(COPTS) -double-size-32 -o $@ $<"  ; false )

.c_64.doj:
		$(COMPILER) -c $(COPTS) -double-size-64 -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(COPTS) -double-size-64 -o $@ $<"  ; false )

.PHONY: libio
libio: $(LIBNAME) $(COPY_OF_LIB)

$(LIBNAME): $(DOJS)
	@$(CREATE_ARVERSION_FILE)
	$(ARCHIVER) $(ARFLAGS) -o $(LIBNAME) $(DOJS)
	@rm -f $(ARVERSION_FILE)

$(COPY_OF_LIB): $(LIBNAME)
	@cp $(LIBNAME) $(COPY_OF_LIB)

.PHONY: clean_libio
clean_libio:
	rm -rf $(DOJS) $(LIBNAME) $(COPY_OF_LIB) ; rm -rf MISRARepository
