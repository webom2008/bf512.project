/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/src/commands.c 
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   This file provides the different commands' routines.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "commands.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern uint8_t opcode;
extern EventStatus i2c_event;
uint8_t  I2C_CH_Buffer_Tx[1200];
uint16_t I2C_CH_Buffer_Rx[1200];
uint8_t op_result;
extern __IO uint16_t Tx_Idx , Rx_Idx;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Read opcode command transferred from the Master.
  * @param  None
  * @retval - The received opcode
  */
uint8_t Read_Opcode(void)
{
    /* Private variables ---------------------------------------------------------*/
    __IO uint8_t Opcode;
    Opcode = I2C_CH_Buffer_Rx[0];
    return (Opcode);
}

/**
  * @brief  This function reads the Address memory  transferred from
  *   Master.
  * @param  None
  * @retval -The address transfered from the Master
  */
uint32_t Read_Add(void)
{
    uint32_t Add_High1,Add_High0,Add_Low1,Add_Low0,Add_High,Add_Low,Add;

    Add_High1 = I2C_CH_Buffer_Rx[1] ;
    Add_High0 = I2C_CH_Buffer_Rx[2];
    Add_Low1  = I2C_CH_Buffer_Rx[3];
    Add_Low0  = I2C_CH_Buffer_Rx[4];

    Add_High1 = Add_High1 << 24;
    Add_High0 = Add_High0 << 16;
    Add_Low1 = Add_Low1 << 8;
    Add_High = Add_High1 | Add_High0;
    Add_Low =Add_Low1 | Add_Low0;
    Add= Add_High | Add_Low;
    return(Add);
}


/**
  * @brief Read the number of bytes to be read or written/Or the number
  *   of pages to be erased.
  * @param  None
  * @retval -the number of data bytes or pages
  */
uint16_t Read_Byte_Page_Number(void)
{
    /* Private variables ---------------------------------------------------------*/
    uint16_t Numbr_HL = 0x0000, Numbr_H = 0x0000, Numbr_L = 0x0000;

    Numbr_H = I2C_CH_Buffer_Rx[5] ;
    Numbr_L = I2C_CH_Buffer_Rx[6];

    Numbr_H= Numbr_H << 8;
    Numbr_HL=Numbr_H|Numbr_L ;
    return(Numbr_HL);
}

/**
  * @brief  This function ensures the read_memory_command including the reading add,
  *   number of data to read and the transfer of data from STM32 to host
  * @param none
  * @retval none
  */
void Read_Memory_Command(void)
{
    /* Private variables ---------------------------------------------------------*/
#if 0
    uint32_t *Add_Flash;
    uint16_t Byte_Number;
    uint16_t Index=0;
    uint8_t *Aux_Add;
#endif
    __IO uint32_t Temp;
    uint32_t r_select;

    /* wait until receiving opcode + address + number of data */
    while (Rx_Idx!= 0);
    I2C_ITConfig(I2C_CH, I2C_IT_EVT, DISABLE);
#if 0 //add by QWB
    /* Address reading */
    Add_Flash = (uint32_t*)Read_Add();
    /* reading the number of data to be read */
    Byte_Number = Read_Byte_Page_Number();

    for (Index=0; Index<(Byte_Number) ;Index++)
    {
        Aux_Add = (uint8_t*)Add_Flash;
        I2C_CH_Buffer_Tx[Index] = *(Aux_Add + Index) ;
    }
#else //repond TAG(=0xAA55) to AIO, means ready to download new user app.
    
    r_select = Read_Add();
    if (0 == r_select)
    {
        I2C_CH_Buffer_Tx[0] = 0xAA;
        I2C_CH_Buffer_Tx[1] = 0x55;
    }
    if (1 == r_select)
    {
        I2C_CH_Buffer_Tx[0] = op_result;
        op_result = 0; //reset value
    }
#endif
    /* Enable  the I2C_IT_EVT after the I2C_CH_Buffer_Tx is fully filled */
    while (!I2C_GetFlagStatus(I2C_CH, I2C_FLAG_ADDR));
    /* Enable EVT IT in order to launch data transmission */
    I2C_ITConfig(I2C_CH, I2C_IT_EVT, ENABLE);
    /* Clear ADDR flag by reading SR2 register */
    Temp = I2C_CH->SR2 ;
    while (Tx_Idx !=0);
    /* Reset opcode */
    //printf("\r\nR(%d)",r_select);
    opcode=0;
    printf("1");
}

/**
  * @brief  This function writes a number of data beginning from a provided Flash address.
  * @param none
  * @retval none
  */
void Write_Memory_Command(void)
{
    uint16_t WriteCounter= 0;
    uint16_t Number_Bytes_Transferred = 0;
    uint32_t Add_Flash,DATA_SIZE_IN_PAGE = 0;
    uint16_t Idx = 0;
    uint32_t timeout = 0x10000;
    //uint32_t RamSource;
    static char dd = 0;  //debug
    /* Wait until receiving Flash address to  write into + number of data to be written 
      + the data to be written */
    while ((Rx_Idx!= 0) && --timeout);
    if (!timeout)
    {
        printf("\r\nW_E1");
        opcode=0;
        op_result = 0;
        return;
    }

    /* Read the address  */
    Add_Flash = Read_Add();

    /* Read the number of bytes to be written  */
    Number_Bytes_Transferred = Read_Byte_Page_Number();
    
    //printf("\r\nW:A(%X)L(%d)",Add_Flash,Number_Bytes_Transferred);
    //printf(">W");
    if (Number_Bytes_Transferred <= PAGE_SIZE)
    {
        DATA_SIZE_IN_PAGE = 1;
    }
    else
    {
        if ((Number_Bytes_Transferred%PAGE_SIZE)==0)
        {
            DATA_SIZE_IN_PAGE= Number_Bytes_Transferred/PAGE_SIZE;
        }
        else
        {
            DATA_SIZE_IN_PAGE=(uint32_t) (Number_Bytes_Transferred/PAGE_SIZE)+1;
        }
    }

    if ((Add_Flash%PAGE_SIZE)==0)//Add by QWB 128Byte Data per one time
    {
        Erase_Page(Add_Flash,DATA_SIZE_IN_PAGE);
    }
#if 1
    for (Idx=4; Idx<(Number_Bytes_Transferred+8)/2; Idx++)
    {
        I2C_CH_Buffer_Rx[2*Idx+1]=I2C_CH_Buffer_Rx[2*Idx+1]<<8;
        I2C_CH_Buffer_Rx[2*Idx]=I2C_CH_Buffer_Rx[2*Idx]|I2C_CH_Buffer_Rx[2*Idx+1];
    }

    for (WriteCounter = 0; (WriteCounter < (Number_Bytes_Transferred)/2 ); WriteCounter++)
    {
        FLASH_ProgramHalfWord((Add_Flash+2*WriteCounter), I2C_CH_Buffer_Rx[2*(WriteCounter+4)]);
    }
#else //reference from uart-bootloadr write for word
    for (WriteCounter = 0;
          WriteCounter < Number_Bytes_Transferred;
          WriteCounter += 4)
    {
        RamSource = (uint32_t)((I2C_CH_Buffer_Rx[8+WriteCounter]&0xFF) \
                                | ((I2C_CH_Buffer_Rx[9+WriteCounter]&0xFF)<<8) \
                                | ((I2C_CH_Buffer_Rx[10+WriteCounter]&0xFF)<<16) \
                                | ((I2C_CH_Buffer_Rx[11+WriteCounter]&0xFF)<<24));
        /* Program the data received into STM32F10x Flash */
        FLASH_ProgramWord(Add_Flash, RamSource);

        if (*(uint32_t*)Add_Flash != RamSource)
        {
            printf("\r\nW_E2");
            opcode=0;
            op_result = 0;
            return;
        }
        Add_Flash += 4;
    }
#endif
    if (dd) //debug
    {
        //dd = 0;
        printf("\r\nAdd_Flash = 0x%08X",Add_Flash);
        printf("\r\n");
        for (WriteCounter = 0; WriteCounter < Number_Bytes_Transferred; WriteCounter++)
            printf("%02X ",*(__IO uint8_t*)(Add_Flash+WriteCounter));
        printf("\r\n");
    }
    printf(" %04X ",*(__IO uint16_t*)(0x08004000));
    opcode=0;
    op_result = 1;
    //printf("<");
    printf("2");
}



/**
  * @brief   Performs erase page based on the Flash address and
  * the number of pages to be erased.
  * @param ADD_FLASH: adress in the first page to be erased
  * @param page_number: the number of pages to be erased.
  * @retval None
  */
void Erase_Page(uint32_t Add_Flash,uint16_t Page_Number)
{
    /* Private variables ---------------------------------------------------------*/
    __IO uint32_t EraseCounter = 0x00;
    uint32_t BASE_ADDRESS_PAGE;
    __IO uint32_t page_index;

    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    /* Erase the FLASH pages */

    page_index =(uint32_t) (Add_Flash-0x08000000)/PAGE_SIZE;
    BASE_ADDRESS_PAGE=(uint32_t)(0x08000000 + page_index * PAGE_SIZE);
    printf("\r\n%d",Page_Number);
    for (EraseCounter=0;(EraseCounter < Page_Number);EraseCounter++ )
    {
        FLASH_ErasePage(BASE_ADDRESS_PAGE+EraseCounter*PAGE_SIZE);
    }
    printf(">3.1");
}


/**
  * @brief   Erase page command.
  * @param none
  * @retval :   none
  */
void Erase_Page_Command(void)
{
    uint16_t Page_Number;
    uint32_t Add_Flash;
    printf("3");
    /* Read the Flash  address */
    Add_Flash= (uint32_t)Read_Add();
    /* Read the number of page to be erased*/
    Page_Number = Read_Byte_Page_Number();
    /* Erase the corresponding page(s) */
    Erase_Page(Add_Flash,Page_Number);
    /* Reset opcode */
    opcode=0;
    op_result = 1;
    printf("\r\nE:A(0x%08X)P(%d)",Add_Flash,Page_Number);
}



/**
  * @brief  Erase the user space memory (from 0x8001000 to the
  *   flash memory end address)".
  * @param  none.
  * @retval :  none.
  */
void User_Space_Memory_Erase_Command(void)
{

    /* Erase the user space memory */
    Erase_Page(USER_START_ADDRESS,(TOTAL_PAGE_NUMBER-((uint32_t) (USER_START_ADDRESS-0x08000000)/PAGE_SIZE)));
    opcode=0;
}

/************************END OF FILE****/

