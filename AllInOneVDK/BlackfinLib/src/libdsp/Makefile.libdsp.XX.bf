#/************************************************************************
#*
#* Makefile - LIBDSP (Blackfin)
#*
#* (c) Copyright 2002-2010 Analog Devices, Inc.  All rights reserved.
#*    
#************************************************************************/

LIBDSP       = libdsp${LIBCHIP}${Y}.dlb
LIBRARY_NAME     = DSP Run-Time Library

include ../../common.mk


LIBC_INCLUDE_PATH = ../../LIBC/include
LIBETSI_INCLUDE_PATH = ../../LIBETSI/include
INCLUDE_DIRS = -I ../include/platform/frio           \
               -I ../include/internal                \
               -I ../include/                        \
               -I $(LIBC_INCLUDE_PATH)/platform/frio \
               -I $(LIBC_INCLUDE_PATH)/internal      \
               -I $(LIBC_INCLUDE_PATH)/standard      \
               -I $(LIBETSI_INCLUDE_PATH)/standard   \
               -I $(LIBETSI_INCLUDE_PATH)/platform/frio

ASM_OPTS = $(COMMON_ASM_OPTS) $(INCLUDE_DIRS)
C_OPTS   = $(COMMON_C_OPTS) $(INCLUDE_DIRS)

VPATH = ../src/platform/frio

# <complex.h> Lists

COMPLEX_C_SRC64 = \
   argd.c        \
   cabsd.c       \
   caddd.c       \
   cartesiand.c  \
   cdivd.c       \
   cexpd.c       \
   cmltd.c       \
   conjd.c       \
   csubd.c       \
   normd.c       \
   polard.c    

COMPLEX_C_SRC32 = \
   arg.c            \
   arg16.c          \
   arg_fr32.c       \
   cabs.c           \
   cabs_fr32.c      \
   cadd16.c         \
   cadd_fr32.c      \
   cartesian.c      \
   cartesian16.c    \
   cartesian_fr32.c \
   cdiv.c           \
   cdiv_fr32.c      \
   cexp.c           \
   cmlt16.c         \
   conj16.c         \
   conj_fr32.c      \
   csub16.c         \
   csub_fr32.c      \
   norm.c           \
   polar.c          \
   polar_fr32.c     \
   polar16.c

COMPLEX_ASM_SRC = \
   cabfr16.asm    \
   cdiv_fr16.asm  \
   cmlt_fr32.asm

COMPLEX_C_OBJS32 = $(COMPLEX_C_SRC32:.c=32.doj)
COMPLEX_C_OBJS64 = $(COMPLEX_C_SRC64:.c=64.doj)
COMPLEX_ASM_OBJS = $(COMPLEX_ASM_SRC:.asm=.doj)

COMPLEX_OBJS     = $(COMPLEX_C_OBJS32) $(COMPLEX_C_OBJS64) \
                   $(COMPLEX_ASM_OBJS)

# <filter.h> Lists

FILTER_C_SRC =          \
   cfft2d_fr32.c        \
   cfir_fr32.c          \
   coeff_iirdf1_fr16.c  \
   coeff_iirdf1_fr32.c  \
   conv2d3x3_fr16.c     \
   conv2d3x3_fr32.c     \
   conv2d_fr32.c        \
   fft_magnitude_fr16.c \
   fft_magnitude_fr32.c \
   ifft2d_fr32.c        \
   rfft2d_fr32.c        \
   twiddle_fr16.c       \
   twidfft2d_fr16.c     \
   twidfftrad2_fr16.c   \
   twidfftrad2_fr32.c   \
   twidfftrad4_fr16.c   \
   twidfftf_fr16.c      \
   twidfftf_fr32.c 

FILTER_ASM_SRC =       \
   a_compress16.asm    \
   a_expand16.asm      \
   cfft2d_fr16.asm     \
   cfftdyn_fr16.asm    \
   cfft_fr32.asm       \
   cfftf_fr32.asm      \
   cfftf_fr16.asm      \
   cfftrad2_fr16.asm   \
   cfftrad4_fr16.asm   \
   cfir_fr16.asm       \
   conv2dasm.asm       \
   convolve_asm.asm    \
   convolve_fr32.asm   \
   fir_decima_fr32.asm \
   fir_decima16.asm    \
   fir_fr16.asm        \
   fir_fr32.asm        \
   fir_interp_fr32.asm \
   fir_interp16.asm    \
   ifft2d_fr16.asm     \
   ifftdyn_fr16.asm    \
   ifft_fr32.asm       \
   ifftf_fr32.asm      \
   ifftrad2_fr16.asm   \
   ifftrad4_fr16.asm   \
   iir_fr16.asm        \
   iir_fr32.asm        \
   iirdf1_fr16.asm     \
   iirdf1_fr32.asm     \
   mu_compress16.asm   \
   mu_expand16.asm     \
   rfft2d_fr16.asm     \
   rfftdyn_fr16.asm    \
   rfft_fr32.asm       \
   rfftf_fr32.asm      \
   rfftrad2_fr16.asm   \
   rfftrad4_fr16.asm 

FILTER_C_OBJS   = $(FILTER_C_SRC:.c=.doj)
FILTER_ASM_OBJS = $(FILTER_ASM_SRC:.asm=.doj)

FILTER_OBJS     = $(FILTER_C_OBJS) $(FILTER_ASM_OBJS)

# <matrix.h> Lists

MATRIX_C_SRC64 = \
   cmatmmltd.c \
   matmmltd.c  

MATRIX_C_SRC32 = \
   cmatmmlt.c      \
   cmatmmlt_fr32.c \
   matmmltf.c  

MATRIX_ASM_SRC =    \
   cmatmmlt_asm.asm \
   matmmlt_fr16.asm \
   matmmlt_fr32.asm \
   transpm16.asm    \
   transpm32.asm    \
   transpm64.asm    \
   transpm128.asm

MATRIX_C_OBJS32 = $(MATRIX_C_SRC32:.c=32.doj)
MATRIX_C_OBJS64 = $(MATRIX_C_SRC64:.c=64.doj)
MATRIX_ASM_OBJS = $(MATRIX_ASM_SRC:.asm=.doj)

MATRIX_OBJS     = $(MATRIX_C_OBJS32) $(MATRIX_C_OBJS64) $(MATRIX_ASM_OBJS)

# <stats.h> Lists

STATS_C_SRC64 =  \
   autocohd.c   \
   autocorrd.c  \
   crosscohd.c  \
   crosscorrd.c \
   histogramd.c \
   meand.c      \
   rmsd.c       \
   vard.c       

STATS_C_SRC32 =  \
   autocoh.c        \
   autocoh_fr32.c   \
   autocorr.c       \
   autocorr_fr32.c  \
   crosscoh.c       \
   crosscoh_fr32.c  \
   crosscorr.c      \
   crosscorr_fr32.c \
   histogram.c      \
   histogram_fr32.c \
   mean.c           \
   mean_fr32.c      \
   means.c          \
   rms.c            \
   rms_fr32.c       \
   var.c            \
   var_fr32.c       \
   zerocross_fr32.c

STATS_ASM_SRC =       \
   autocoh_fr16.asm   \
   autocorr_fr16.asm  \
   crosscoh_fr16.asm  \
   crosscorr_fr16.asm \
   histogram_fr16.asm \
   mean_fr16.asm      \
   rms_fr16.asm       \
   var_fr16.asm       \
   zerocross.asm      \
   zero_crossf.asm    \
   zero_crossd.asm

STATS_C_OBJS32 = $(STATS_C_SRC32:.c=32.doj)
STATS_C_OBJS64 = $(STATS_C_SRC64:.c=64.doj)
STATS_ASM_OBJS = $(STATS_ASM_SRC:.asm=.doj)

STATS_OBJS     = $(STATS_C_OBJS32) $(STATS_C_OBJS64) \
                 $(STATS_ASM_OBJS)

# <math.h> fract16 trig functions Lists

TRIG_16_C_SRC =

TRIG_16_ASM_SRC = \
   acos_fr16.asm  \
   asin_fr16.asm  \
   atan_fr16.asm  \
   atan2_fr16.asm \
   cos_fr16.asm   \
   sin_fr16.asm   \
   sqrt_fr16.asm  \
   tan_fr16.asm

TRIG_16_C_OBJS   = $(TRIG_16_C_SRC:.c=.doj)
TRIG_16_ASM_OBJS = $(TRIG_16_ASM_SRC:.asm=.doj)

TRIG_16_OBJS     = $(TRIG_16_C_OBJS) $(TRIG_16_ASM_OBJS)

# <math.h> fract32 trig functions Lists

TRIG_32_C_SRC =   \
   acos_fr32.c    \
   asin_fr32.c    \
   atan_fr32.c    \
   atan2_fr32.c   \
   cos_fr32.c     \
   sin_fr32.c     \
   tan_fr32.c

TRIG_32_ASM_SRC = \
   sqrt_fr32.asm 

TRIG_32_C_OBJS   = $(TRIG_32_C_SRC:.c=.doj)
TRIG_32_ASM_OBJS = $(TRIG_32_ASM_SRC:.asm=.doj)

TRIG_32_OBJS     = $(TRIG_32_C_OBJS) $(TRIG_32_ASM_OBJS)

# <math.h> non-ANSI floating point trig functions Lists

TRIG_NSTDFLT_C_SRC64 = \
   alog10d.c \
   alogd.c   \
   cotd.c

TRIG_NSTDFLT_C_SRC32 = \
   alogf.c   \
   alog10f.c \
   cotf.c

TRIG_NSTDFLT_ASM_SRC = \
   rsqrtf.asm \
   rsqrtd.asm

TRIG_NSTDFLT_C_OBJS32 = $(TRIG_NSTDFLT_C_SRC32:.c=32.doj)
TRIG_NSTDFLT_C_OBJS64 = $(TRIG_NSTDFLT_C_SRC64:.c=64.doj)
TRIG_NSTDFLT_ASM_OBJS = $(TRIG_NSTDFLT_ASM_SRC:.asm=.doj)

TRIG_NSTDFLT_OBJS     = $(TRIG_NSTDFLT_C_OBJS32) \
                        $(TRIG_NSTDFLT_C_OBJS64) \
                        $(TRIG_NSTDFLT_ASM_OBJS)

# <math.h> floating point trig functions Lists

TRIG_FLT_C_SRC64 = \
   acosd.c  \
   asind.c  \
   atan2d.c \
   atand.c  \
   ceild.c  \
   cosd.c   \
   coshd.c  \
   expd.c   \
   floord.c \
   frexpd.c \
   ldexpd.c \
   log10d.c \
   logd.c   \
   modfd.c  \
   powd.c   \
   sind.c   \
   sinhd.c  \
   tand.c   \
   tanhd.c

TRIG_FLT_C_SRC32 = \
   acosf.c       \
   asinf.c       \
   atanf.c       \
   atan2f.c      \
   ceil.c        \
   cosf.c        \
   cosh.c        \
   cotf.c        \
   exp.c         \
   floor.c       \
   fmod.c        \
   frexp.c       \
   log.c         \
   log10.c       \
   modf.c        \
   pow.c         \
   sinf.c        \
   sinh.c        \
   tanf.c        \
   tanh.c

TRIG_FLT_ASM_SRC = \
   lib_mult64.asm     \
   lib_sqrt_seeds.asm \
   sqrtd.asm          \
   sqrtf.asm

TRIG_FLT_C_OBJS32 = $(TRIG_FLT_C_SRC32:.c=32.doj)
TRIG_FLT_C_OBJS64 = $(TRIG_FLT_C_SRC64:.c=64.doj)
TRIG_FLT_ASM_OBJS = $(TRIG_FLT_ASM_SRC:.asm=.doj)

TRIG_FLT_OBJS     = $(TRIG_FLT_C_OBJS32) $(TRIG_FLT_C_OBJS64) \
                    $(TRIG_FLT_ASM_OBJS)

# <vector.h> Lists

VECTOR_C_SRC64 = \
   cvecdotd.c   \
   cvecsaddd.c  \
   cvecsmltd.c  \
   cvecssubd.c  \
   cvecvaddd.c  \
   cvecvmltd.c  \
   cvecvsubd.c  \
   vecdotd.c    \
   vecmaxd.c    \
   vecmaxlocd.c \
   vecmind.c    \
   vecminlocd.c \
   vecsaddd.c   \
   vecsmltd.c   \
   vecssubd.c   \
   vecvaddd.c   \
   vecvmltd.c   \
   vecvsubd.c

VECTOR_C_SRC32 = \
   cvecdot_fr32.c   \
   cvecdotf.c       \
   cvecsadd_fr32.c  \
   cvecsaddf.c      \
   cvecsmlt_fr32.c  \
   cvecsmlt.c       \
   cvecssub_fr32.c  \
   cvecssubf.c      \
   cvecvadd_fr32.c  \
   cvecvaddf.c      \
   cvecvmlt_fr32.c  \
   cvecvmltf.c      \
   cvecvsub_fr32.c  \
   cvecvsubf.c      \
   vecdotf.c        \
   vecmax_fr32.c    \
   vecmaxf.c        \
   vecmaxloc_fr32.c \
   vecmin_fr32.c    \
   vecminf.c        \
   vecminloc_fr32.c \
   vecsadd_fr32.c   \
   vecsaddf.c       \
   vecsmlt_fr32.c   \
   vecsmltf.c       \
   vecssub_fr32.c   \
   vecssubf.c       \
   vecvadd_fr32.c   \
   vecvaddf.c       \
   vecvmlt_fr32.c   \
   vecvmltf.c       \
   vecvsub_fr32.c   \
   vecvsubf.c       \
   vmaxlocf.c       \
   vminlocf.c

VECTOR_ASM_SRC =      \
   csadd.asm          \
   cssub.asm          \
   cvecdot_fr16.asm   \
   cvecsmult_fr16.asm \
   cvecvmult_fr16.asm \
   cvvadd.asm         \
   cvvsub.asm         \
   vecdot.asm         \
   vecdot_fr32.asm    \
   vmax.asm           \
   vmaxloc.asm        \
   vmin.asm           \
   vminloc.asm        \
   vsadd.asm          \
   vsmlt.asm          \
   vssub.asm          \
   vvadd.asm          \
   vvmlt.asm          \
   vvsub.asm

VECTOR_C_OBJS32 = $(VECTOR_C_SRC32:.c=32.doj)
VECTOR_C_OBJS64 = $(VECTOR_C_SRC64:.c=64.doj)
VECTOR_ASM_OBJS = $(VECTOR_ASM_SRC:.asm=.doj)

VECTOR_OBJS     = $(VECTOR_C_OBJS32) $(VECTOR_C_OBJS64) $(VECTOR_ASM_OBJS)

# <window.h> Lists

WINDOW_C_SRC =  \
   Lib_bessel0d.c         \
   Lib_cos16_2PIx.c       \
   Lib_cos32_2PIx.c       \
   gen_bart.c             \
   gen_bartlett_fr32.c    \
   gen_black.c            \
   gen_blackman_fr32.c    \
   gen_gaussian_fr16.c    \
   gen_gaussian_fr32.c    \
   gen_hamm.c             \
   gen_hamming_fr32.c     \
   gen_hann.c             \
   gen_hanning_fr32.c     \
   gen_harris.c           \
   gen_harris_fr32.c      \
   gen_kaiser.c           \
   gen_kaiser_fr32.c      \
   gen_rect.c             \
   gen_rectangular_fr32.c \
   gen_trian.c            \
   gen_triangle_fr32.c    \
   gen_vonh.c

WINDOW_ASM_SRC = 

WINDOW_C_OBJS   = $(WINDOW_C_SRC:.c=.doj)
WINDOW_ASM_OBJS = $(WINDOW_ASM_SRC:.asm=.doj)

WINDOW_OBJS     = $(WINDOW_C_OBJS) $(WINDOW_ASM_OBJS)

# Miscellaneous functions Lists

MISC_C_SRC64 = \
   copysignd.c \
   fabsd.c     \
   fclipd.c    \
   fmaxd.c     \
   fmind.c     \
   isinfd.c    \
   isnand.c

MISC_C_SRC32 = \
   abs16.c    \
   abs_fr32.c \
   clip.c     \
   copysign_fr32.c \
   countll.c  \
   fclipf.c   \
   fmaxf.c    \
   fminf.c    \
   llclip.c   \
   max.c      \
   max16.c    \
   maxll.c    \
   max_fr32.c \
   min.c      \
   min16.c    \
   minll.c    \
   min_fr32.c \
   signf.c    \
   fl32max.c  \
   fl32min.c  \
   trncr4u4.c

MISC_ASM_SRC =    \
   clip_fr16.asm  \
   counti.asm     \
   div16.asm      \
   divfract32.asm \
   float_to_raw64.asm \
   fltsif.asm     \
   fltuif.asm     \
   fltcmp.asm     \
   flteq.asm      \
   fltgt.asm      \
   fltgteq.asm    \
   fltlt.asm      \
   fltlteq.asm    \
   fpadd.asm      \
   fpdiv.asm      \
   fpmult.asm     \
   roundflti.asm  \
   sign_fr16.asm  \
   f32toi32z.asm  \
   f32tou32z.asm  \
   divrem_s32.asm \
   divrem_u32.asm \
   divrem_s64.asm \
   divrem_u64.asm \
   div_s32.asm    \
   rem_s32.asm    \
   div_u32.asm    \
   rem_u32.asm    \
   rem_s64.asm    \
   rem_u64.asm    \
   lldiv.asm      \
   isinf.asm      \
   isnan.asm      \
   ashfti64.asm   \
   lshfti64.asm   \
   mul_64.asm     \
   trncr4i8.asm   \
   trncr4u8.asm   \
   floatdisf.asm  \
   floatdiuf.asm  \
   mult64_32x32.asm \
   multu64_32x32.asm \

FLOAT16_ASM_SRC = \
   addfl16.s \
   cmpfl16.s \
   divfl16.s \
   mulfl16.s \
   subfl16.s

OLD_FLOAT16_ASM_SRC = \
   absfl16.s   \
   addsub_fl16.s  \
   cmp_fl16.s  \
   div_fl16.s  \
   fitsfl16.s  \
   fl162fl.s   \
   fl162fr16.s \
   fl2fl16.s   \
   fr162fl16.s \
   mul_fl16.s  \
   negfl16.s   \
   normfl16.s 

FLOAT32_ASM_SRC = \
   absfl32.s   \
   addfl32.s   \
   cmpfl32.s   \
   divfl32.s   \
   fl322fl.s   \
   fl322fr32.s \
   fl2fl32.s   \
   fr322fl32.s \
   mulfl32.s   \
   negfl32.s   \
   normfl32.s  \
   subfl32.s



MISC_C_OBJS32 = $(MISC_C_SRC32:.c=32.doj)
MISC_C_OBJS64 = $(MISC_C_SRC64:.c=64.doj)
MISC_ASM_OBJS = $(MISC_ASM_SRC:.asm=.doj)

MISC_OBJS     = $(MISC_C_OBJS32) $(MISC_C_OBJS64) \
                $(MISC_ASM_OBJS) $(OLD_FLOAT16_ASM_OBJS) $(FLOAT32_ASM_OBJS)

OLD_FLOAT16_ASM_OBJS = $(OLD_FLOAT16_ASM_SRC:.s=.doj)
FLOAT16_ASM_OBJS  = $(FLOAT16_ASM_SRC:.s=.doj)
FLOAT32_ASM_OBJS  = $(FLOAT32_ASM_SRC:.s=.doj)
FLOAT64_ASM_OBJS  = $(FLOAT64_ASM_SRC:.asm=.doj)
FLOAT64F_ASM_OBJS = $(FLOAT64F_ASM_SRC:.asm=.doj)

# Library Object Lists

LIBDSP_LIST       = $(COMPLEX_OBJS)      \
                    $(FILTER_OBJS)       \
                    $(MATRIX_OBJS)       \
                    $(STATS_OBJS)        \
                    $(TRIG_16_OBJS)      \
                    $(TRIG_32_OBJS)      \
                    $(TRIG_NSTDFLT_OBJS) \
                    $(TRIG_FLT_OBJS)     \
                    $(VECTOR_OBJS)       \
                    $(WINDOW_OBJS)       \
                    $(MISC_OBJS)

# Suffix Rules

.SUFFIXES: .asm .c .doj 32.doj 64.doj .s 

.asm.doj:
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

.s.doj:
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

.c.doj:
	$(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $<" ;false )

.c32.doj:
	$(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $<" ;false )

.c64.doj:
	$(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $< || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -double-size-any -no-fp-associative -o $@ $<" ;false )

.PHONY: libdsp
libdsp:		$(LIBDSP)

$(LIBDSP):	$(LIBDSP_LIST)
	$(CREATE_ARVERSION_FILE)
	$(ARCHIVER) $(ARFLAGS) -o $(LIBDSP) $(LIBDSP_LIST)
	rm -f $(ARVERSION_FILE)

.PHONY: clean_libdsp
clean_libdsp:
	rm -f *.doj *.dlb ; rm -rf MISRARepository

# Special Rules

cfftdyn_fr16.doj:	cfft_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )
   
cfftrad2_fr16.doj:	cfftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

cfftrad4_fr16.doj:	cfftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $<" ;false )

ifftdyn_fr16.doj:	ifft_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

ifftrad2_fr16.doj:	ifftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

ifftrad4_fr16.doj:	ifftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $<" ;false )

rfftdyn_fr16.doj:	rfft_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

rfftrad2_fr16.doj:	rfftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -o $@ $<" ;false )

rfftrad4_fr16.doj:	rfftrad24_fr16.asm
	$(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $< || ( echo "Error in Assemble [`pwd`]: $(ASM) $(ASM_OPTS) -flags-asm -double-size-any -D__RADIX_4_FFT -o $@ $<" ;false )

cadd1632.doj:
	$(COMPILER) -c $(C_OPTS) -no-builtin \
		    -double-size-any -o $@ $< \
                    ../src/platform/frio/cadd16.c || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -no-builtin -double-size-any -o $@ $<" ;false )

cmlt1632.doj:
	$(COMPILER) -c $(C_OPTS) -no-builtin \
		    -double-size-any -o $@ $< \
                    ../src/platform/frio/cmlt16.c || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -no-builtin -double-size-any -o $@ $<" ;false )

conj1632.doj:
	$(COMPILER) -c $(C_OPTS) -no-builtin \
		    -double-size-any -o $@ $< \
		    ../src/platform/frio/conj16.c || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -no-builtin -double-size-any -o $@ $<" ;false )

csub1632.doj:
	$(COMPILER) -c $(C_OPTS) -no-builtin \
		    -double-size-any -o $@ $< \
                    ../src/platform/frio/csub16.c || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -no-builtin -double-size-any -o $@ $<" ;false )

pow32.doj: pow.c
	$(COMPILER) -c $(C_OPTS) -no-fp-associative \
		    -double-size-any -o pow32.doj \
		    ../src/platform/frio/pow.c || ( echo "Error in Compile [`pwd`]: $(COMPILER) -c $(C_OPTS) -no-builtin -double-size-any -o $@ $<" ;false )
