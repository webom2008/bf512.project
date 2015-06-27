#include "low_level_api.h"


.section    L1_code;

.global     _otp_command;
.global     _otp_read;
.global     _otp_write;
.align      8;

_otp_command:
	nop;
	nop;
_otp_command.end:

_otp_read:
	nop;
    nop;
_otp_read.end: 

_otp_write:
	nop;
	nop;
_otp_write.end: 
