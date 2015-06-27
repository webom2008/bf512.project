/* Copyright (C) 2006 Analog Devices, Inc. All Rights Reserved.
**
** An out-of-line version of the adi_try_lock() routine.
** If you include <ccblkfn.h> in your C source, you will have
** an inline version, which is recommended.
*/

#include <ccblkfn.h>

#pragma linkage_name _adi_try_lock
int adi_try_lock_out_of_line(testset_t *_t) {
  return adi_try_lock(_t);
}
