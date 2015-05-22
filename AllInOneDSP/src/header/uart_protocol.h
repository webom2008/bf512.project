/*
 * uart_protocol.h
 *
 *  Created on: 2013-10-10
 *      Author: QiuWeibo
 */

#ifndef UART_PROTOCOL_H_
#define UART_PROTOCOL_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

typedef enum
{
    UPDATE_ACK       = 0x01,
    UPDATE_NAK,
    UPDATE_EOT,
    UPDATE_SOL, //start of *.bin lenght
    UPDATE_SOD, //start of data:length = packet data lenght - 2(CID + Number)
    UPDATE_CA,  //one of these in succession aborts transfer
    UPDATE_RP,  //resend the packet
} UPDATE_COMMUNICATE_CID;

extern unsigned char crc8(unsigned char *ptr, unsigned char len);
extern u8 CalculatePacketCRC8(UartProtocolPacket *p_Packet);
extern void Uart_RxPacketCheck(void);
extern void UploadDataByID(const UART_PacketID id,
                            const TRUE_OR_FALSE IsPassData,
                            char* pData,
                            const u8 lenght);

#ifdef __cplusplus
}
#endif

#endif /* UART_PROTOCOL_H_ */

