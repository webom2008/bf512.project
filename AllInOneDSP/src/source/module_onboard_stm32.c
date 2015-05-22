/*
 * module_onboard_stm32.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "module_onboard_stm32.h"

//#define _STM32_W_
//#define _STM32_R_

#define STM32_I2C_ADDR    0x60

#define AIO_STM32_RESET_PIN     PH4

#define AIO_STM32_READ_MASK     (0x01)
#define AIO_STM32_WRITE_MASK    (0x00)

#define  OPC_READ       (uint8_t)(0x03)     //IAP I2C Read command
#define  OPC_WREN       (uint8_t)(0x06)     //IAP I2C Write command
#define  OPC_ERPG       (uint8_t)(0x20)     //IAP I2C Erase Page command
#define  OPC_ERUSM      (uint8_t)(0x60)     //IAP I2C Erase User Space Memory command
#define  OPC_USRCD      (uint8_t)(0x77)     //IAP I2C Go To User Code command
#define  DUM_BYTE       (uint8_t)(0xFF)

#define STM_PAGE_SIZE                         (0x400)       // 1 Kbyte
#define STM_FLASH_SIZE                        (0x10000)     //64 K
#define STM_BOOT_SIZE    	                  (0x4000)      //16 Kbyte
#define STM_TOTAL_PAGE_NUMBER    		    ((uint16_t)0x40)//64 PAGES
#define STM_TOTAL_USER_PAGE_NUMBER    	    ((uint16_t)0x30)//48 PAGES

#define STM_USER_START_ADDRESS              ((u32)0x08004000)

#define STM_UPDATE_BUF_LEN       (1200)


#pragma section("sdram0_bank3")
u8 stm32_sw_update_buf[STM_UPDATE_BUF_LEN];
unsigned char g_stm32_mask_count;
unsigned char g_is_aio_stm_error;

const static u8 MASTER_UPDATE_TAG[11] = {0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00};
const static u8 SLAVER_UPDATE_TAG[11] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA};

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void OnBoardSTM32_Module_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->


#define _DEF_STM32_INFO_
#ifdef _DEF_STM32_INFO_
#define STM32_DBG(fmt, arg...) AIO_printf("\r\n[STM32] "fmt, ##arg)
#else
#define STM32_DBG(fmt, arg...)
#endif

static void STM32_ResetIO_Init(void)
{
	/* setup PH4 as an output */
	*pPORTHIO_INEN &= ~AIO_STM32_RESET_PIN;			/* disable */
	*pPORTHIO_DIR |= AIO_STM32_RESET_PIN;			/* output */

	/* clear interrupt settings */
	*pPORTHIO_EDGE &= ~AIO_STM32_RESET_PIN;
    *pPORTHIO_MASKA_CLEAR = AIO_STM32_RESET_PIN;

    /* now clear the flag */
	*pPORTHIO_SET = AIO_STM32_RESET_PIN;
}

void STM32_Reset(void)
{
    *pPORTHIO_CLEAR = AIO_STM32_RESET_PIN;
    Delay1ms(10);
    *pPORTHIO_SET = AIO_STM32_RESET_PIN;
}

void OnBoardSTM32_Module_Init(void)
{
    g_stm32_mask_count = UPDATE_MASK_AIO_TIME_S +1;
    g_is_aio_stm_error = 0;
    
    STM32_ResetIO_Init();
}

//@return : < 0 -- error, > 0 --success
int aio_stm32_writeByID(I2C_PacketID id, u8 *pWBuf, u8 nBufLen)
{
    int result = 0;
    if (nBufLen > I2C_PACKET_VAL_MAX_LEN)
    {
        return -1;
    }
    I2CProtocolPacket packet;
    packet.ID = (unsigned char)(id << 1) | AIO_STM32_WRITE_MASK;
    packet.Lenght = nBufLen;
    memset(packet.pValue, 0, nBufLen);
    memcpy(packet.pValue, pWBuf, nBufLen);
    result = TWI_MasterMode_Write(STM32_I2C_ADDR, (u8 *)&packet, 1, (2 + packet.Lenght));
    return result;
}

//@return : < 0 -- error, > 0 --success
int aio_stm32_readByID(I2C_PacketID id, u8 *pRBuf, u8 nBufLen)
{
    int result = 0;
    static int aio_stm32_comm_fail_cnt = 0;
    if (nBufLen > I2C_PACKET_VAL_MAX_LEN)
    {
        return -1;
    }
    unsigned char u8ID;
    u8ID = (unsigned char)(id << 1) | AIO_STM32_READ_MASK;
    
    TWI_MasterMode_Write(STM32_I2C_ADDR, &u8ID, 1, 1);
    result = TWI_MasterMode_Read(STM32_I2C_ADDR, pRBuf, nBufLen);
    if (result < 0)
    {
        if (aio_stm32_comm_fail_cnt++ > 10)
        {
            g_is_aio_stm_error = 1;
        }
    }
    else
    {
        aio_stm32_comm_fail_cnt = 0;
        g_is_aio_stm_error = 0;
    }
    return result;
}

static void aio_stm32_ResetMaskAIOTick(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        if (0 == g_stm32_mask_count)
        {
            g_IsECG_Upload = false;
            g_IsRESP_Upload = false;
            g_IsTemp_Upload = false;
            g_IsCommon_Upload = false;
            g_IsNIBP_Upload = false;
            g_IsSpO2_Upload = false;
        }
        if (UPDATE_MASK_AIO_TIME_S == g_stm32_mask_count)
        {
            g_IsECG_Upload = true;
            g_IsRESP_Upload = true;
            g_IsTemp_Upload = true;
            g_IsCommon_Upload = true;
            g_IsNIBP_Upload = true;
            g_IsSpO2_Upload = true;
        }
        if (g_stm32_mask_count++ > UPDATE_MASK_AIO_TIME_S)
        {
            g_stm32_mask_count = UPDATE_MASK_AIO_TIME_S + 1;
        }
        ulNextChangeTime += getTickNumberByMS(1000);
    }
}

/*******************************************************************************
 <!-- STM32 I2C Api For Software Update (STM32_I2C_IAP) Start -->
 ******************************************************************************/

/**
  * @brief  This function enter stm32 bootloader ready to software update
  * @param  note
  * @retval     <0 : error
  *             >0 : normal operation
  */
static int aio_stm32_enter_boot(void)
{
    int result = -1;
    u8 buf = 0xAA;
    result = aio_stm32_writeByID(SOFTWARE_UPDATE, &buf, 1);
    return result;
}

/**
  * @brief  This function read operation when STM in bootloader mode
  * @param  *pAddr : flash inter address to be reading
  * @param  *nByte : the number of byte to be reading
  * @param  *pData : the read date where to save.
  * @retval     -1 : read number of byte error
  *             >0 : normal operation
  * @note   pData[0] = 0xAA && pData[1] = 0x55 means in bootloader, else in user
  *         code.
  */
static int stm_update_read_op(u32 *pAddr, u16 *nByte, u8 *pData)
{
    /*
    if (2 != *nByte) //only support data[0]=0xAA data[1]=0x55
    {
        return -1; //read number of byte error
    }
    */

    int result = 0;
    stm32_sw_update_buf[0] = OPC_READ;
    
    stm32_sw_update_buf[1] = (*pAddr >> 24) & 0xFF;
    stm32_sw_update_buf[2] = (*pAddr >> 16) & 0xFF;
    stm32_sw_update_buf[3] = (*pAddr >> 8) & 0xFF;
    stm32_sw_update_buf[4] = (*pAddr >> 0) & 0xFF;
    
    stm32_sw_update_buf[5] = (*nByte >> 8) & 0xFF;
    stm32_sw_update_buf[6] = (*nByte >> 0) & 0xFF;
    
    result = TWI_MasterMode_Write(STM32_I2C_ADDR, stm32_sw_update_buf, 1, 7);
    if (result < 0)
    {
        return result;
    }
    result = TWI_MasterMode_Read(STM32_I2C_ADDR, pData, *nByte);
    return result;
}

/**
  * @brief  This function read operation when STM in bootloader mode
  * @param  void
  * @retval     1 : write packet success
  *             0 : write packet fail
  * @note
  */
static int stm_update_read_op_result(void)
{
    u32 addr = 1;
    u16 nByte = 1;
    u8 data = 0;
    int result = 0;
    
    result = stm_update_read_op(&addr, &nByte ,&data);
    if (result < 0)
    {
        return 0;
    }

    if (1 == data)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
  * @brief  This function write operation when STM in bootloader mode
  * @param  *pAddr : flash inter address to be writting
  * @param  *nByte : the number of byte to be writting
  * @param  *pData : the date to write.
  * @retval     -1 : read number of byte error
  *             >0 : normal operation
  *
  * @note   in this operation, stm32 will erase necessary flash page to wirte,
  *         so, we don't need to call stm_update_erase_page_op() function.
  */
static int stm_update_write_op(u32 *pAddr, u16 *nByte, u8 *pData)
{
    int result = 0;

    if (*nByte > STM_UPDATE_BUF_LEN-8)
    {
        return -1;
    }

    stm32_sw_update_buf[0] = OPC_WREN;
    
    stm32_sw_update_buf[1] = (*pAddr >> 24) & 0xFF; //ADD_HGH1
    stm32_sw_update_buf[2] = (*pAddr >> 16) & 0xFF; //ADD_HGH0
    stm32_sw_update_buf[3] = (*pAddr >> 8) & 0xFF;  //ADD_LOW1
    stm32_sw_update_buf[4] = (*pAddr >> 0) & 0xFF;  //ADD_LOW0
    
    stm32_sw_update_buf[5] = (*nByte >> 8) & 0xFF;  //NUMBER_H
    stm32_sw_update_buf[6] = (*nByte >> 0) & 0xFF;  //NUMBER_L
    
    stm32_sw_update_buf[7] = DUM_BYTE;              //DUM_BYTE

    memcpy(&stm32_sw_update_buf[8], pData, *nByte);

    result = TWI_MasterMode_Write(STM32_I2C_ADDR, stm32_sw_update_buf, 1, (8 + *nByte));
    return result;
}

/**
  * @brief  This function erase page operation when STM in bootloader mode
  * @param  *pAddr : flash inter address to be erase
  * @param  *nByte : the number of page to be erase
  * @retval     <0 : error
  *             >0 : normal operation
  */
static int stm_update_erase_page_op(const u32 *pAddr, const u32 *nPage)
{
    int result = 0;

    stm32_sw_update_buf[0] = OPC_ERPG;
    
    stm32_sw_update_buf[1] = (*pAddr >> 24) & 0xFF; //ADD_HGH1
    stm32_sw_update_buf[2] = (*pAddr >> 16) & 0xFF; //ADD_HGH0
    stm32_sw_update_buf[3] = (*pAddr >> 8) & 0xFF;  //ADD_LOW1
    stm32_sw_update_buf[4] = (*pAddr >> 0) & 0xFF;  //ADD_LOW0
    
    stm32_sw_update_buf[5] = (*nPage >> 8) & 0xFF;  //NUMBER_OF_PAGE_H
    stm32_sw_update_buf[6] = (*nPage >> 0) & 0xFF;  //NUMBER_OF_PAGE_L
    
    result = TWI_MasterMode_Write(STM32_I2C_ADDR, stm32_sw_update_buf, 1, 7);
    return result;
}

/**
  * @brief  This function erase user space memory when STM in bootloader mode
  * @param  note
  * @retval     <0 : error
  *             >0 : normal operation
  */
static int stm_update_erase_user_op(void)
{
    int result = 0;
    stm32_sw_update_buf[0] = OPC_ERUSM;
    
    result = TWI_MasterMode_Write(STM32_I2C_ADDR, stm32_sw_update_buf, 1, 1);
    return result;
}

/**
  * @brief  This function Go To User Code when STM in bootloader mode
  * @param  note
  * @retval     <0 : error
  *             >0 : normal operation
  */
static int stm_update_go_user_op(void)
{
    int result = 0;
    stm32_sw_update_buf[0] = OPC_USRCD;
    
    result = TWI_MasterMode_Write(STM32_I2C_ADDR, stm32_sw_update_buf, 1, 1);
    return result;
}

static int stm_update_download_ready(void)
{
    UploadDataByID(SF_AIO_STM_UPDATE,
                    TRUE,
                    (char *)SLAVER_UPDATE_TAG,
                    sizeof(SLAVER_UPDATE_TAG));
    return 0;
}


static int stm_update_packet_resend(u8 number)
{
    u8 pBuf[2];

    pBuf[0] = UPDATE_SOD;
    pBuf[1] = number;
    
    UploadDataByID(SF_AIO_STM_UPDATE, TRUE, (char *)pBuf, sizeof(pBuf));
    return 0;
}

static int stm_update_respond(UPDATE_COMMUNICATE_CID cid)
{
    char buf = (char)cid;
    UploadDataByID(SF_AIO_STM_UPDATE, TRUE, &buf, 1);
    return 0;
}

RETURN_TypeDef aio_stm32_UpdateHandle(UartProtocolPacket *pPacket)
{
    u32 addr, file_len;
    u16 nLen;
    int reslut = 0, timeout = 0x1000;
    u8 pBuf[2] = {0,0}, state = 0;
    static u8 data_number = 0;
    static u32 wFlashAddr = 0;

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //S1: update Tag Handle
    if (sizeof(MASTER_UPDATE_TAG) == pPacket->Length)
    {
        if (0 == memcmp(&pPacket->DataAndCRC[0],
                &MASTER_UPDATE_TAG[0],
                sizeof(MASTER_UPDATE_TAG)))
        {
            state = 1;
        }
    }
    if (1 == state) //get the start update message.
    {
        aio_stm32_enter_boot();
        addr = 0;
        nLen = 2;
        while(timeout--)  //wait for boot mode ready to download
        {
            pBuf[0] = 0;
            pBuf[1] = 0;
            reslut = stm_update_read_op(&addr, &nLen, pBuf);
            if ((reslut >= 0) && (0xAA == pBuf[0]) && (0x55 == pBuf[1]))
            {
                break;
            }
        }
        if (timeout > 0) //no timeout, means ready to download.
        {
            stm_update_download_ready();
            wFlashAddr = STM_USER_START_ADDRESS;
        }
        return RETURN_OK;
    }
    
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //S2:   bin lenght handle
    if ((UPDATE_SOL == pPacket->DataAndCRC[0]) \
        && (0x00 == pPacket->DataAndCRC[1]))
    {
        file_len = ((pPacket->DataAndCRC[2] << 24) \
                    | (pPacket->DataAndCRC[3] << 16) \
                    | (pPacket->DataAndCRC[4] << 8) \
                    | (pPacket->DataAndCRC[5] & 0xFF));
        
        if (file_len > (STM_FLASH_SIZE-STM_BOOT_SIZE))
        {
            stm_update_respond(UPDATE_CA);
        }
#if 0   //stm_update_write_op have earse function, so no need to earse again.
        Delay1ms(40);
        file_len = (file_len >> 10) + 1;//adjust one page (1K)
        reslut = stm_update_erase_page_op(&wFlashAddr, &file_len);
        if (reslut < 0)
        {
            return RETURN_ERROR; //i2c error, not ack.
        }
        
        Delay1ms(40);/*
        if (1 == stm_update_read_op_result())
        {*/
            data_number = 0x01;
            stm_update_respond(UPDATE_ACK);
        /*}
        else
        {
            return RETURN_ERROR; //stm32 operate error.
        }*/
#else
        else
        {
            data_number = 0x01;
            stm_update_respond(UPDATE_ACK);
        }
#endif
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //S3:   bin data handle
    if (UPDATE_SOD == pPacket->DataAndCRC[0])
    {
        if(data_number == pPacket->DataAndCRC[1]) //packet number normal
        {
            nLen = pPacket->Length - 2; //128,must is (2N)
            if (nLen & 0x0001) nLen ++;
            reslut = stm_update_write_op(&wFlashAddr, &nLen, &pPacket->DataAndCRC[2]);
            
            Delay1ms(40);
            if ((reslut > 0) && (1 == stm_update_read_op_result()))
            {
                data_number++;
                wFlashAddr += nLen;
                stm_update_respond(UPDATE_ACK);
            }
            else //error happen
            {
                stm_update_packet_resend(data_number);
            }
        }
        else //packet number error, means some packet loss. resend it
        {
            stm_update_packet_resend(data_number);
        }
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //S4:   End of transmit handle
    if (UPDATE_EOT == pPacket->DataAndCRC[0])
    {
        stm_update_respond(UPDATE_NAK);
    }
    
    if (UPDATE_CA == pPacket->DataAndCRC[0]) //master abort
    {
        stm_update_respond(UPDATE_ACK);
    }
    
    return RETURN_OK;
}

/*******************************************************************************
 <!-- STM32 I2C Api For Software Update (STM32_I2C_IAP) End -->
 ******************************************************************************/


/*******************************************************************************
 <!-- STM32 I2C Api For Debug or Test Begin -->
 ******************************************************************************/
int aio_stm32_readProtect(u8 *pindex, u16 *pmaxPress)
{
    int result = -1;
    u8 buf[3] = {0,};
    result = aio_stm32_readByID(PROTECT_PRESSURE, buf, 3);
    *pindex = buf[0];
    *pmaxPress = (buf[1] << 8)| buf[2];
    return result;
}

int aio_stm32_writeProtect(u8 *pindex, u16 *pmaxPress)
{
    int result = -1;
    u8 buf[3];
    buf[0] = *pindex;
    buf[1] = (u8)(*pmaxPress >> 8) & 0xFF;    //MSB
    buf[2] = (u8)(*pmaxPress & 0xFF);         //LSB
    result = aio_stm32_writeByID(PROTECT_PRESSURE, buf, 3);
    return result;
}

//@param: state --1:enter verify --0:exit verify
int aio_stm32_setVerify(u8 state)
{
    int result = -1;
    u8 buf = state;
    result = aio_stm32_writeByID(NIBP_VERIFY, &buf, 1);
    return result;
}

//@param pState:1 -- stm32 fast-rele on; 0 -- stm32 fast-rele off
//@return:
int aio_stm32_readNIBPState(u8 *pState)
{
    int result = -1;
    result = aio_stm32_readByID(NIBP_PROTECT_STATE, pState, 1);
    return result;
}

//@param pState:1 -- stm32 fast-rele on; 0 -- stm32 fast-rele off
int aio_stm32_writeNIBPState(u8 *pState)
{
    int result = -1;
    result = aio_stm32_writeByID(NIBP_PROTECT_STATE, pState, 1);
    return result;
}
/*******************************************************************************
 <!-- STM32 I2C Api For Debug or Test End -->
 ******************************************************************************/

void STM32_I2C_Handler(void)
{
    aio_stm32_ResetMaskAIOTick();
}

void STM32_I2C_Test(void)
{
    static unsigned long ulNextChangeTime = 0;
    
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        u16 value = 0;
        u8 buf[2] = {0,};
        aio_stm32_readByID(CURRENT_PRESSURE, buf, 2);
        value = (buf[0] << 8)| buf[1];
        STM32_DBG("ADC u16 = 0x%04X",value);

        ulNextChangeTime += getTickNumberByMS(1000);
    }
}


