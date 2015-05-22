/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_check.c
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_check
**
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-12-02
** Version:             V1.00
** Descriptions:        system_check
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_check
**
** Rechecked by:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Descriptions:        
**
** Rechecked by:        
*********************************************************************************************************/
#include "system_check.h"

#ifdef __SYSTEM_CHECK_C_

#define SYSTEM_CHECK_DEBUG_INFO

#ifdef SYSTEM_CHECK_DEBUG_INFO
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SYSTEM_CHECK] "fmt, ##arg)
#else
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...)
#endif

system_state g_system_state;
system_state *pSystemState = &g_system_state;

#define BIT_CPU             (1<<7)
#define BIT_FLASH           (1<<6)
#define BIT_SDRAM           (1<<5)
#define BIT_WDG             (1<<4)
#define BIT_EEPROM          (1<<3)

#define SDRAM_BASE          (0x00000000)
#define SDRAM_WDG_BAK       (0x01000000 - 256)

#define STM32_I2C_SPO2      (0x70)
//#define STM32_I2C_ADDR      (0x60)//delete by QWB,20131218

/*********************************************************************************************************
** Function name:           system_check_init
** Descriptions:            system_check_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_check_init(void)
{

    return 0;
}

int system_check_cpu(void)
{
    //SYSTEM_CHECK_DBG_INFO("core clk = %d,system clk = %d\n",getCodeCLK(),getSystemCLK());
    
    if(CORE_CLOCK != getCodeCLK())
    {
        SYSTEM_CHECK_DBG_INFO("core clk error.");
        pSystemState->dsp | BIT_CPU;
    }
    if(SYSTEM_CLOCK != getSystemCLK())
    {
        SYSTEM_CHECK_DBG_INFO("system clk error.");
        pSystemState->dsp | BIT_CPU;        
    }
    
    return 0;
}

int system_check_flash(void)
{
    return 0;
}

int system_check_sdram(void)
{
    volatile u16 *ptr = (volatile u16 *)(SDRAM_BASE+2);
    u16 temp = *ptr;
    
    *ptr = 0xaaaa;
    if(*ptr != 0xaaaa)
    {
        SYSTEM_CHECK_DBG_INFO("system sdram error *ptr = 0x%.4X.\n",*ptr);
        pSystemState->dsp | BIT_SDRAM;        

    }
    *ptr = 0x5555;
    if(*ptr != 0x5555)
    {
        SYSTEM_CHECK_DBG_INFO("system sdram error *ptr = 0x%.4X.\n",*ptr);
        pSystemState->dsp | BIT_SDRAM;        

    }
    
    *ptr = temp;
    
    if(!(pSystemState->dsp & BIT_SDRAM))
    {
        //SYSTEM_CHECK_DBG_INFO("system sdram ok *ptr = 0x%.4X.\n",*ptr);

    }
    
    return 0;
}

int wait_watchdog_reset(void)
{
    int timeout = 1000;
    
    while(timeout--)
    {
        Delay1ms(1);
    }
    
    if(timeout > 1)
    {
        return 0;
    }
    
    return -1;
}

int system_check_watchdog(void)
{
    volatile u16 *ptr = (volatile u16 *)(SDRAM_WDG_BAK);
    u16 temp = *ptr;
    if(*ptr != 0xAA)
    {
        *ptr = 0xAA;
        if(wait_watchdog_reset() != 0)
        {
            SYSTEM_CHECK_DBG_INFO("system watchdog failed. *ptr = 0x%.4X.\n",*ptr);
            pSystemState->dsp | BIT_WDG;        
        }
    }
    
    return 0;
}

int system_check_eeprom(void)
{
    if(RETURN_ERROR == EEPROM_ExamineHeadTail())
    {
        SYSTEM_CHECK_DBG_INFO("system eeprom failed.\n");
        pSystemState->dsp | BIT_EEPROM;        
    }
    return 0;
}

int system_check_stm32(void)
{
    u8 buf;
    aio_stm32_readByID(POWER_STATUS, &buf, 1);
    pSystemState->power = buf;
    aio_stm32_readByID(SELF_CHECK, &buf, 1);
    pSystemState->stm32 = buf;
    return 0;
}

int system_check_ecg(void)
{
    pSystemState->ecg = 0;
    
    return 0;
}

int system_check_nibp(void)
{
    pSystemState->nibp = 0;
    
    return 0;
}

void uart_packet_transmit(const UART_PacketID id, const TRUE_OR_FALSE IsPassData, char* pData, const u8 lenght)
{
    UartProtocolPacket packet;
    static u8 s_packetNum = 0;
    
    packet.DR_Addr = UART_MCU_ADDR;
    packet.SR_Addr = UART_AIO_ADDR;
    packet.PacketID = id;

    if (TRUE == IsPassData)
    {
        packet.Length = lenght;
        memcpy(packet.DataAndCRC, pData, lenght);
    }
    packet.PacketNum = s_packetNum;
    packet.DataAndCRC[packet.Length] = CalculatePacketCRC8(&packet);
    
    AIO_UART_SendBuf((char *)&packet, packet.Length + PACKET_FIXED_LENGHT);
    s_packetNum++;


}

int system_check(void)
{
    system_check_cpu();
    system_check_flash();
    system_check_sdram();
    system_check_watchdog();
    system_check_eeprom();
    system_check_stm32();
    system_check_ecg();
    system_check_nibp();
    
    
    //UploadDataByID(COM_SELF_CHECK_ID, TRUE, (char *)pSystemState, 6);
    uart_packet_transmit(COM_SELF_CHECK_ID, TRUE, (char *)pSystemState, 6);
    
    return 0;
}


/*********************************************************************************************************
** Function name:           system_check_server
** Descriptions:            system_check_server
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_check_server(void)
{
    static u32 SystemCheckTime = 0;
    
    
    if(IsOnTime(SystemCheckTime))
    {
        SystemCheckTime += 1000;
        
        //AIO_printf("system_check_server.\n");
        //system_check();  
    }
    
    
    return 0;
}

#endif //__SYSTEM_CHECK_C_
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
