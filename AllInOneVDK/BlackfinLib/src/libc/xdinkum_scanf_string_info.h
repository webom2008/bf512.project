/************************************************************************
 *
 * xdinkum_scanf_string_info.h 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.4.26.1 $
 ************************************************************************/

#ifndef XDINKUM_SCANF_STRING_INFO_H
#define XDINKUM_SCANF_STRING_INFO_H

#include <stdbool.h>

typedef struct {
  _Sft *px;
  char prev_char;
  bool input_failure;
} dinkum_scanf_string_info_t;

#pragma linkage_name __read_char_dinkum_scanf
char read_char_dinkum_scanf(void *cl, char curr_c, int *speculatively_read);

#pragma linkage_name __replace_char_dinkum_scanf
char replace_char_dinkum_scanf(void *cl, char c, int *speculatively_read);

#pragma linkage_name __set_end_position_dinkum_scanf
void set_end_position_dinkum_scanf(void *cl);

#pragma linkage_name __set_overflow_dinkum_scanf
void set_overflow_dinkum_scanf(void *cl);

#endif

/* End of file */
