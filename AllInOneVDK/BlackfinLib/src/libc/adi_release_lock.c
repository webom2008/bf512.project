/* Copyright (C) 2006 Analog Devices, Inc. All Rights Reserved.
**
** An out-of-line version of the adi_release_lock() routine.
** If you include <ccblkfn.h> in your C source, you will have
** an inline version, which is recommended.
*/

#include <sys/mc_typedef.h>

void adi_release_lock(testset_t *t) {
  __builtin_untestset((char *) t);
  __builtin_ssync();
}
