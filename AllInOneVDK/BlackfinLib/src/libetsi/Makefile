# **************************************************************************
# 
# 	Analog Devices Blackfin ETSI Implementation. 
#  	Copyright (C). 2002 Analog Devices Inc., All Rights Reserved.
# 
#  **************************************************************************

CC=ccblkfn
ARCH=535
CFLAGS=-O -21${ARCH} -I. -I ../../../include/platform/frio
ETSI_FLAGS=-D__SET_ETSI_FLAGS=0
ETSI_FLAG_EXTENSION=
ETSI_DIV_CHECK=-D__ETSI_DIV_CHECKS=0

SRC= 	Div_32${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	etsi_flags${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_deposit_l${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_sat${ARCH}${ETSI_FLAG_EXTENSION}.doj    \
	Mpy_32_16${ARCH}${ETSI_FLAG_EXTENSION}.doj  \
	msu_r${ARCH}${ETSI_FLAG_EXTENSION}.doj   \
	saturate${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_mac${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_shl${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	abs_s${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	mult${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	shl${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_Extract${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_macNs${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_shr${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	add${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	mult_r${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	shr${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_abs${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_msu${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_shr_r${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	div_s${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	negate${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	shr_r${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_add${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_Comp${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_msuNs${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_sub${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	extract_h${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	norm_l${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	sub${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_add_c${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_mult${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_sub_c${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	extract_l${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	norm_s${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_deposit_h${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	L_negate${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	Mpy_32${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	mac_r${ARCH}${ETSI_FLAG_EXTENSION}.doj \
	round${ARCH}${ETSI_FLAG_EXTENSION}.doj

LIBETSI=libetsi_${ARCH}${ETSI_FLAG_EXTENSION}.dlb

.SUFFIXES:
.SUFFIXES: .s .c ${ARCH}${ETSI_FLAG_EXTENSION}.doj

.s${ARCH}${ETSI_FLAG_EXTENSION}.doj:
	${CC}  -c -o $@ $<  ${CFLAGS} ${ETSI_FLAGS} ${ETSI_DIV_CHECK} 

.c${ARCH}${ETSI_FLAG_EXTENSION}.doj:
	${CC} -c -o $@ $< ${CFLAGS} ${ETSI_FLAGS} ${ETSI_DIV_CHECK}

all: ${LIBETSI}

libetsi_${ARCH}${ETSI_FLAG_EXTENSION}.dlb: ${SRC}
	${CC} -build-lib \
		-o libetsi_${ARCH}${ETSI_FLAG_EXTENSION}.dlb ${SRC}

clean:
	-rm -f *.doj *.dlb
