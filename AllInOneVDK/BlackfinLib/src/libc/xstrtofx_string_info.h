/************************************************************************
 *
 * xstrtofx_string_info.h 
 *
 * (c) Copyright 2008 Analog Devices, Inc.  All rights reserved.
 * $Revision: 1.4.26.1 $
 ************************************************************************/

#ifndef XSTRTOFX_STRING_INFO_H
#define XSTRTOFX_STRING_INFO_H

typedef struct {
  const char *nptr;
  char **endptr;
  int index;
} strtofx_string_data_t;

#pragma linkage_name __read_char_strtofx
char read_char_strtofx(void *cl, char curr_c, int *speculatively_read);

#pragma linkage_name __replace_char_strtofx
char replace_char_strtofx(void *cl, char c, int *speculatively_read);

#pragma linkage_name __set_end_position_strtofx
void set_end_position_strtofx(void *cl);

#pragma linkage_name __set_overflow_strtofx
void set_overflow_strtofx(void *cl);

#endif

/* End of file */
