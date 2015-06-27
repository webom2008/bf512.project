/* 
 * adi_fatal_data.c: The data needed for adi_fatal_error and 
 * adi_fatal_exception. Kept seperately so that either function can be
 * replaced by the user 
 * 
 * (c) Copyright 2010 Analog Devices Inc. All rights reserved.
 */

int adi_fatal_error_general_code = 0;
int adi_fatal_error_specific_code = 0;
int adi_fatal_error_value = 0;
unsigned int adi_fatal_error_pc = 0;
