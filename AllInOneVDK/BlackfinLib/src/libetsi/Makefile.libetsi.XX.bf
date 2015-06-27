#/************************************************************************
#*
#* Makefile - LIBETSI
#*
#* (c) Copyright 2001-2008 Analog Devices, Inc.  All rights reserved.
#*
#************************************************************************/

ETSI_FLAGS=-D__SET_ETSI_FLAGS=0
ETSI_EXT=
ETSI_DIV_CHECK=-D__ETSI_DIV_CHECKS=0

LIBNAME = libetsi${LIBCHIP}${Y}${ETSI_EXT}.dlb
LIBRARY_NAME = ETSI Run-Time Support Library
libetsi${LIBCHIP}${Y}co.dlb: LIBRARY_OPTIONS_CO = Set ETSI flags

include ../../common.mk

LIBC_INCLUDE_PATH = ../../LIBC/include
LIBC_INCLUDE_DIRS = -I $(LIBC_INCLUDE_PATH)/platform/frio \
                    -I $(LIBC_INCLUDE_PATH)/standard
INCLUDE_DIRS =      -I ../include/platform/frio \
                    $(LIBC_INCLUDE_DIRS)

# Compilation Options

ASMOPTS = $(COMMON_ASM_OPTS) $(INCLUDE_DIRS) \
          $(ETSI_FLAGS) $(ETSI_DIV_CHECK)
COPTS   = $(COMMON_C_OPTS) $(INCLUDE_DIRS) \
          $(ETSI_FLAGS) \
          -DNO_ETSI_BUILTINS

VPATH=../src/platform/frio:../src/standard:

# Object List:

DOJS = 	Div_32.doj \
	etsi_flags.doj \
	L_deposit_l.doj \
	L_sat.doj    \
	Mpy_32_16.doj  \
	msu_r.doj   \
	saturate.doj \
	L_mac.doj \
	L_shl.doj \
	abs_s.doj \
	mult.doj \
	shl.doj \
	L_Extract.doj \
	L_macNs.doj \
	L_shr.doj \
	add.doj \
	mult_r.doj \
	shr.doj \
	L_abs.doj \
	L_msu.doj \
	L_shr_r.doj \
	div_s.doj \
	negate.doj \
	shr_r.doj \
	L_add.doj \
	L_Comp.doj \
	L_msuNs.doj \
	L_sub.doj \
	extract_h.doj \
	norm_l.doj \
	sub.doj \
	L_add_c.doj \
	L_mult.doj \
	L_sub_c.doj \
	extract_l.doj \
	norm_s.doj \
	L_deposit_h.doj \
	L_negate.doj \
	Mpy_32.doj \
	mac_r.doj \
	round.doj \
	L_mls.doj \
	div_l.doj 

.SUFFIXES: .c .s .doj .asm

.s${MT}.doj:
		$(ASM) $(ASMOPTS) -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASMOPTS) -o $@ $<" ;false )

.asm${MT}.doj:
		$(ASM) $(ASMOPTS) -o $@ $<  || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASMOPTS) -o $@ $<" ;false )

.c${MT}.doj:
		$(COMPILER) -c $(COPTS) -o $@ $<  || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(COPTS) -o $@ $<" ;false )


.PHONY: libetsi
libetsi: $(LIBNAME)

$(LIBNAME): $(DOJS)
	$(CREATE_ARVERSION_FILE)
	$(ARCHIVER) $(ARFLAGS) -o $(LIBNAME) $(DOJS)
	rm -f $(ARVERSION_FILE)

.PHONY: clean_libetsi
clean_libetsi:
	rm -f $(DOJS) $(LIBNAME) ; rm -rf MISRARepository
