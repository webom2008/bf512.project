# #########################################################################
# Common defines. For all architectures
# #########################################################################

# Always add these switches to the compiler/assembler, no matter what!
ALWAYS_DEFINE = -no-std-inc

# Promote warnings to errors
WARN_OPT += -Werror-warnings -flags-asm -Wwarn-error

# Fault unused variables and undefined macros
C_WARN_OPT += -warn-protos -Werror 177 -Werror 193

# Enable compiler self-checking
C_WARN_OPT += +check_bril

# Suppressed compiler warnings
# cc164[56]: function exceeds speed/space ratio - inlining anyway (so be it)
C_WARN_OPT += -Wsuppress 1645,1646

# Multi-threaded support
MT_OPT=
ASM_MT_OPT=
LIBRARY_OPTIONS_MT=
ifeq ($(MT),mt)
  ASM_MT_OPT = -threads
  MT_OPT = -threads
  LIBRARY_OPTIONS_MT = multi-threaded
endif

# C++ exceptions and RTTI Support
EXCEPT_OPT=
RTTI_OPT=
LIBRARY_OPTIONS_EH=
ifeq ($(X),x)
  EXCEPT_OPT=-eh
  RTTI_OPT=-rtti
  LIBRARY_OPTIONS_EH = C++ exceptions and RTTI
endif

# #########################################################################
# SHARC Common Options
# #########################################################################
ifeq ($(ARCH),sharc)

ifeq ($(CHIP),020)
IS_210xx = true
endif

ifeq ($(CHIP),060)
IS_210xx = true
endif

ifeq ($(IS_210xx), true)
# ea1195, ea1196: .MESSAGE for diagnostics that don't apply to 210xx
#                 (ignore, to avoid having to guard .MESSAGE directives)
ASM_WARN_OPT += -flags-asm -Wsuppress,1195,-Wsuppress,1196
# ea1131: .swf_{on,off} not supported by 21k assembler
WARN_OPT += -flags-asm -Wsuppress,1131
else
# Enable all assembler anomaly warnings (not supported by 21k assembler)
WARN_OPT += -flags-asm -anomaly-detect,all
endif

ifeq ($(CHIP),160)
IS_211xx=1
# ea2500: possible shadow write FIFO anomaly (false positives)
WARN_OPT += -flags-asm -Wsuppress,2500
# Due to anomaly 02000014, RFRAME should not be used on 21160
ASM_WARN_OPT += -DRFRAME=NO_RFRAME -Drframe=no_rframe
endif

ifeq ($(CHIP),161)
IS_211xx=1
endif

ifdef IS_211xx
# ea250[678]: possible BR glitch anomalies (false positives)
WARN_OPT += -flags-asm -Wsuppress,2506,-Wsuppress,2507,-Wsuppress,2508
endif

ifeq ($(CHIP), 369)
# ea2509: The recommended workaround for anomaly 08000014 on 2136[789] rev 0.1
#         is to put code and DMA-accessed data in different memory blocks
WARN_OPT += -flags-asm -Wsuppress,2509
# ea2504: false positives for 08000002
C_WARN_OPT += -flags-asm -Winfo,2504
endif

ifeq ($(CHIP), 375)
# ea251[89]: infos about 09000011 workarounds (ok)
WARN_OPT += -flags-asm -Wsuppress,2518,-Wsuppress,2519
endif

ifeq ($(CHIP), 469)
# ea2536: info about fixup of return address (ok)
WARN_OPT += -flags-asm -Wsuppress,2536
endif

COMPILER = $(CC21K) $(ALWAYS_DEFINE)

ifndef DONT_SET_CHIP
PROCESSOR = ADSP-21$(CHIP)
COMPILER += -proc $(PROCESSOR) -si-revision $(SI) \
            -check-si-rev -check-proc
else
# Used in cases where the Makefile builds for multiple platofrms
ifndef DONT_SET_SI
COMPILER += -si-revision $(SI) -check-si-rev
endif
endif
ASM      = $(COMPILER) -c
ARCHIVER = $(COMPILER)
ARWFLAGS = -build-lib -flags-lib -W0080


# Controlling optimisations
# Optimisations used during the library build are controlled using the env.
# var $PART_OPT.
# Libraries are built using either the global $PART_OPT or by a 
# per-lib $PART_OPT, which is set in the library makefile before this
# file is included. To override it (for example, setting it to -g to build
# debug libraries), set it below this comment.

ANOMALY_OPT=
ASM_ANOMALY_OPT=
VISA_OPT=
NOSIMD_OPT=
SWFA_OPT=
SWFA_ASM_OPT=
LIBRARY_OPTIONS_VISA=
LIBRARY_OPTIONS_SWFA=
LIBRARY_OPTIONS_SIMD=simd

# Chip/Revision/Flavour specific options
ifeq ($(SWFA),swfa)
  SWFA_ASM_OPT = -D_ADI_SWFA=1 -flags-asm -swf_screening_fix
  SWFA_OPT = -workaround swfa
  LIBRARY_OPTIONS_SWFA = $(SWFA_OPT)
endif

ifeq ($(ARCHDIR),214xx)
ifneq ($(VISA),_nwc)
  VISA_OPT = -swc
  LIBRARY_OPTIONS_VISA = $(VISA_OPT)
endif
endif

ifneq ($(IS_210xx),true)
  NOSIMD_OPT = -no-simd
  LIBRARY_OPTIONS_SIMD=
endif

ifneq ($(SI),none)
  ifeq ($(CHIP),160)
    ANOMALY_OPT  = -workaround rframe,dag-stall \
                   -flags-asm -anomaly-detect,21160-anomaly-64
    ASM_ANOMALY_OPT = -D__21160_GLITCH -D__WORKAROUND_2116X_ANOMALY_64__ \
                      -flags-asm -anomaly-detect,21160-anomaly-64
  endif
  ifeq ($(CHIP),161)
    ANOMALY_OPT  = -workaround 21161-anomaly-45,dag-stall \
                   -flags-asm -anomaly-detect,21160-anomaly-64
    ASM_ANOMALY_OPT = -D__21160_GLITCH -flags-asm \
                      -anomaly-detect,21160-anomaly-64             
  endif
  ifeq ($(CHIP),267)
    ASM_ANOMALY_OPT = -D__WORKAROUND_2126X_ANOMALY4__
  endif
  ifeq ($(CHIP),365)
    ASM_ANOMALY_OPT = -D__WORKAROUND_2136X_ANOMALY2__ \
                      -D__WORKAROUND_2136X_ANOMALY3__
  endif
  ifeq ($(CHIP),369)
    ASM_ANOMALY_OPT = -D__WORKAROUND_2136X_ANOMALY3__
  endif
  ifeq ($(CHIP),375)
    ASM_ANOMALY_OPT = -D__WORKAROUND_2136X_ANOMALY2__ \
                      -D__WORKAROUND_2136X_ANOMALY3__ \
                      -flags-asm -anomaly-detect,09000014 \
                      -flags-asm -anomaly-detect,09000015 \
                      -flags-asm -anomaly-detect,09000018 \
                      -flags-asm -Werror,2521,-Werror,2522,-Werror,2523
    ANOMALY_OPT = -flags-asm -anomaly-detect,09000014 \
                  -flags-asm -anomaly-detect,09000015 \
                  -flags-asm -anomaly-detect,09000018 \
                  -flags-asm -Werror,2521,-Werror,2522,-Werror,2523
  endif
endif

# Flags for Compilation
RESERVE_REGS = i0,b0,l0,m0,i1,b1,l1,m1,i8,b8,l8,m8,i9,b9,l9,m9,mrb,ustat1,ustat2
RESERVE_OPT  = -reserve $(RESERVE_REGS)


# ADDITIONAL_ASMFLAGS is a global option that can be set by the user when 
# building the libraries.
COMMON_ASM_OPTS = $(ADDITIONAL_ASMFLAGS) $(SWFA_ASM_OPT) $(VISA_OPT) \
                  $(ASM_ANOMALY_OPT) $(ASM_MT_OPT) $(WARN_OPT) $(ASM_WARN_OPT) \
                  -D__ADI_LIB_BUILD__ \
                  -double-size-any

# ADDITIONAL_CFLAGS is a global option that can be set by the user when 
# building the libraries.
# PART_OPT is an optimisation control flag, and is passed in to each library 
# built, by the top level makefile.
COMMON_C_AND_CPP_OPTS = $(ADDITIONAL_CFLAGS) $(VISA_OPT) $(PART_OPT) \
                        $(MT_OPT) $(NOSIMD_OPT) $(SWFA_OPT) $(ANOMALY_OPT) \
                        $(RESERVE_OPT) $(WARN_OPT) $(C_WARN_OPT) \
                        -D__ADI_LIB_BUILD__ \
                        -double-size-any -no-shift-to-add \
                        -structs-do-not-overlap -annotate

COMMON_C_OPTS   = $(COMMON_C_AND_CPP_OPTS) -c89
COMMON_CPP_OPTS = $(COMMON_C_AND_CPP_OPTS) -c++ $(RTTI_OPT) $(EXCEPT_OPT)

endif

# ##########################################################################
# Blackfin Common Options
# ##########################################################################
ifeq ($(ARCH),bf)

# Enable all assembler anomaly warnings 
WARN_OPT += -flags-asm -anomaly-detect,all

ifeq ($(CHIP),535)
# ea5504: anomaly 0165 not fully worked around (TAR24721)
WARN_OPT += -flags-asm -Wsuppress5504
endif

COMPILER   = $(CCBLKFN) $(ALWAYS_DEFINE)

ifndef DONT_SET_CHIP
ifeq ($(CHIP), 506f)
PROCESSOR = ADSP-BF506F
else
PROCESSOR = ADSP-BF$(CHIP)
endif
COMPILER += -proc $(PROCESSOR) -si-revision $(SI) \
            -check-si-rev -check-proc
endif
ASM       = $(COMPILER) -c
ARCHIVER  = $(COMPILER)
ARWFLAGS  = -build-lib -flags-lib -W0080

FIXED_POINT_OPTS=
ifeq ($(FIXED_POINT_ALLOWED),yes)
FIXED_POINT_OPTS = -DFIXED_POINT_ALLOWED
endif

# ADDITIONAL_ASMFLAGS is a global option that can be set by the user when 
# building the libraries.
COMMON_ASM_OPTS = $(ADDITIONAL_ASMFLAGS) $(ASM_MT_OPT) \
                  $(FIXED_POINT_OPTS) $(WARN_OPT) $(ASM_WARN_OPT) \
                  -flags-asm -double-size-any -D__ADI_LIB_BUILD__

# ADDITIONAL_CFLAGS is a global option that can be set by the user when 
# building the libraries.
# PART_OPT is an optimisation control flag, and is passed in to each library 
# built, by the top level makefile.
COMMON_C_AND_CPP_OPTS   = $(ADDITIONAL_CFLAGS) $(PART_OPT) $(MT_OPT) \
                  $(FIXED_POINT_OPTS) $(WARN_OPT) $(C_WARN_OPT) \
                  -double-size-any -no-extra-keywords \
                  -structs-do-not-overlap -reserve M3 -bss \
                  -annotate -D__ADI_LIB_BUILD__

COMMON_C_OPTS    = $(COMMON_C_AND_CPP_OPTS) -c89
COMMON_CPP_OPTS  = $(COMMON_C_AND_CPP_OPTS) -c++ $(RTTI_OPT) $(EXCEPT_OPT)
endif

# #########################################################################
# Common Library information settings
# Ensure that LIBRARY_NAME is defined by the including makefile before this
# file is included.
# PART_OPT is the optimisation switch used to build the lib.
###########################################################################

# VERSION HISTORY: Please update this table when you update the version info
# VERSION    # DATE ISSUED  # AUTHOR   # COMMENT
# 8.0.9.2      20/10/10       SS         Changes post 22/10 Stage drop
# 8.0.9.1      08/10/10       SS         Changes post 7/10 Stage drop
# 8.0.9.0      15/08/10       DJG        Initial version implementation

VERSION          = 8.0.9.2
LIBRARY_OPTIONS  = $(LIBRARY_OPTIONS_MT) $(LIBRARY_OPTIONS_VISA) \
                   $(LIBRARY_OPTIONS_SWFA) $(PART_OPT) $(ANOMALY_OPT) \
                   $(ASM_ANOMALY_OPT) $(LIBRARY_OPTIONS_SIMD) \
                   $(LIBRARY_OPTIONS_EH) $(LIBRARY_OPTIONS_CO)
LIBRARY_VERSION  = $(VERSION)
LIBRARY_BUILT    = `date`
LIBRARY_PLATFORM = $(PROCESSOR)
LIBRARY_SI_REV   = $(SI)

# Use extended name to support parallel builds
ARVERSION_FILE   = elfar_version_$(CHIP)_$(SI)$(MT)$(SWFA)$(VISA)$(X).txt
ARFLAGS          = $(ARWFLAGS) -flags-lib -tx,$(ARVERSION_FILE)
CREATE_ARVERSION_FILE = (                                            \
   echo 'Library_Name:    $(LIBRARY_NAME)'     >  $(ARVERSION_FILE); \
   echo 'Library_Version: $(LIBRARY_VERSION)'  >> $(ARVERSION_FILE); \
   echo "Library_Built:   $(LIBRARY_BUILT)"    >> $(ARVERSION_FILE); \
   echo 'Platform:        $(LIBRARY_PLATFORM)' >> $(ARVERSION_FILE); \
   echo 'Silicon Revision:$(LIBRARY_SI_REV)'   >> $(ARVERSION_FILE); \
   echo 'Special_Options: $(LIBRARY_OPTIONS)'  >> $(ARVERSION_FILE); \
   echo 'Special_Notes:   $(LIBRARY_NOTES)'    >> $(ARVERSION_FILE); \
)

