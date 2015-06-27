/*
** Diagnostic exception reporter. Displays information about
** the exception that occurred.
** Copyright (C) 2003 Analog Devices Inc. All Rights Reserved.
*/
#pragma file_attr(  "libName=libeventdbg")
#pragma file_attr(  "libFunc=_ex_report_event")
#pragma file_attr(  "libFunc=__ex_report_event")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <stdio.h>
#include <sys/exception.h>

/*
** Mapping for names.
*/

static const char *interrupt_names[] = {
  "emulation",
  "reset",
  "nmi",
  "exception",
  "global_int_enable",
  "hardware_err",
  "timer",
  "ivg7",
  "ivg8",
  "ivg9",
  "ivg10",
  "ivg11",
  "ivg12",
  "ivg13",
  "ivg14",
  "ivg15",
  0
};

extern void _Exit(void);

static void access_err(interrupt_info *info, const char *msg)
{
  fprintf(stderr, "%s at PC=0x%p\n"
                  "while %s address 0x%p via DAG%c\n",
                  msg, info->pc,
                  (info->status&EX_DATA_FAULT_READWRITE)? "writing" : "reading",
                  info->addr, (info->status&EX_DATA_FAULT_DAG)? '1' : '0');
};

/*
** Display the information concerning the event that occurred.
*/

void _ex_report_event(interrupt_info *info)
{
  interrupt_kind kind = info->kind;

  if (kind < 0) {
    fprintf(stderr,"Unknown interrupt/exception occurred at 0x%p\n",
      info->pc);
    _Exit();
    /* NOTREACHED */
  }
  switch (kind) {
  case ik_reset:
  case ik_global_int_enable:
  case ik_emulation:
  case ik_timer:
  case ik_nmi:
  case ik_ivg7:
  case ik_ivg8:
  case ik_ivg9:
  case ik_ivg10:
  case ik_ivg11:
  case ik_ivg12:
  case ik_ivg13:
  case ik_ivg14:
  case ik_ivg15:
    fprintf(stderr,"Interrupt '%s' at PC=0x%p\n",interrupt_names[kind], info->pc);
    break;
  case ik_exception:
    fprintf(stderr,"Exception 0x%08x at PC=0x%p\n", info->value, info->pc);
    if (EX_IS_ERROR_EXCEPTION(info->value)) {
      unsigned cause = info->value & EX_BITS;
      switch (cause) {
      case EX_SYS_UNDEFINSTR:
        fprintf(stderr, "Undefined instruction at PC=0x%p\n", info->pc);
        break;
      case EX_SYS_ILLINSTRC:
        fprintf(stderr, "Illegal multi-issue instruction combination at PC=0x%p\n", info->pc);
        break;
      case EX_SYS_DCPLBPROT:
        access_err(info, "CPLB protection violation");
        break;
      case EX_SYS_DALIGN:
        access_err(info, "Misaligned data access");
        break;
      case EX_SYS_UNRECEVT:
        fprintf(stderr, "Unrecoverable exception (i.e. during another exception) at PC=0x%p\n",
                        info->pc);
        break;
      case EX_SYS_DCPLBMISS:
        access_err(info,"Data CPLB miss");
        break;
      case EX_SYS_DCPLBMHIT:
        access_err(info,"Multiple Data CPLB hits");
        break;
      case EX_SYS_EMWATCHPT:
        fprintf(stderr, "Emulation Watchpoint match occurred at PC=0x%p\n", info->pc);
        break;
      case EX_SYS_CACCESSEX:
        fprintf(stderr, "Instruction fetch error at PC=0x%p\n", info->pc);
        break;
      case EX_SYS_CALIGN:
        access_err(info, "Misaligned instruction fetch");
        break;
      case EX_SYS_CCPLBPROT:
        access_err(info,"Instruction CPLB protection violation");
        break;
      case EX_SYS_CCPLBMISS:
        access_err(info, "Instruction CPLB Miss");
        break;
      case EX_SYS_CCPLBMHIT:
        access_err(info, "Multiple Instruction CPLB Hits");
        break;
      case EX_SYS_ILLUSESUP:
        access_err(info, "Illegal Use of Supervisor Source");
        break;
      }
    } else if (EX_IS_DEBUG_EXCEPTION(info->value)) {
      switch (info->value & EX_BITS) {
      case EX_DB_SINGLE_STEP:
        fprintf(stderr,"Single-step exception at PC=0x%p\n", info->pc);
        break;
      case EX_DB_EMTRCOVRFLW:
        fprintf(stderr,"Emulator trace buffer overflow at PC=0x%p\n", info->pc);
        break;
      default:
        fprintf(stderr,"Unknown debug exception occurred at PC=0x%p\n", info->pc);
        break;
      }
    } else {
      /* User requested a service of some sort. */
      fprintf(stderr,"Undefined user exception occurred at PC=0x%p: 0x%04x\n",
        info->pc, info->value);
    }
    break;
  case ik_hardware_err:
    fprintf(stderr,"Hardware error occurred at PC=0x%p. HWERRCAUSE=0x%x\n",
      info->pc, info->value);
    if (info->status&EX_DATA_FAULT_ILLADDR)
      fprintf(stderr,"No such memory\n");
    switch (info->value & EX_HWBITS) {
      case EX_HW_DMAHIT:
        fprintf(stderr,"DMA Bus Comparator Source Hit\n");
        break;
#if defined(EX_HW_SYSMMR)
      case EX_HW_SYSMMR:
        fprintf(stderr,"Bad System MMR Access\n");
        break;
#endif
#if defined(EX_HW_EXTMEM)
      case EX_HW_EXTMEM:
        fprintf(stderr,"External Memory Addressing Error\n");
        break;
#endif
      case EX_HW_PERFMON:
        fprintf(stderr,"Performance Monitor Overflow\n");
        break;
#if defined(EX_HW_NOMEM1)
      case EX_HW_NOMEM1:
        fprintf(stderr,"Error accessing reserved or undefined memory\n");
        break;
#endif
#if defined(EX_HW_NOMEM2)
      case EX_HW_NOMEM2:
        fprintf(stderr,"Error accessing reserved or undefined memory\n");
        break;
#endif
      case EX_HW_RAISE:
        fprintf(stderr,"Software issued a RAISE instruction\n");
        break;
      default:
        fprintf(stderr,"Unrecognised hardware error\n");
        break;
    }
    break;
  }
}
