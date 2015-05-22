/*
 * eeprom.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
     
#include <string.h>

#include "eeprom.h"

//#define _IS24C08_
#define _GT24C16_

#define EEPROM_START_ADDR   0
#if defined(_IS24C08_)
#define EEPROM_SIZE         1024
#endif
#if defined(_GT24C16_)
#define EEPROM_SIZE         2048
#endif
#define EEPROM_END_ADDR     (EEPROM_SIZE - 1)
#define PAGE_WRITE_LENGHT   16
#define EEPROM_ADDRESS      0xA0    //bit[7:4]=1010,VendorIdentifier;bit[3](A2)=0
#if defined(_IS24C08_)
#define BLOCK_SELECT_MASK   0x03
#endif
#if defined(_GT24C16_)
#define BLOCK_SELECT_MASK   0x07
#endif
#define EEPROM_HEAD         "HEAD"
#define EEPROM_TAIL         "TAIL"

typedef struct
{
    u8 addr;
    u8 value;
} EEPROM_TWI_BYTE_Struct;

typedef struct
{
    u8 addr;
    u8 value[PAGE_WRITE_LENGHT];
} EEPROM_TWI_PAGE_Struct;

#pragma section("sdram0_bank3_page4")
EEPROM_StructureDef g_eeprom_struct;
#pragma section("sdram0_bank3_page4")
EEPROM_StructureDef* pEeprom_struct = &g_eeprom_struct;

RETURN_TypeDef EEPROM_SequentialRead(const u16 u16Addr, u8 *pData, const u16 u8Len);
RETURN_TypeDef EEPROM_BufferWrite(u16 u16Addr, u8 *pData, u16 u8Len);
RETURN_TypeDef EEPROM_PageWrite(const u16 u16Addr, u8 *pData, const u8 u8Len);

//<!-- ����һ����ִ�еĺ�����ȫ��������SDRAM(����Ĭ�ϵ�DATA SRAM) Stop -->
#pragma section("sdram0_bank3")
void EEPROM_Init(void);
#pragma section("sdram0_bank3")
void EEPROM_ExamineAll(void);
//<!-- ����һ����ִ�еĺ�����ȫ��������SDRAM(����Ĭ�ϵ�DATA SRAM) Stop -->

/*
  * ��������EEPROM_WaitStandbyState
  * ����  ��Wait for EEPROM Standby state
  * ����  ����
  * ���  ����
  * ����  ����
  * ����  ��
  */
void EEPROM_WaitStandbyState(void)
{
    Delay1ms(6);
}

/*******************************************************************************
 *  Eeprom write Api Begin
 ******************************************************************************/
RETURN_TypeDef EEPROM_ByteWrite(const u16 u16Addr, const u8 u8Data)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    EEPROM_TWI_BYTE_Struct sEepromTWI;
    
    if (u16Addr > EEPROM_END_ADDR)
    {
        return RETURN_ERROR;
    }
    
    sEepromTWI.addr= (u8)(u16Addr & 0xFF);
    sEepromTWI.value = u8Data;
    u8DeviceAddr |= (u8)(((u16Addr >> 8) & BLOCK_SELECT_MASK) << 1);//block select
    
    TWI_MasterMode_Write(u8DeviceAddr,
                        (unsigned char *)&sEepromTWI,
                        1,
                        2);
    return RETURN_OK;
}

RETURN_TypeDef EEPROM_PageWrite(const u16 u16Addr, u8 *pData, const u8 u8Len)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    EEPROM_TWI_PAGE_Struct sEepromTWI;
    
    if (u8Len > PAGE_WRITE_LENGHT)
    {
        AIO_printf("\r\nEEPROM_PageWrite over Lenght error!");
        return RETURN_ERROR;
    }
    sEepromTWI.addr= (u8)(u16Addr & 0xFF);
    memcpy(sEepromTWI.value, pData, u8Len);
    u8DeviceAddr |= (u8)(((u16Addr >> 8) & BLOCK_SELECT_MASK) << 1);//block select
    
    TWI_MasterMode_Write(u8DeviceAddr,
                        (unsigned char *)&sEepromTWI,
                        1,
                        1+u8Len);//1bytes Addr + Bytes Data lenght
    return RETURN_OK;
}

RETURN_TypeDef EEPROM_BufferWrite(u16 u16Addr, u8 *pData, u16 u8Len)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    EEPROM_TWI_PAGE_Struct sEepromTWI;
    u8 addr = u16Addr % PAGE_WRITE_LENGHT;//�鿴����ĵ�ַ�ǲ��� 16��������
    u8 count = PAGE_WRITE_LENGHT - addr;//��ʾ������һҳҳ�׵�ַ�ľ��루��������
    u8 numOfPage = u8Len / PAGE_WRITE_LENGHT;//���һ���ж���ҳ
    u8 numOfSinglePage = u8Len % PAGE_WRITE_LENGHT;//�������һҳ�����ݵ�����
    
    if (addr == 0)//�������ĵ�ַ����ҳ��ַ
    {
        if (numOfPage == 0)//�������һҳ����
        {
            EEPROM_PageWrite(u16Addr, pData, numOfSinglePage);
            EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
        }
        else//���������һҳ����
        {
            while(numOfPage--)//��һ��whileѭ����ִ��ҳдѭ���������ж���ҳ��д���ٴ�
            {
                EEPROM_PageWrite(u16Addr, pData, PAGE_WRITE_LENGHT);//����д��������I2C_PageSize������Ϊʵ��ִ��ҳд
                EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
                u16Addr += PAGE_WRITE_LENGHT;//ÿִ����һ��ҳд��Ӧ�ĵ�ַҲ��Ҫ��λ
                pData += PAGE_WRITE_LENGHT;//����ָ����λ
            }

            if (numOfSinglePage != 0)//����в���һҳ������������ִ��
            {
                EEPROM_PageWrite(u16Addr, pData, numOfSinglePage);
                EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
            }
        }
    }
    else//����ĵ�ַ������ҳ��ַ
    {
        //TODO,numOfSinglePage+addr > page size ??????????????
        if (numOfPage == 0)//�������һҳ����
        {
            EEPROM_PageWrite(u16Addr, pData, numOfSinglePage);
            EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
        }
        else//�����һҳ��һҳ����
        {
            /*����ַ������ȱʡλ�ò������ݣ����ݵĶ��پ��� count��ֵ��
            NumByteToWrite ������ֵ���ǲ�������֮��ʣ��δ���͵�����*/
            u8Len -= count;
            numOfPage = u8Len / PAGE_WRITE_LENGHT;//ʣ���ҳ��
            numOfSinglePage = u8Len % PAGE_WRITE_LENGHT;//����һҳ����������

            if (count != 0)//����ַ������ȱʡλ�ò�������
            {
                EEPROM_PageWrite(u16Addr, pData, count);//����д��������I2C_PageSize������Ϊʵ��ִ��ҳд
                EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
                u16Addr += count;//ÿִ����һ��ҳд��Ӧ�ĵ�ַҲ��Ҫ��λ
                pData += count;//����ָ����λ
            }

            while(numOfPage--)
            {
                EEPROM_PageWrite(u16Addr, pData, PAGE_WRITE_LENGHT);//����д��������I2C_PageSize������Ϊʵ��ִ��ҳд
                EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
                u16Addr += PAGE_WRITE_LENGHT;//ÿִ����һ��ҳд��Ӧ�ĵ�ַҲ��Ҫ��λ
                pData += PAGE_WRITE_LENGHT;//����ָ����λ
            }
            
            if (numOfSinglePage != 0)//����в���һҳ������������ִ��
            {
                EEPROM_PageWrite(u16Addr, pData, numOfSinglePage);
                EEPROM_WaitStandbyState();//�ȴ� EEPROM ��������ڲ�����
            }
        }
    }
    return RETURN_OK;
}
/*******************************************************************************
 *  Eeprom write Api End
 ******************************************************************************/

/*******************************************************************************
 *  Eeprom Read Api Begin
 ******************************************************************************/
RETURN_TypeDef EEPROM_CurrentRead(u8 *pData)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    
    TWI_MasterMode_Read(u8DeviceAddr,
                        pData,
                        1);
    return RETURN_OK;
}

RETURN_TypeDef EEPROM_RandomRead(const u16 u16Addr, u8 *pData)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    u8 u8InterAddr = 0;
    
    
    if (u16Addr > EEPROM_END_ADDR)
    {
        return RETURN_ERROR;
    }
    
    u8InterAddr = (u8)(u16Addr & 0xFF);
    u8DeviceAddr |= (u8)(((u16Addr >> 8) & BLOCK_SELECT_MASK) << 1);//block select
    TWI_MasterMode_Write(u8DeviceAddr,
                        &u8InterAddr,
                        1,
                        1);
    TWI_MasterMode_Read(u8DeviceAddr,
                        pData,
                        1);
    return RETURN_OK;
}

RETURN_TypeDef EEPROM_SequentialRead(const u16 u16Addr, u8 *pData, const u16 u8Len)
{
    u8 u8DeviceAddr = EEPROM_ADDRESS;
    u8 u8InterAddr = 0;
    
    //When the memory address boundary of 1023 is reached,
    //the address counter ��rolls over�� to address 0.
    if ((u16Addr + u8Len ) > EEPROM_END_ADDR)
    {
        return RETURN_ERROR;
    }
    
    u8InterAddr = (u8)(u16Addr & 0xFF);
    u8DeviceAddr |= (u8)(((u16Addr >> 8) & BLOCK_SELECT_MASK) << 1);//block select
    TWI_MasterMode_Write(u8DeviceAddr,
                        &u8InterAddr,
                        1,
                        1);
    TWI_MasterMode_Read(u8DeviceAddr,
                        pData,
                        u8Len);
    return RETURN_OK;
}
/*******************************************************************************
 *  Eeprom Read Api End
 ******************************************************************************/

/*******************************************************************************
 *  Eeprom Api For App Begin
 ******************************************************************************/
//���ͷβ��־�Ƿ�һ��
RETURN_TypeDef EEPROM_ExamineHeadTail(void)
{
    if (0 != memcmp(pEeprom_struct->HeadOfEeprom, EEPROM_HEAD,strlen(EEPROM_HEAD)))
    {
        AIO_printf("\r\n[EEPROM] examine head error!");
        return RETURN_ERROR;
    }
    if (0 != memcmp(pEeprom_struct->TailOfEeprom, EEPROM_TAIL,strlen(EEPROM_TAIL)))
    {
        AIO_printf("\r\n[EEPROM] examine Tail error!");
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

//�ָ�ͷβ��ǩ
RETURN_TypeDef EEPROM_RestoreHeadTail(void)
{
    RETURN_TypeDef result = RETURN_ERROR;
    u16 u16Addr;
    
    memcpy(pEeprom_struct->HeadOfEeprom, EEPROM_HEAD,strlen(EEPROM_HEAD));
    memcpy(pEeprom_struct->TailOfEeprom, EEPROM_TAIL,strlen(EEPROM_TAIL));

    
    u16Addr = (char *)&(pEeprom_struct->HeadOfEeprom[0]) - (char *)pEeprom_struct;
    result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->HeadOfEeprom[0]), strlen(EEPROM_HEAD));

    u16Addr = (char *)&(pEeprom_struct->TailOfEeprom[0]) - (char *)pEeprom_struct;
    result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->TailOfEeprom[0]), strlen(EEPROM_TAIL));

    return result;
}

//��ȡEEPROM����ֵȫ�ֱ���
void EEPROM_Init(void)
{
    u16 nlen = (char *)&(pEeprom_struct->TailOfEeprom[0])+ sizeof(pEeprom_struct->TailOfEeprom) - (char *)pEeprom_struct;
    EEPROM_SequentialRead(0, (u8 *)pEeprom_struct, nlen);
}

//���������Ч��
void EEPROM_ExamineAll(void)
{
    if (RETURN_ERROR == EEPROM_ExamineHeadTail())
    {
        EEPROM_RestoreHeadTail();
    }
}

//��ȡEEPROM��Ѫ��У׼ֵ��ȫ�ֱ���
RETURN_TypeDef EEPROM_RefreshNIBPVerify(void)
{
    u16 u16Addr = 0;
    u8 u8Len = 0;
    RETURN_TypeDef result = RETURN_ERROR;

    //�����ڴ����
    u16Addr = (char *)&(pEeprom_struct->nibp_Verify_mmHg[0]) - (char *)pEeprom_struct;
    u8Len = (char *)&(pEeprom_struct->nibp_IsVerify) + sizeof(TRUE_OR_FALSE) - (char *)&(pEeprom_struct->nibp_Verify_mmHg[0]);
    result =  EEPROM_SequentialRead(u16Addr, (u8 *)(&(pEeprom_struct->nibp_Verify_mmHg[0])), u8Len);

    return result;
}

RETURN_TypeDef EEPROM_saveRAMSTM32NIBPVerify(u8 index, u16 *mmHg, u16 *adc)
{
    if (index < 2)
    {
        pEeprom_struct->nibp_protect_mmHg[index] = *mmHg;
        pEeprom_struct->nibp_protect_adc[index] = *adc;
        return RETURN_OK;
    }
    return RETURN_ERROR;
}

RETURN_TypeDef EEPROM_saveSTM32NIBPVerify(void)
{
    u16 u16Addr = 0;
    u8 u8Len = 0;
    RETURN_TypeDef result = RETURN_ERROR;

    //���ڴ����д��EEPROM����ȥ
    pEeprom_struct->chIsProtectVerify = 1;
    u16Addr = (char *)&(pEeprom_struct->nibp_protect_mmHg[0]) - (char *)pEeprom_struct;
    u8Len = &(pEeprom_struct->chIsProtectVerify) + sizeof(char) \
            - (char *)&(pEeprom_struct->nibp_protect_mmHg[0]);
    result =  EEPROM_BufferWrite(u16Addr, (u8 *)&(pEeprom_struct->nibp_protect_mmHg[0]), u8Len);
    return result;
}

//�ָ�EEPROM����ֵΪ0xFF
void EEPROM_RestoreAllBank(void)
{
    int i;
    u8 pData[16]={0xFF,0xFF,0xFF,0xFF,
                    0xFF,0xFF,0xFF,0xFF,
                    0xFF,0xFF,0xFF,0xFF,
                    0xFF,0xFF,0xFF,0xFF};
    for (i = 0; i < EEPROM_SIZE;)
    {
        EEPROM_BufferWrite(i, pData, 16);
        i += 16;
        Delay1ms(10);
    } 
    AIO_printf("\r\nEEPROM_Restore Done!");
}
/*******************************************************************************
 *  Eeprom Api For App End
 ******************************************************************************/

/*******************************************************************************
 <!-- Eeprom Api For Debug or Test Begin -->
 ******************************************************************************/
void EEPROM_WriteTestAllBank(void)
{
    int i;
    for (i = 0; i < EEPROM_SIZE; i++)
    {
        EEPROM_ByteWrite(i, i % 256);
        Delay1ms(10);
    }
    AIO_printf("\r\nEEPROM_WriteTest Done!");
}

//��ȡ����EEPROM��ֵ
void EEPROM_ReadAllBank(void)
{
    int i;
    u8 rData[PAGE_WRITE_LENGHT];
    AIO_printf("\r\nEEPROM_ReadAll Start!\n");
    for (i = 0; i < EEPROM_SIZE;)
    {
        EEPROM_SequentialRead(i, rData, PAGE_WRITE_LENGHT);
        AIO_printf("\r\n%03d:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
            "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",i,
            rData[0],rData[1],rData[2],rData[3],rData[4],rData[5],rData[6],rData[7],
            rData[8],rData[9],rData[10],rData[11],rData[12],rData[13],rData[14],rData[15]);
        i += PAGE_WRITE_LENGHT;
    }
    AIO_printf("\r\nEEPROM_ReadAll End!\n");
}

void EEPROM_TestInterface(const EEPROM_Debug_TypeDef type)
{
    u8 rData = 0, i;
    u8 ee_w_buf[PAGE_WRITE_LENGHT] = {0,};
    u8 ee_r_buf[PAGE_WRITE_LENGHT] = {0,};
    
    switch(type)
    {
    case _EEPROM_TEST_ALL_W_:
        EEPROM_WriteTestAllBank();
        break;
    case _EEPROM_TEST_ALL_R_://��Ҫ���������������ݣ���ֹ���ڱ���
        EEPROM_ReadAllBank();
        break;
    case _EEPROM_RESTORE_ALL_:
        EEPROM_RestoreAllBank();
        break;
    case _EEPROM_TEST_BYTE_:
        EEPROM_ByteWrite(100, 0x12);
        Delay1ms(10);
        EEPROM_RandomRead(100, &rData);
        AIO_printf("\r\nAddr=100 Val=0x%02X",rData);
        break;
    case _EEPROM_TEST_PAGE_WRITE_:
        for (i = 0; i < PAGE_WRITE_LENGHT; i++)
        {
            ee_w_buf[i] = i;
        }
        EEPROM_PageWrite(PAGE_WRITE_LENGHT/2, ee_w_buf, PAGE_WRITE_LENGHT);
        Delay1ms(10);
        break;
    case _EEPROM_TEST_PAGE_READ_:
        EEPROM_SequentialRead(PAGE_WRITE_LENGHT/2, ee_r_buf, PAGE_WRITE_LENGHT);
        for (i = 0; i < PAGE_WRITE_LENGHT; i++)
        {
            AIO_printf("0x%02X\t",ee_r_buf[i]);
            if (i%8 == 7)
            {
                AIO_printf("\r\n");
            }
        }
        break;
        
    default :
        break;
    }
}

//�жϵ�ַ�������Ƿ����
//ǰ������Ҫִ�й�:EEPROM_WriteTestAllBank()�������г�ʼ��EEPROM
void EEPROM_LoopTest(void)
{
    static unsigned long ulNextChangeTime = 0;
    static short int addr = 0;
    u8 rData = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        if (addr > EEPROM_END_ADDR)
        {
            addr = 0;
        }
        EEPROM_RandomRead(addr, &rData);
        if ((u8)(addr&0xFF) != rData)
        {
            AIO_printf("\r\n[EEPROM] Read [Addr]0x%04X [Value]0x%02X",addr,rData);
        }
        addr++;
        ulNextChangeTime += getTickNumberByMS(2000);
    }

}
/*******************************************************************************
 <!-- Eeprom Api For Debug or Test End -->
 ******************************************************************************/
 
