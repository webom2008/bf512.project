/************************************************************************
 *
 * LV_matrix.h
 *
 * (c) Copyright 2005-2006 Analog Devices, Inc.  All rights reserved.
 * $Revision: 4 $
 ************************************************************************/

#ifndef __LV_MATRIX_DEFINED
#define __LV_MATRIX_DEFINED

#include <matrix.h>

#ifdef __cplusplus
  extern "C" {
#endif


/* Passing of complex 2D-matrices from LabView into Blackfin LibDSP

   | Function argument generated by LabView |
        |
         --> | Pointer to data structure |
                  |
                   --> | Number of rows                |
                       | Number of columns             |
                       | Start complex matrix elements |  <= Address required
                       | ..                            |     by BLackfin libdsp
                                                             library functions.
*/


#pragma always_inline
static __inline void
cmatsaddf_LV (const complex_float ** const _matrix[],
              const complex_float * _scalar,
              int _rows, int _columns,
              complex_float ** _sum[])
{
   cmatsaddf( (complex_float *) ((*_matrix) + 2),
              *_scalar, _rows, _columns,
              (complex_float *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatssubf_LV (const complex_float ** const _matrix[],
              const complex_float * _scalar, 
              int _rows, int _columns,
              complex_float ** _difference[])
{
   cmatssubf( (complex_float *) ((*_matrix) + 2),
              *_scalar, _rows, _columns,
              (complex_float *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatsmltf_LV (const complex_float ** const _matrix[],
              const complex_float * _scalar, 
              int _rows, int _columns,
              complex_float ** _product[])
{
   cmatsmltf( (complex_float *) ((*_matrix) + 2),
              *_scalar, _rows, _columns,
              (complex_float *) ((*_product) + 2) );
}  

#pragma always_inline
static __inline void
cmatmaddf_LV (const complex_float ** const _matrix_a[],
              const complex_float ** const _matrix_b[],
              int _rows, int _columns,
              complex_float ** _sum[])
{
   cmatmaddf( (complex_float *) ((*_matrix_a) + 2), 
              (complex_float *) ((*_matrix_b) + 2),
              _rows, _columns, 
              (complex_float *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatmsubf_LV (const complex_float ** const _matrix_a[],
              const complex_float ** const _matrix_b[],
              int _rows, int _columns,
              complex_float ** _difference[])
{
   cmatmsubf( (complex_float *) ((*_matrix_a) + 2),
              (complex_float *) ((*_matrix_b) + 2),
              _rows, _columns,
              (complex_float *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatmmltf_LV (const complex_float ** const _matrix_a[],
              int _rows_a, int _columns_a,
              const complex_float ** const _matrix_b[],
              int _columns_b,
              complex_float ** _product[])
{
   cmatmmltf( (complex_float *) ((*_matrix_a) + 2), 
              _rows_a, _columns_a, 
              (complex_float *) ((*_matrix_b) + 2), 
              _columns_b, 
              (complex_float *) ((*_product) + 2) );
}


#pragma always_inline
static __inline void
cmatsaddd_LV (const complex_long_double ** const _matrix[],
              const complex_long_double * _scalar, 
              int _rows, int _columns,
              complex_long_double ** _sum[])
{
   cmatsaddd( (complex_long_double *) ((*_matrix) + 2), 
              *_scalar, _rows, _columns, 
              (complex_long_double *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatssubd_LV (const complex_long_double ** const _matrix[],
              const complex_long_double * _scalar, 
              int _rows, int _columns,
              complex_long_double ** _difference[])
{
   cmatssubd( (complex_long_double *) ((*_matrix) + 2),
              *_scalar, _rows, _columns,
              (complex_long_double *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatsmltd_LV (const complex_long_double ** const _matrix[],
              const complex_long_double * _scalar, 
              int _rows, int _columns,
              complex_long_double ** _product[])
{
   cmatsmltd( (complex_long_double *) ((*_matrix) + 2),
              *_scalar, _rows, _columns,
              (complex_long_double *) ((*_product) + 2) );
}

#pragma always_inline
static __inline void
cmatmaddd_LV (const complex_long_double ** const _matrix_a[],
              const complex_long_double ** const _matrix_b[],
              int _rows, int _columns,
              complex_long_double ** _sum[])
{
   cmatmaddd( (complex_long_double *) ((*_matrix_a) + 2), 
              (complex_long_double *) ((*_matrix_b) + 2), 
              _rows, _columns, 
              (complex_long_double *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatmsubd_LV (const complex_long_double ** const _matrix_a[],
              const complex_long_double ** const _matrix_b[],
              int _rows, int _columns,
              complex_long_double ** _difference[])
{
   cmatmsubd( (complex_long_double *) ((*_matrix_a) + 2),
              (complex_long_double *) ((*_matrix_b) + 2),
              _rows, _columns,
              (complex_long_double *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatmmltd_LV (const complex_long_double ** const _matrix_a[],
              int _rows_a, int _columns_a,
              const complex_long_double ** const _matrix_b[],
              int _columns_b,
              complex_long_double ** _product[])
{
   cmatmmltd( (complex_long_double *) ((*_matrix_a) + 2),
              _rows_a, _columns_a,
              (complex_long_double *) ((*_matrix_b) + 2),
              _columns_b,
              (complex_long_double *) ((*_product) + 2) );
}


#pragma always_inline
static __inline void
cmatsadd_fr16_LV (const complex_fract16 ** const _matrix[],
                  const complex_fract16 * _scalar, 
                  int _rows, int _columns,
                  complex_fract16 ** _sum[])
{
   cmatsadd_fr16( (complex_fract16 *) ((*_matrix) + 2), 
                  *_scalar, _rows, _columns, 
                  (complex_fract16 *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatssub_fr16_LV (const complex_fract16 ** const _matrix[],
                  const complex_fract16 * _scalar, 
                  int _rows, int _columns,
                  complex_fract16 ** _difference[])
{
   cmatssub_fr16( (complex_fract16 *) ((*_matrix) + 2),
                  *_scalar, _rows, _columns,
                  (complex_fract16 *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatsmlt_fr16_LV (const complex_fract16 ** const _matrix[],
                  const complex_fract16 * _scalar, 
                  int _rows, int _columns,
                  complex_fract16 ** _product[])
{
   cmatsmlt_fr16( (complex_fract16 *) ((*_matrix) + 2),
                  *_scalar, _rows, _columns,
                  (complex_fract16 *) ((*_product) + 2) );
}

#pragma always_inline
static __inline void
cmatmadd_fr16_LV (const complex_fract16 ** const _matrix_a[],
                  const complex_fract16 ** const _matrix_b[],
                  int _rows, int _columns,
                  complex_fract16 ** _sum[])
{
   cmatmadd_fr16( (complex_fract16 *) ((*_matrix_a) + 2), 
                  (complex_fract16 *) ((*_matrix_b) + 2), 
                  _rows, _columns, 
                  (complex_fract16 *) ((*_sum) + 2) );
}

#pragma always_inline
static __inline void
cmatmsub_fr16_LV (const complex_fract16 ** const _matrix_a[],
                  const complex_fract16 ** const _matrix_b[],
                  int _rows, int _columns,
                  complex_fract16 ** _difference[])
{
   cmatmsub_fr16( (complex_fract16 *) ((*_matrix_a) + 2),
                  (complex_fract16 *) ((*_matrix_b) + 2),
                  _rows, _columns,
                  (complex_fract16 *) ((*_difference) + 2) );
}

#pragma always_inline
static __inline void
cmatmmlt_fr16_LV (const complex_fract16 ** const _matrix_a[],
                  int _rows_a, int _columns_a,
                  const complex_fract16 ** const _matrix_b[],
                  int _columns_b,
                  complex_fract16 ** _product[])
{
   cmatmmlt_fr16( (complex_fract16 *) ((*_matrix_a) + 2),
                  _rows_a, _columns_a,
                  (complex_fract16 *) ((*_matrix_b) + 2), 
                  _columns_b,
                  (complex_fract16 *) ((*_product) + 2) );
}


#ifdef __cplusplus
  }
#endif
#endif  /* __LV_MATRIX_DEFINED */
