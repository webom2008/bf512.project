readme.txt for OTP driver

__________________________________________________________

There are two versions of the OTP driver.  The first version uses the
System Services and Device Driver model, and is implemented with:

$(VDSP)\Blackfin\lib\src\drivers\OTP\adi_otp.c
$(VDSP)\Blackfin\lib\src\drivers\OTP\adi_otp.h

The second version does not depend on the System Services and Device
Driver model and it is implemented with:

$(VDSP)\Blackfin\lib\src\drivers\OTP\otp_basic.c
$(VDSP)\Blackfin\include\drivers\otp\otp_basic.h

Both versions use the helper functions implemented with:

$(VDSP)\Blackfin\lib\src\drivers\OTP\otp_helper_functions.c
$(VDSP)\Blackfin\lib\src\drivers\OTP\otp_helper_functions.h

Other files in this folder are for legacy purposes.

__________________________________________________________

To use the otp_basic version you must #define _USING_OTP_BASIC_ in
your VisualDSP++ project then include otp_basic.c and otp_helper_functions.c
in your project as well.
