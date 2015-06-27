#######################################################
#  Generated Makefile for libio
#######################################################

# Set CC to the compiler to build the target
CCBLKFN = ../../../../../ccblkfn.exe

# Set CHIP and SI to the chip and silicon revision of the library you wish to build.
# Libraries are only built for inflection points, and the following combinations are valid:
# Chip = 506f, Revisions = any,
# Chip = 512, Revisions = any,
# Chip = 522, Revisions = any,
# Chip = 523, Revisions = any,
# Chip = 542, Revisions = any,
# Chip = 532, Revisions = none,0.1,0.2,0.3,0.5,any,
# Chip = 561, Revisions = none,0.2,0.3,0.5,any,
# Chip = 534, Revisions = 0.0,0.3,any,
# Chip = 592-A, Revisions = 0.1,any,
# Chip = 535, Revisions = none,0.2,any,

CHIP = 506f
SI = any

# Y is used in the naming of the library, when set to y, it implies the library was built
# with workarounds enabled
ifeq ($(SI),none)
Y =
else
Y = y
endif

# LIBCHIP is used in the naming of some libraries
LIBCHIP=532
ifeq ($(CHIP),535)
LIBCHIP=535
endif
ifeq ($(CHIP),561)
LIBCHIP=561
endif
# Set PART_OPT to the optimization used during the build.
# This library is typically built with: "-Os", 
PART_OPT = -Os

# Combination values can be set to built variants of the library.
# In some combinations you may need to set more than one variable.
# Valid combinations are:
# MT (Multi-threaded): <no value>, mt, 
# FX (Embedded C fract support): <no value>, fx, 
MT = 
FX = 

default_rule:
	mkdir -p .libio_$(CHIP)_$(SI) ;\
	 make -C .libio_$(CHIP)_$(SI) -f ../Makefile.libio.XX.bf CCBLKFN="$(CCBLKFN)" ARCH="bf" CHIP="$(CHIP)" SI="$(SI)" Y="$(Y)" PART_OPT="-Os" MT="$(MT)" FX="$(FX)"  libio


clean:
	rm -rf .libio_$(CHIP)_$(SI)

