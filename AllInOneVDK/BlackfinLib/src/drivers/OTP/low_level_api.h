/******************************************************
* prototypes for lowest level OTP interface functions *
******************************************************/

#if !defined(_LANGUAGE_ASM)

/*******************************************
* OTP generic initialisation and management routine
*******************************************/
u32  otp_command(u32 command, u32 value);

/***************************************************************************************
* these two functions should cover all read and write cases
* the parameters are explained below
***************************************************************************************/

u32  otp_read (u32 page, u32 flags, u64 *page_content);
u32  otp_write(u32 page, u32 flags, u64 *page_content);

#endif //#if !defined(_LANGUAGE_ASM)
