/* Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** Get information about the event currently in progress.
*/

#pragma file_attr(  "libName=libevent")
#pragma file_attr(  "libFunc=get_interrupt_info")
#pragma file_attr(  "libFunc=_get_interrupt_info")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#include <sys/exception.h>
#include <sys/excause.h>
#include <ccblkfn.h>
#include <sysreg.h>


/*
** pointers to the MMRs
*/

static volatile unsigned int *data_status =
  (volatile unsigned int *)EX_DATA_FAULT_STATUS;
static volatile unsigned int *code_status =
  (volatile unsigned int *)EX_CODE_FAULT_STATUS;
static void * volatile *data_addr = (void * volatile *)EX_DATA_FAULT_ADDR;
static void * volatile *code_addr = (void * volatile *)EX_CODE_FAULT_ADDR;

static __inline void *get_retn(void)
{
  return (void *)sysreg_read(reg_RETN);
}

static __inline void *get_reti(void)
{
  return (void *)sysreg_read(reg_RETI);
}

static __inline void *get_retx(void)
{
  return (void *)sysreg_read(reg_RETX);
}

static __inline int get_seqstat(void)
{
  return (int)sysreg_read(reg_SEQSTAT);
}


/*
** Fetch interrupt info. Note that we have to be told which kind
** of interrupt we're dealing with; we should really work this out.
*/

void get_interrupt_info(interrupt_kind kind, interrupt_info *info)
{
  int seqstat = get_seqstat();

  info->kind = kind;
  switch (kind) {
  default:
    info->kind = ik_err;	/* FALLTHROUGH */
  case ik_reset:
  case ik_global_int_enable:
  case ik_emulation:
  case ik_timer:
    info->pc = get_reti();	/* Actually, false for emulation */
    return;
  case ik_nmi:
    info->pc = get_retn();
    return;
  case ik_exception:
    info->pc = get_retx();
    info->value = seqstat;
    if (EX_IS_ERROR_EXCEPTION(seqstat)) {
      switch (seqstat&EX_BITS) {
      case EX_SYS_DCPLBPROT:
      case EX_SYS_DALIGN:
      case EX_SYS_DCPLBMISS:
      case EX_SYS_DCPLBMHIT:
        info->addr = *data_addr;
        info->status = *data_status;
        break;
      case EX_SYS_CACCESSEX:
      case EX_SYS_CALIGN:
      case EX_SYS_CCPLBPROT:
      case EX_SYS_CCPLBMISS:
      case EX_SYS_CCPLBMHIT:
        info->addr = *code_addr;
        info->status = *code_status;
      }
    }
    return;
  case ik_hardware_err:
    info->pc = get_reti();
    info->value = seqstat;
    return;
  case ik_ivg7:
  case ik_ivg8:
  case ik_ivg9:
  case ik_ivg10:
  case ik_ivg11:
  case ik_ivg12:
  case ik_ivg13:
  case ik_ivg14:
  case ik_ivg15:
    info->pc = get_reti();
    return;
  }
}
