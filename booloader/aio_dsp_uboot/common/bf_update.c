/*
 * AIO-BF512 CVTE UPDATE PROTOCOL BY UART
 *
 * Copyright (c) 2014 CVTE SeeCare Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <command.h>
#include <stdarg.h>

#ifdef CONFIG_CHECK_AIO_BF512_UPDATE
#include <bfin_protocol.h>
#include <spi_flash.h>
#include <asm/io.h>
#include <watchdog.h>

typedef struct 
{ 
    u8 DR_Addr;             //目的地址。上位机地址：0x55；下位机地址：0xCC。 
    u8 SR_Addr;             //源地址 
    u8 PacketNum;           //包编号 
    u8 PacketID;            //包标识
    u8 Length;              //数据长度 
    u8 DataAndCRC[251];     //数据内容和CRC校验值
} UartProtocolPacket;
typedef enum
{
    Packet_DesAddress       = 0,
    Packet_SouAddress,
    Packet_Number,
    Packet_ID,
    Packet_DataLen,
    Packet_DataStart,
} UartProtocolPacketIndex;
#define PACKET_FIXED_LENGHT     6   // Packet中固有元素的总长度

typedef enum
{
    UPDATE_ACK       = 0x01,
    UPDATE_NAK,
    UPDATE_EOT,
    UPDATE_SOL, //start of *.bin lenght
    UPDATE_SOD, //start of data:length = packet data lenght - 2(CID + Number)
    UPDATE_CA,  //one of these in succession aborts transfer
    UPDATE_RP,  //resend the packet
    UPDATE_W_F_DONE,  //Write data into flash done
    UPDATE_W_F_ERR,  //Write data into flash error
} UPDATE_COMMUNICATE_CID;

typedef enum
{
    PKT_OK,
    PKT_TRY_HEAD,
    PKT_TIMEOUT,
    PKT_CRC_ERR,
    PKT_UPDATE_TAG,
    PKT_ABORT,
    PKT_NOT_UPDATE_PKT
} RxPacketResult_TypeDef;

#define SOFTWARE_UPDATE_ID          (0xD3)
#define ERR_LICENSE_FAILED          (0xF0)
#define NAK_TIMEOUT             (0x100000)

const u8 SOFTWARE_UPDATE_ANSWER[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA};
const u8 SOFTWARE_UPDATE_GET[] = {0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00};

uint8_t packet_cID_HEAD[2]; //[0]= UPDATE_COMMUNICATE_CID, [1] = packet number

/*
 * @return  :1 -- OK    0 -- Error
 */
static int Receive_Byte (u8 *c, ulong timeout)
{
#if 1
#define DELAY 20
    unsigned long counter = 0;
    while (!tstc () && (counter < timeout * 1000 / DELAY))
    {
        udelay (DELAY);
        counter++;
    }
    if (tstc ())
    {
        *c = getc ();
        return 1;
    }
    return 0;
#else
    unsigned long counter = 0;
    int data = 0;
    do
    {
        data = getc ();
        counter++;
    }while ((data < 0) && (counter < timeout));

    if (counter < timeout)
    {
        *c = (u8)data;
        //printf(" %.2X", *c);
        return 1;
    }
    return 0;
#endif
}

#if defined __ADSPBF51x__
#define UART0_MMR_OFFSET    0x0000
#define UART1_MMR_OFFSET    0x1C00
#define THRE			    0x20	/* THR Empty */
#endif

static short UartPutc(unsigned char UartNum, char c)
{
    unsigned short UartMmrOffset = 0;

    switch (UartNum) {
        case 0: UartMmrOffset = UART0_MMR_OFFSET; break;
        case 1: UartMmrOffset = UART1_MMR_OFFSET; break;
        default: break;
    }

    volatile unsigned short *pUartLsr = (volatile unsigned short*) (UART0_LSR + UartMmrOffset);
    volatile unsigned short *pUartThr = (volatile unsigned short*) (UART0_THR + UartMmrOffset);

    while (!(*pUartLsr & THRE)) { /* wait */ }
    *pUartThr = c;

    return 0;
}

static void UART_SendBuf(u8 *data, ulong length)
{
    ulong i;
    for (i = 0; i < length; i++)
    {
        //putc (*data);
        UartPutc(1, *data);
        data++;
        //udelay (20);
    }
}

static u8 CalculateCRC(u8* pu8Buf, u8 u8Len)
{
    u8 crc;
    u8 i;
    crc = 0;
    while(u8Len--)
    {
        crc ^= *pu8Buf++;
        for(i = 0; i < 8; i++)
        {
            if(crc&0x01)
            {
                crc = (crc >> 1) ^ 0x8C;
            }   
            else
            {
                crc >>= 1;
            }
        }   
    }
    return crc;
}

static void InitUartPacketStruct(u8* pu8Buf)
{
    static u8 u8PacketNum = 0;

    pu8Buf[Packet_DesAddress] = 0xAA;
    pu8Buf[Packet_SouAddress] = 0x55;
    pu8Buf[Packet_Number] = u8PacketNum++;
}

static void protocol_RespondReady(void)
{
    u8 u8Buf[PACKET_FIXED_LENGHT+32];
    u8 u8Index = 0;

    InitUartPacketStruct(u8Buf);
    u8Buf[Packet_ID] = SOFTWARE_UPDATE_ID;
    memcpy(&u8Buf[Packet_DataStart+u8Index],
        SOFTWARE_UPDATE_ANSWER,
        sizeof(SOFTWARE_UPDATE_ANSWER));
    u8Index += sizeof(SOFTWARE_UPDATE_ANSWER);
    u8Buf[Packet_DataLen] = u8Index;
    u8Buf[Packet_DataStart + u8Index] = \
        CalculateCRC(&u8Buf[Packet_Number], u8Index+Packet_DataStart-Packet_Number);
    UART_SendBuf(u8Buf, u8Buf[Packet_DataLen] + PACKET_FIXED_LENGHT);
}

static void protocol_Respond(UPDATE_COMMUNICATE_CID CID)
{
    u8 u8Buf[PACKET_FIXED_LENGHT+1];
    u8 u8Index = 0;

    InitUartPacketStruct(u8Buf);
    u8Buf[Packet_ID] = SOFTWARE_UPDATE_ID;
    u8Buf[Packet_DataStart + (u8Index++)] = (u8)CID;
    u8Buf[Packet_DataLen] = u8Index;
    u8Buf[Packet_DataStart + u8Index] = \
        CalculateCRC(&u8Buf[Packet_Number], u8Index+Packet_DataStart-Packet_Number);
    UART_SendBuf(u8Buf, u8Buf[Packet_DataLen] + PACKET_FIXED_LENGHT);
}

static void protocol_ReSendByCID(unsigned char count)
{
    u8 u8Buf[PACKET_FIXED_LENGHT+2];
    u8 u8Index = 0;

    InitUartPacketStruct(u8Buf);
    u8Buf[Packet_ID] = SOFTWARE_UPDATE_ID;
    u8Buf[Packet_DataStart + (u8Index++)] = packet_cID_HEAD[0];
    u8Buf[Packet_DataStart + (u8Index++)] = count;
    u8Buf[Packet_DataLen] = u8Index;
    u8Buf[Packet_DataStart + u8Index] = \
        CalculateCRC(&u8Buf[Packet_Number], u8Index+Packet_DataStart-Packet_Number);
    UART_SendBuf(u8Buf, u8Buf[Packet_DataLen] + PACKET_FIXED_LENGHT);
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    >0: data buffer length
  * @retval :RxPacketResult_TypeDef
  */
static RxPacketResult_TypeDef aio_uart_Receive_Packet (unsigned char *data, int *length, ulong timeout)
{
    UartProtocolPacket packet;
    u8 crc = 0;
    u8 i = 0;
    char isOK = 0;
    
    *length = 0;

    memset(&packet, 0, sizeof(UartProtocolPacket));

    WATCHDOG_RESET();
    if (Receive_Byte(&packet.DR_Addr, timeout))
    {
        if (0x55 == packet.DR_Addr)
        {
            if (Receive_Byte(&packet.SR_Addr, timeout))
            {
                if (0xAA == packet.SR_Addr)
                {
                    if (0 == Receive_Byte(&packet.PacketNum, timeout))  return PKT_TIMEOUT;
                    if (0 == Receive_Byte(&packet.PacketID, timeout))   return PKT_TIMEOUT;
                    if (0 == Receive_Byte(&packet.Length, timeout))     return PKT_TIMEOUT;
                    for (i=0; i <= packet.Length; i++)  //get data and crc value
                    {
                        if (0 == Receive_Byte(&packet.DataAndCRC[i], timeout)) return PKT_TIMEOUT;
                    }
        
                    //check CRC vlaue
                    crc = CalculateCRC(&packet.PacketNum,packet.Length+3);
                    if (packet.DataAndCRC[packet.Length] == crc) isOK = 1;
                }
                else return PKT_TRY_HEAD;
            }
            else    return PKT_TIMEOUT;
        }
        else return PKT_TRY_HEAD;
    }
    else    return PKT_TIMEOUT;

    if (0 == isOK) return PKT_CRC_ERR; //Error Packet
    /*
    printf("\r\n[U-boot]Get a MCU->DSP packet %.2X(DR) %.2X(SR) %.2X(ID) %.2X(Len)!\n",
        packet.DR_Addr, packet.SR_Addr, packet.PacketID, packet.Length);
    for (i = 0; i < packet.Length; i++)
    {
        printf(" %X", packet.DataAndCRC[i]);
    }
    printf("\r\n");
    */
    if (packet.PacketID != SOFTWARE_UPDATE_ID) return PKT_NOT_UPDATE_PKT; //Not the update Packet

    if (packet.Length == sizeof(SOFTWARE_UPDATE_GET))
    {
        if (0 == memcmp(&packet.DataAndCRC[0],SOFTWARE_UPDATE_GET,sizeof(SOFTWARE_UPDATE_GET)))
        {
            return PKT_UPDATE_TAG;
        }
    }
    
    packet_cID_HEAD[0] = packet.DataAndCRC[0];

    switch (packet.DataAndCRC[0])
    {
    case (u8)UPDATE_SOL:
    case (u8)UPDATE_SOD:
        break;
        
    case (u8)UPDATE_EOT:
        *length = 0;
        return PKT_OK;
        
    case (u8)UPDATE_CA:
        *length = -1;
        return PKT_ABORT;
        
    default:
        return PKT_TIMEOUT;
    }
    packet_cID_HEAD[1] = packet.DataAndCRC[1];
    memcpy(data, &packet.DataAndCRC[2], packet.Length - 2);
    *length = packet.Length - 2;
    return PKT_OK;
}

/**
  * @brief  Receive a file using the aio uart protocol
  * @param  buf: Address of the Buffer first byte
  * @retval  :
  *         0: normal return
  *        -1: Abort by sender
  *        -2: bin file lenght out of STM32-flash
  *        -3: write STM32-flash error
  */
static int load_uart_bf_protocol(const ulong ram_addr, unsigned long *p_file_len)
{
    int i;
    int data_length;
    unsigned char data_packets_count = 0;
    unsigned long size = 0;
    unsigned char buf_ptr[1024];
    *p_file_len = 0;
    ulong  ram_offset = ram_addr;
    
    while(1)
    {
        WATCHDOG_RESET();
        switch (aio_uart_Receive_Packet(buf_ptr, &data_length, NAK_TIMEOUT))
        {
        case PKT_OK: // normally return
        {
            if ((u8)UPDATE_EOT == packet_cID_HEAD[0])
            {
                //End of transmission
                protocol_Respond(UPDATE_NAK);
                return 0;
            }
            
            //get update.bin length
            if ((u8)UPDATE_SOL == packet_cID_HEAD[0])
            { 
                size = ((buf_ptr[0] << 24) | (buf_ptr[1] << 16) | (buf_ptr[2] << 8) | buf_ptr[3]);
                data_packets_count = 1;
                ram_offset = ram_addr;
                /* Test the size of the image to be sent */
                /* Image size is greater than Flash size */
                if (size > (CONFIG_USER_SPI_FLASH_SIZE - 1))
                {
                    /* End session */
                    protocol_Respond(UPDATE_CA);
                    while (1)
                    {
                        udelay(100000);
                        WATCHDOG_RESET();
                        printf("\r\nOut of SFlash Lenght:bin_size= 0x%02X%02X%02X%02X(%d Bytes) > MAX(%d)",
                            (u8)buf_ptr[0],(u8)buf_ptr[1],(u8)buf_ptr[2],(u8)buf_ptr[3],size, (CONFIG_USER_SPI_FLASH_SIZE - 1));

                    }
                    return (-2);
                }
                
                *p_file_len = size;
            }

            //get *.bin data and write to RAM
            if ((u8)UPDATE_SOD == packet_cID_HEAD[0])
            {
                //Data packet
                if (packet_cID_HEAD[1] != data_packets_count) //packet number error, end of download
                {
                    protocol_ReSendByCID(data_packets_count);
                    break;
                }
                else
                {
                    memcpy((unsigned char *)ram_offset, buf_ptr, data_length);
                    ram_offset += data_length;
                    data_packets_count ++;
                }
            }
            
            protocol_Respond(UPDATE_ACK);
            break;
        }
        case PKT_UPDATE_TAG: //Get an Update Tag
            data_packets_count = 0;
            size = 0;
            *p_file_len = 0;
            ram_offset = ram_addr;
            bf_protocol_RespondReady();
            break;
        case PKT_CRC_ERR://packet error, need to resend
            if (data_packets_count > 0)
            {
                protocol_ReSendByCID(data_packets_count);
            }
            break;
        case PKT_ABORT://abort by sender
        {
            protocol_Respond(UPDATE_ACK);
            return (-1);
        }
        case PKT_TIMEOUT://timeout
        case PKT_TRY_HEAD:
        case PKT_NOT_UPDATE_PKT://Not the update Packet
        default:
            break;
        } // end of switch (Receive_Packet(packet_data, &data_length, NAK_TIMEOUT))
    } // end of while(1)
}

static int bf_protocol_RespondWriteFlash(UPDATE_COMMUNICATE_CID id)
{
    int i;
    WATCHDOG_RESET();
    for (i=0; i< 3; i++) protocol_Respond(id);
    return 0;
}

static int do_load_ldr_uart(void)
{
    ulong sdram_addr = 0, sFlash_offset = 0;
    unsigned long file_size = 0;
    int rcode = 0;
    char *s;

    /* pre-set sdram_addr from CONFIG_SYS_LOAD_ADDR */
    sdram_addr = CONFIG_SYS_LOAD_ADDR;

    /* pre-set sdram_addr from $loadaddr */
    if ((s = getenv("loadaddr")) != NULL)
    {
        sdram_addr = simple_strtoul(s, NULL, 16);
    }

    //sf probe 2;
	unsigned int bus = 0;
	unsigned int cs = 2;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = SPI_MODE_3;
    struct spi_flash *pSFlash;
    pSFlash = spi_flash_probe(bus, cs, speed, mode);
    if (!pSFlash)
    {
        printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
        return -1;
    }
    
    rcode = load_uart_bf_protocol(sdram_addr, &file_size);

    if ((0 != rcode) || (file_size > CONFIG_USER_SPI_FLASH_SIZE))
    {
        printf("\r\nFailed to get bin file recode = %d file_size = %d!",rcode,file_size);
        return -1;
    }
    else
    {
        printf("\r\nGet bin file = 0x%X OK!",file_size);
    }
    
    file_size = ((file_size >> 12) + 1 ) << 12; //adjust to 4K
    
    printf("\r\nsf erase 0x80000 0x%X",file_size);
    sFlash_offset = 0x80000;
	rcode = spi_flash_erase(pSFlash, sFlash_offset, file_size);
    if (rcode < 0)
    {
        printf("\r\nsf erase 0x80000 0x%X Error",file_size);
    }
    
    printf("\r\nsf write 0x%X 0x%X 0x%X",sdram_addr, sFlash_offset, file_size);
    
    WATCHDOG_RESET();
    rcode = spi_flash_write(pSFlash, sFlash_offset, file_size, (void *)sdram_addr);
	//unmap_physmem(buf, file_size);
    if (rcode < 0)
    {
        bf_protocol_RespondWriteFlash(UPDATE_W_F_ERR);
        printf("\r\nsf write 0x%X 0x%X 0x%X Error",sdram_addr, sFlash_offset, file_size);
    }

    bf_protocol_RespondWriteFlash(UPDATE_W_F_DONE);

    spi_flash_free(pSFlash);

    printf("\r\nUpdate success.");
    return rcode;
}

int bf_protocol_RespondReady(void)
{
    WATCHDOG_RESET();
    protocol_RespondReady();
    return 0;
}

int bf_protocol_LicenseFailed(void)
{
    u8 u8Buf[PACKET_FIXED_LENGHT+1];
    u8 u8Index = 0;

    InitUartPacketStruct(u8Buf);
    u8Buf[Packet_ID] = ERR_LICENSE_FAILED;
    u8Buf[Packet_DataStart + (u8Index++)] = 0x0F;
    u8Buf[Packet_DataLen] = u8Index;
    u8Buf[Packet_DataStart + u8Index] = \
        CalculateCRC(&u8Buf[Packet_Number], u8Index+Packet_DataStart-Packet_Number);
    UART_SendBuf(u8Buf, u8Buf[Packet_DataLen] + PACKET_FIXED_LENGHT);
}

/**
  * @brief  test for update TAG packet
  * @param  void
  * @retval  :
  *         0: not UPDATE TAG packet
  *         1: get Update TAG Packet
  */
int bf_protocol_GetUpdatePacket(void)
{
    char buf[256];
    int len;
    ulong timeout = 1000;
    
    RxPacketResult_TypeDef result = aio_uart_Receive_Packet(buf, &len, timeout);

    if (PKT_UPDATE_TAG == result)
    {
        //printf("\n\r[U-boot]Get a MCU->DSP Update packet!");
        return 1;
    }
    else
    {
        return 0;
    }
}

int bf_protocol_SaveLDRFile(void)
{
    int reslut = 0;
    
    reslut = do_load_ldr_uart();
    return reslut;
}
#endif  /* CONFIG_CHECK_AIO_BF512_UPDATE	*/

