/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** local definition of signal table.
**/

#ifndef XSIGNAL_H
#define XSIGNAL_H

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_6_3)
#endif /* _MISRA_RULES */


typedef struct {
  _Sigfun *fn;		/* what we call to process it */
  char once_only;	/* whether to restore default action afterwards */
} vector_entry;

extern vector_entry _vector_table[_NSIG];

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif
