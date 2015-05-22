****************************************************************************************************

ezkitBF5xx_initcode EXAMPLE README

Analog Devices, Inc.
DSP Division

Date Created:   05/22/07

____________________________________________________________________________________________________


General EXAMPLE project for initialization code.
This file will be executed on the processor prior to application code BOOTING.
Settings be included within this file are:
- Dynamic Power Management / Voltage Regulator (optional, must be activated by using #define __ACTIVATE_DPM__)
- PLL (optional, must be activated by using #define __ACTIVATE_DPM__)
- EBIU settings (Asynchronous and DDR-/SDR-SDRAM memory)
- Bit Rate handler for Mode 'Boot from UART host (slave mode)'

-> ADSP-BF50x, ADSP-BF51x, ADSP-BF52x Silicon Revision 0.1 (and above) and ADSP-BF54x Silicon Revision 0.2 (and above):
These Boot ROMs now support SysControl() function for handling Dynamic Power Management and Voltage Control.
More details can be found in the Hardware Reference Manual. The code choses by preprocessor definitions
the correct functions.

Booting different Applications (DXE) with callable ROM functions:
Multi-DXE Boot Streams can be handled by the init code. The Boot ROM supports some functions that help to identify and
select a specific application within the boot stream.
HINT:   Negative numbers are used to identify the DXE files. If the -CRC32 switch is used with the elfloader,
        DXE0 (-> dBlockCount = '-1') is for the CRC32 function. DXE1 (-> dBlockCount = '-2') is the init block.
        DXE2 (-> dBlockCount = '-3') is the first application.
You can use the following command line within the project options (post-build section) to create a ldr-file with
multiple DXE files. Include one of them to the init code example project options.
$(VDSP)\elfloader.exe -proc ADSP-BF548 -si-revision 0.2 01.dxe 02.dxe -CRC32 -b flash     -f hex -width 16 -init ..Debug\ezkitBF548_initcode_ROM-V02.dxe -o ..Debug\flash.ldr
$(VDSP)\elfloader.exe -proc ADSP-BF548 -si-revision 0.2 01.dxe 02.dxe -CRC32 -b spimaster -f hex -width  8 -init ..Debug\ezkitBF548_initcode_ROM-V02.dxe -o ..Debug\spi.ldr
User needs to insert a kind of messaging system (e.g. push button/GPIO signal).
Within the example, the boot mode is used to make the decision, if application 1 or 2 will be bootet.

NOTE: UART Bit Rate handler sends a pattern of 4 byte containing 0xBF, UARTx_DLL, UARTx_DLH, 0x00.

IMPORTANT: Settings are EZ-KIT Lite specific!

____________________________________________________________________________________________________


Creating my individual init code

Projects can be taken as pattern for individual init code projects. Individual settings are done in the
project options (e.g. chose the correct processor and silicon revision) as well as in the corresponding
initcode header file (e.g. ezkitBF527_initcode.h). Changes in source code are not required.

____________________________________________________________________________________________________

The following preprocessor definitions are created in 'init_platform.h' and are not provided by the tools:

__ACTIVATE_DPM__            Activate dynamic power management. This definition can be set individually
                            within the ezboardBF5xx_initcode.h files or globally in 'init_platform.h'.


The following preprocessor definitions are created in the project options or in 'init_platform.h'
(if Sil. Rev. specific) and are not provided by the tools:

__USEBFSYSCONTROL__         Use BfSysControl Functions
__EXTVOLTAGE__              Target uses external voltage regulator instead of internal voltage regulator.
                            Feature only implemented for C-version of the init_code examples.
__WORKAROUND_05000432__     Workaround for anomaly 05-000-432
__WORKAROUND_05000440__     Workaround for anomaly 05-000-440
__BMODE_UART0HOST__         Target supports booting from UART0 host (slave mode)
__BMODE_UART1HOST__         Target supports booting from UART1 host (slave mode)
__BMODE_SPIMEM_INT__        Target supports booting from internal SPI Flash memory (master mode)
__OLD_ROM_REV__             Includes (supported) Blackfin family processors ADSP-BF53x and ADSP-BF561

Unused, but created in the project options:
__SDRSDRAM__                Target features SDR-SDRAM
__DDRSDRAM__                Target Features DDR-SDRAM

____________________________________________________________________________________________________


Files contained in this directory:
|
|-> ezkit_initcode_all.dpg
|
[c]
|
|-> ezkit_initcode_c_all.dpg
|-> README.txt
|
|- [ADSP-BF506F EZ-KIT Lite]
   |
   |-> ezkitBF506f_initcode.c (EZ-Board specific source file)
   |-> ezkitBF506f_initcode_ROM-V00.dpj (EZ-Board specific project file)
   |-> ezkitBF506f_initcode.h (EZ-Board specific header file)
   |-> ezkitBF506f_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF518F EZ-Board]
   |
   |-> ezboardBF518f_initcode.c (EZ-Board specific source file)
   |-> ezboardBF518f_initcode_ROM-V00.dpj (EZ-Board specific project file)
   |-> ezboardBF518f_initcode_ROM-V01.dpj (EZ-Board specific project file)
   |-> ezboardBF518f_initcode_ROM-V02.dpj (EZ-Board specific project file)
   |-> ezboardBF518f_initcode.h (EZ-Board specific header file)
   |-> ezboardBF518f_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF526 EZ-Board]
   |
   |-> ezboardBF526_initcode.c (EZ-Board specific source file)
   |-> ezboardBF526_initcode_ROM-V00.dpj (EZ-Board specific project file)
   |-> ezboardBF526_initcode_ROM-V01.dpj (EZ-Board specific project file)
   |-> ezboardBF526_initcode_ROM-V02.dpj (EZ-Board specific project file)
   |-> ezboardBF526_initcode_ROM-V03.dpj (EZ-Board specific project file)
   |-> ezboardBF526_initcode.h (EZ-Board specific header file)
   |-> ezboardBF526_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF527 EZ-KIT Lite]
   |
   |-> ezkitBF527_initcode.c (EZ-KIT Lite specific source file)
   |-> ezkitBF527_initcode_ROM-V00.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF527_initcode_ROM-V01.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF527_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF527_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF527_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF548 EZ-KIT Lite]
   |
   |-> ezkitBF548_initcode.c (EZ-KIT Lite specific source file)
   |-> ezkitBF548_initcode_ROM-V00.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode_ROM-V01.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode_ROM-V04.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF548_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF526 EZ-KIT Lite - Multi-DXE]
   |
   |-> ezkitBF548_initcode_Multi-DXE.c (EZ-KIT Lite specific source file)
   |-> ezkitBF548_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF548_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF527 EZ-KIT Lite - Multi-DXE]
   |
   |-> ezkitBF548_initcode_Multi-DXE.c (EZ-KIT Lite specific source file)
   |-> ezkitBF548_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF548_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF548 EZ-KIT Lite - Multi-DXE]
   |
   |-> ezkitBF548_initcode_Multi-DXE.c (EZ-KIT Lite specific source file)
   |-> ezkitBF548_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode_ROM-V04.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF548_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF548_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [ADSP-BF592 EZ-KIT Lite]
   |
   |-> ezkitBF592_initcode.c (EZ-KIT Lite specific source file)
   |-> ezkitBF592_initcode_ROM-V00.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF592_initcode_ROM-V01.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF592_initcode_ROM-V02.dpj (EZ-KIT Lite specific project file)
   |-> ezkitBF592_initcode.h (EZ-KIT Lite specific header file)
   |-> ezkitBF592_initcode.ldf (sets Architecture and just includes ezkitBF5xx_initcode.ldf)
|
|- [src]
   |
   |-> ezkitBF5xx_initcode.ldf (general linker description file, included by specific ldf files, e.g. ezkitBF527_initcode.ldf)
   |-> init_platform.h
   |-> NOR_FLASH.c (configure NOR FLASH memory and configure EBIU for specific FLASH modes)
   |-> NOR_FLASH.h
   |-> nBlackfin.h
   |-> system.c
   |-> system.h
   |-> UART.c
   |-> UART.h
   |-> TWI.c
   |-> TWI.h
   |-> VR_DIGIPOT.c
   |-> VR_DIGIPOT.h

****************************************************************************************************EOF
