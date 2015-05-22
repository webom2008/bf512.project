#include <string.h>

#include "system_check.h"

//#define SYSTEM_CHECK_DEBUG_INFO

#ifdef SYSTEM_CHECK_DEBUG_INFO
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...) printf("\r\n[SYS_CHECK] "fmt, ##arg)
#else
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...)
#endif

#define FLASH_START_ADDR    0x08000000
#define FLASH_SIZE          0x10000     //64KByte
#define FLASH_CRC_U16_OFFSET            (FLASH_SIZE - sizeof(u16))
#define FLASH_APP_LENGHT_U32_OFFSET     (FLASH_CRC_U16_OFFSET - sizeof(u32))
#define SRAM_START_ADDR     0x20000000
#define SRAM_END_ADDR       0x20007FFF

#define BUF_SIZE            512
const u16 crcl6_table[16] = /* CRC-16s */
{
	0x0000, 0xCC01, 0xD801, 0x1400,
	0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0X6C00, 0x7800, 0xB401,
	0x5000, 0x9C01, 0x8801, 0x4400
};

u8 Sys_Self_Check_Status;

void Flash_Check(void);
void SRAM_Check(void);
void CPU_Check(void);

void SystemCore_Check(void)
{
    SRAM_Check();
    Flash_Check();
    CPU_Check();
}

/**
  * @brief  get crc16 value
  * @param  start, starting value
  * @param  const char *p,  points to chars to process
  * @retval int n, how many chars to process
  */
u16 GetCRC16(u16 start, const u8 *p, u16 n)
{

	u16 total = 0;
	u16 r1;
	total = start;
	/* process each byte */
	while (n-- > 0)
	{
		/* do the lower four bits */
		r1 = crcl6_table[total & 0x0F];
		total = (total >> 4) & 0x0FFF;
		total = total ^ r1 ^ crcl6_table[*p & 0x0F];
		/* do the upper four bits */
		r1 = crcl6_table[total & 0xF];
		total = (total >> 4) & 0x0FFF;
		total = total ^ r1 ^ crcl6_table[(*p >> 4) & 0x0F];
		/* advance to next byte */
		p++;
	}
	return total;
}

static void Flash_Check(void)
{
#if 0
    u16 crc = 0;
    u32 offset = 0;
    u16 length = 0;
    u16 buffer[BUF_SIZE];
    
    u32 APP_CODE_LEN = *(u32 *)(FLASH_START_ADDR+FLASH_APP_LENGHT_U32_OFFSET);
    u16 APP_CRC = *(u16 *)(FLASH_START_ADDR+FLASH_CRC_U16_OFFSET);
    
    memset(buffer, 0, sizeof(buffer));

    if (APP_CODE_LEN > (FLASH_SIZE-sizeof(u16)-sizeof(u32)))
    {
        return;
    }
    for (offset = 0; offset < APP_CODE_LEN; offset += sizeof(buffer))
    {
        memcpy(buffer, (u16 *)(FLASH_START_ADDR+offset), sizeof(buffer));
        if ((offset + sizeof(buffer)) > APP_CODE_LEN)
        {
            length = APP_CODE_LEN-offset;
        }
        else
        {
            length = sizeof(buffer);
        }
        crc = GetCRC16(crc, (u8 *)buffer, length); 
    }
    if (APP_CRC == crc)
    {
        RESETBIT(Sys_Self_Check_Status, 2);
    }
    SYSTEM_CHECK_DBG_INFO("sizeof(buffer)=%d\tFileZise=0x%.4X",sizeof(buffer), 
                            offset-sizeof(buffer)+length);
    SYSTEM_CHECK_DBG_INFO("crc=0x%.4X flashCRC=0x%.4X", crc, APP_CRC);
#else
    u16 crc = 0;
    u32 APP_CODE_LEN = *(u32 *)(FLASH_START_ADDR+FLASH_APP_LENGHT_U32_OFFSET);
    u16 APP_CRC = *(u16 *)(FLASH_START_ADDR+FLASH_CRC_U16_OFFSET);
    if (APP_CODE_LEN > (FLASH_SIZE-sizeof(u16)-sizeof(u32)))
    {
        SYSTEM_CHECK_DBG_INFO("APP_CODE_LEN = 0x%.6X", APP_CODE_LEN);
        return;
    }
    crc = GetCRC16(crc, (u8 *)(FLASH_START_ADDR), APP_CODE_LEN);
    if (APP_CRC == crc)
    {
        RESETBIT(Sys_Self_Check_Status, 2);
    }
    SYSTEM_CHECK_DBG_INFO("crc = 0x%.4X, flashCRC = 0x%.4X", crc, APP_CRC);

#endif
}

static void SRAM_Check(void)
{
    /* Enable PWR and BKP clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    /* Enable write access to Backup domain */
    PWR_BackupAccessCmd(ENABLE);
    /* Clear Tamper pin Event(TE) pending flag */
    BKP_ClearFlag();
    if(BKP_ReadBackupRegister(BKP_DR2) == 0)
    {
        RESETBIT(Sys_Self_Check_Status, 1);
    }
    SYSTEM_CHECK_DBG_INFO("SRAM ERROR_COUNT=0x%.4x",BKP_ReadBackupRegister(BKP_DR2));
}

static void CPU_Check(void)
{
    RESETBIT(Sys_Self_Check_Status, 3);
}
