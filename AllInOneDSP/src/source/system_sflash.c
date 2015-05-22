/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE electronics Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           system_sflash.c
** Last modified Date:  2013-12-02
** Last Version:        V1.0
** Descriptions:        system_sflash
**
**--------------------------------------------------------------------------------------------------------
** Created by:          qiuweibo
** Created date:        2013-09-12
** Version:             V1.00
** Descriptions:        system_sflash
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-12-02
** Version:             V1.00
** Descriptions:        system_sflash
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
#include "system_sflash.h"

#ifdef __SYSTEM_SFLASH_C_

#define SYSTEM_CHECK_DEBUG_INFO

#ifdef SYSTEM_CHECK_DEBUG_INFO
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...) AIO_printf("\r\n[SYSTEM_CHECK] "fmt, ##arg)
#else
#define SYSTEM_CHECK_DBG_INFO(fmt, arg...)
#endif

#pragma section("sdram0_bank3")
flash_device g_flash_dev;
flash_device *pFlash_dev = &g_flash_dev;

#define FLASH_BUFFER_SIZE       (256)
#pragma section("sdram0_bank3")
u8 flash_rx_buf[FLASH_BUFFER_SIZE];
#pragma section("sdram0_bank3")
u8 flash_tx_buf[FLASH_BUFFER_SIZE];

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
int system_sflash_init(void);
#pragma section("sdram0_bank3")
void system_flash_device_init(flash_device *pdev);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->


#define FLASH_CMD_WREN          (0x06)
#define FLASH_CMD_WRDI          (0x04)
#define FLASH_CMD_RDSRL         (0x05)
#define FLASH_CMD_RDSRH         (0x35)
#define FLASH_CMD_WRSR          (0x01)
#define FLASH_CMD_READ          (0x03)
#define FLASH_CMD_FAST_READ     (0x0B)
#define FLASH_CMD_PP            (0x02)
#define FLASH_CMD_SE            (0x20)
#define FLASH_CMD_BE_32K        (0x52)
#define FLASH_CMD_BE_64K        (0xD8)
#define FLASH_CMD_CE            (0x60)
#define FLASH_CMD_DP            (0xB9)
#define FLASH_CMD_RDI           (0xAB)
#define FLASH_CMD_REMS          (0x90)
#define FLASH_CMD_RDID          (0x9F)
#define FLASH_CMD_HPM           (0xA3)
#define FLASH_CMD_CRMR          (0xFF)
#define FLASH_CMD_PES           (0x75)
#define FLASH_CMD_PER           (0x7A)
#define FLASH_CMD_ESR           (0x44)
#define FLASH_CMD_PSR           (0x42)
#define FLASH_CMD_RSR           (0x48)

#define FLASH_BIT_SUS           (1<<15)
#define FLASH_BIT_CMP           (1<<14)
#define FLASH_BIT_LB            (1<<10)
#define FLASH_BIT_QE            (1<<9)
#define FLASH_BIT_SRP           (1<<7)
#define FLASH_BIT_BP4           (1<<6)
#define FLASH_BIT_BP3           (1<<5)
#define FLASH_BIT_BP2           (1<<4)
#define FLASH_BIT_BP1           (1<<3)
#define FLASH_BIT_BP0           (1<<2)
#define FLASH_BIT_WEL           (1<<1)
#define FLASH_BIT_WIP           (1<<0)


#define FLASH_PAGE_SIZE         (256)
#define FLASH_SECTOR_SIZE       (4096)
#define FLASH_BLOCK_32K_SIZE    (32*1024)
#define FLASH_BLOCK_64K_SIZE    (64*1024)


/*********************************************************************************************************
** Function name:           SFlash_Init
** Descriptions:            SFlash_Init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void SFlash_Init(void)
{

}

/*********************************************************************************************************
** Function name:           SFlash_SelfCheck
** Descriptions:            SFlash_SelfCheck
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void SFlash_SelfCheck(void)
{

}

/*********************************************************************************************************
** Function name:           system_flash_device_init
** Descriptions:            system_flash_device_init
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void system_flash_device_init(flash_device *pdev)
{
    pdev->rx_buf = flash_rx_buf;
    pdev->tx_buf = flash_tx_buf;
    pdev->rx_len = 0;
    pdev->tx_len = 0;
    
}

/*********************************************************************************************************
** Function name:           system_sflash_init
** Descriptions:            system_sflash_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_sflash_init(void)
{
    system_flash_device_init(pFlash_dev);
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_id
** Descriptions:            system_flash_id
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_id(void)
{
    u8 cmd[1];
    u8 val[3];
    
    cmd[0] = FLASH_CMD_RDID;
    system_spi_read_buf(cmd,1,val,3);
    AIO_UART_SendBuf((char *)val, 3);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_status_high
** Descriptions:            system_flash_status_high
** input parameters:        none
** output parameters:       none
** Returned value:          status
*********************************************************************************************************/
u8 system_flash_status_high(void)
{
    u8 cmd[1];
    u8 status[1];
    
    cmd[0] = FLASH_CMD_RDSRH;
    system_spi_read_buf(cmd,1,status,1);
        
    return status[0];
}

/*********************************************************************************************************
** Function name:           system_flash_status_low
** Descriptions:            system_flash_status_low
** input parameters:        none
** output parameters:       none
** Returned value:          status
*********************************************************************************************************/
u8 system_flash_status_low(void)
{    
    u8 cmd[1];
    u8 status[1];
    
    cmd[0] = FLASH_CMD_RDSRL;
    system_spi_read_buf(cmd,1,status,1);
        
    return status[0];
}

/*********************************************************************************************************
** Function name:           system_flash_status
** Descriptions:            system_flash_status
** input parameters:        none
** output parameters:       none
** Returned value:          status
*********************************************************************************************************/
u16 system_flash_status(void)
{
    u16 status = 0;
    status = (system_flash_status_high()<<8) | system_flash_status_low();
    return status;
}

/*********************************************************************************************************
** Function name:           system_flash_wait
** Descriptions:            system_flash_wait
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_wait(void)
{
    int timeout = 1000000;
    while((system_flash_status_low() & FLASH_BIT_WIP) && (timeout--) )
    {
        delay_us(1);
    }
    
    if(timeout > 1)
    {
        return 0;
    }
    else
    {
        SYSTEM_CHECK_DBG_INFO("system_flash_wait timeout.");
        return -1;
    }
}

/*********************************************************************************************************
** Function name:           system_flash_write_enable
** Descriptions:            system_flash_write_enable
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_write_enable(void)
{
    u8 cmd[1];
    cmd[0] = FLASH_CMD_WREN;
    system_spi_write_buf(cmd,1,NULL,0);
    
    return 0;

}

/*********************************************************************************************************
** Function name:           system_flash_write_enable
** Descriptions:            system_flash_write_enable
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_power_down(u8 mode)
{
    u8 cmd[1];
    
    system_flash_wait();
    if(mode == ENABLE)
    {
        cmd[0] = FLASH_CMD_DP;
        system_spi_write_buf(cmd,1,NULL,0);
    }
    else if(mode == DISABLE)
    {
        cmd[0] = FLASH_CMD_RDI;
        system_spi_write_buf(cmd,1,NULL,0);
    
    }
    
    return 0;

}



/*********************************************************************************************************
** Function name:           system_flash_erase_sector
** Descriptions:            system_flash_erase_sector
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_erase_sector(u32 addr)
{
    u8 cmd[5];
    
    system_flash_wait();
    system_flash_write_enable();
    
    cmd[0] = FLASH_CMD_SE;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    system_spi_write_buf(cmd,4,NULL,0);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_erase_block
** Descriptions:            system_flash_erase_block
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_erase_block(u32 addr)
{
    u8 cmd[5];
    
    system_flash_wait();
    system_flash_write_enable();
    cmd[0] = FLASH_CMD_BE_32K;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    system_spi_write_buf(cmd,4,NULL,0);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_erase_chip
** Descriptions:            system_flash_erase_chip
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_erase_chip(u32 addr)
{
    u8 cmd[1];
    
    system_flash_wait();
    system_flash_write_enable();
    cmd[0] = FLASH_CMD_CE;
    system_spi_write_buf(cmd,1,NULL,0);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_erase
** Descriptions:            system_flash_erase
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_erase(u32 addr,u32 len)
{
    int i;
    u32 address = addr;
    u32 length = len;
    
    for(i=0; i<(len/FLASH_BLOCK_32K_SIZE); i++)
    {
        system_flash_erase_block(address);
        address = address+FLASH_BLOCK_32K_SIZE;
        length = length - FLASH_BLOCK_32K_SIZE;
    }
    
    for(i=0; i<(length/FLASH_SECTOR_SIZE)+1; i++)
    {
        system_flash_erase_block(address);
        address = address+FLASH_SECTOR_SIZE;
    }
    
    return 0;
}


/*********************************************************************************************************
** Function name:           system_flash_write_byte
** Descriptions:            system_flash_write_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_write_byte(u32 addr,u8 data)
{
    u8 cmd[5];
    
    system_flash_wait();
    system_flash_write_enable();
    
    cmd[0] = FLASH_CMD_PP;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    system_spi_write_buf(cmd,4,&data,1);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_write_page
** Descriptions:            system_flash_write_page
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_write_page(u32 addr,u8 *buf)
{
    
    u8 cmd[5];
    
    system_flash_wait();
    system_flash_write_enable();
        
    cmd[0] = FLASH_CMD_PP;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    
    system_spi_write_buf(cmd,4,buf,FLASH_PAGE_SIZE);
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_write_buf
** Descriptions:            system_flash_write_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_write_buf(u32 addr,u8 *buf,u16 len)
{
    u32 address = addr;
    u32 length = 0;
    
    int i;
    for(i=0; i<(len/FLASH_PAGE_SIZE); i++)
    {
        system_flash_write_page(address,&buf[length]);
        address = address+FLASH_PAGE_SIZE;
        length = length+FLASH_PAGE_SIZE;
    
    }
    for(i=0; i<(len%FLASH_PAGE_SIZE); i++)
    {
        system_flash_write_byte(address,buf[length]);
        address++;
        length++;    
    }
    
    return 0;
}

/*********************************************************************************************************
** Function name:           system_flash_read_byte
** Descriptions:            system_flash_read_byte
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
u8 system_flash_read_byte(u32 addr)
{
    u8 cmd[5];
    u8 val[1] = {0};
    
    system_flash_wait();

    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    system_spi_read_buf(cmd,4,val,1);
    AIO_UART_SendBuf((char *)val, 1);
    
    return val[0];
}

/*********************************************************************************************************
** Function name:           system_flash_read_buf
** Descriptions:            system_flash_read_buf
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_flash_read_buf(u32 addr,u8 *buf,u16 len)
{    
    u8 cmd[5];
    
    system_flash_wait();

    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (addr>>16)&0xff;
    cmd[2] = (addr>>8)&0xff;
    cmd[3] = (addr)&0xff;
    system_spi_read_buf(cmd,4,buf,len);
    AIO_UART_SendBuf((char *)buf, len);
    
    return 0;
}

#define FLASH_TEST_ADDR_512K        (0x00080000)
#define FLASH_TEST_ADDR_1024K       (0x00100000)

/*********************************************************************************************************
** Function name:           system_sflash_server
** Descriptions:            system_sflash_server
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int system_sflash_server(void)
{
    static u32 SystemCheckTime = 0;
    static u8 Toggle = 1;
    
    if(IsOnTime(SystemCheckTime))
    {
        SystemCheckTime += 500;
        //AIO_printf("system_sflash_server.\n");
        //system_flash_id(void);
        //system_flash_status(pFlash_dev);
        //system_flash_read_byte(0x00);
        //system_flash_power_down(DISABLE);
        
        if(Toggle)
        {
            //system_flash_read_buf(FLASH_TEST_ADDR_512K,pFlash_dev->rx_buf,FLASH_BUFFER_SIZE);
        }
        else
        {
            //system_flash_erase(FLASH_TEST_ADDR_512K,FLASH_BUFFER_SIZE);
            //memset(pFlash_dev->tx_buf,0xAA,128);
            //system_flash_write_buf(FLASH_TEST_ADDR_512K,pFlash_dev->tx_buf,FLASH_BUFFER_SIZE);
        }
        Toggle = !Toggle;
    }
    
    
    return 0;
}

#endif //__SYSTEM_SFLASH_C_
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
