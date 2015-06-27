/*
** Copyright (C) 2003 Analog Devices, Inc. All Rights Reserved.
** The pointer to the table which contains data to reinitialize
** the runtime.
*/
#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")
#include <meminit.h>
__section("bsz_init") mi_table_header *__inits;
