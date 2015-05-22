/*
 * AIO-BF512 CVTE UPDATE PROTOCOL BY UART
 *
 * Copyright (c) 2014 CVTE SeeCare Inc.
 *
 * Licensed under the GPL-2 or later.
 */


#ifndef _BFIN_PROTOCOL_H_
#define	_BFIN_PROTOCOL_H_

#ifdef CONFIG_CHECK_AIO_BF512_UPDATE

extern int bf_protocol_RespondReady(void);
extern int bf_protocol_GetUpdatePacket(void);
extern int bf_protocol_SaveLDRFile(void);
extern int bf_protocol_LicenseFailed(void);

#endif  /* CONFIG_CHECK_AIO_BF512_UPDATE	*/

#endif	/* _BFIN_PROTOCOL_H_	*/
