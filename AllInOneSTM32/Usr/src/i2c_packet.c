/****************************************Copyright (c)****************************************************
**                            Guangzhou CVTE Co.,LTD.
**
**                                 http://www.cvte.cn
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           packet.c
** Last modified Date:  2013-10-14
** Last Version:        V1.00
** Descriptions:        the specific codes for NIBP target boards
**                      User may modify it as needed
**--------------------------------------------------------------------------------------------------------
** Created by:          zhengxiaobo
** Created date:        2013-11-18
** Version:             V1.00
** Descriptions:        The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:         zhengxiaobo
** Modified date:       2013-11-18
** Version:             V1.00
** Descriptions:        Add some comment
**
*********************************************************************************************************/
#include "i2c_packet.h"

#ifdef __PACKET_C__

#define PACKET_DEBUG_INFO

#ifdef PACKET_DEBUG_INFO
#define PACKET_DBG_INFO(fmt, arg...) printf("\r\n[PACKET] "fmt, ##arg)
#else
#define PACKET_DBG_INFO(fmt, arg...)
#endif



/*********************************************************************************************************
** Function name:           packet_init
** Descriptions:            packet_init
** input parameters:        none
** output parameters:       none
** Returned value:          0:success
*********************************************************************************************************/
int packet_init(void)
{
    return 0;
}

/*********************************************************************************************************
** Function name:           packet_server
** Descriptions:            packet_server
** input parameters:        none
** output parameters:       none
** Returned value:          none
*********************************************************************************************************/
void packet_server(void)
{
    static u32 NextChangeTime = 0;
    
    if(IsOnTime(NextChangeTime))
    {
        
        NextChangeTime += 10;
    }    
}


#endif /* __PACKET_C__ */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/



