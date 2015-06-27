/* Copyright (C) 2000-2008 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP as defined in the
** ADI/Intel Collaboration Agreement.
*/
.section ctorl;
.align 4;
___ctor_end:
.global ___ctor_end;
   .byte4=0;    // NULL terminator reauired by __ctorloop
.type ___ctor_end,STT_OBJECT; 

.section .gdtl;
.align 4;
___eh_gdt_end:
.global ___eh_gdt_end;
    .byte4=0;
.type ___eh_gdt_end,STT_OBJECT; 

