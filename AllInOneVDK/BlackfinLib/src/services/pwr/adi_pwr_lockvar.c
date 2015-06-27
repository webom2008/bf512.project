#pragma file_attr( "sharing=MustShare")

#include <ccblkfn.h>

// Lock variable for core synchronization
section ("l2_shared") testset_t adi_pwr_lockvar = 0;
