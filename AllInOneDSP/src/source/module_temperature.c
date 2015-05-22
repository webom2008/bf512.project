/*
 * module_temperature.c
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */
#include "module_temperature.h"

#define TEST_Temp_PeriodMS    500

#define TEMP_TABLE_SIZE   countof(TEMP_TABLE_ohm)

unsigned long g_TempADC_Val[2]; //分别存Temp1和Temp2的值
bool IsTempNeedSample;          //判断是否更新体温的采样
bool IsSampleTempCOM;           //是否启动测试体温校准信号COM和COM_R
TEMP_Channel_TypeDef g_TempChannel;
UploadFormat_TypeDef g_TempUploadFormat;    //体温结果上传格式
bool g_IsTemp_Upload;

//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Start -->
#pragma section("sdram0_bank3")
void Temperature_Init(void);
//<!-- 声明一次性执行的函数，全部搬移在SDRAM(不是默认的DATA SRAM) Stop -->

static const u16 TEMP_TABLE_ohm[] = // (单位Ω)-(℃)
{
    7378,7341,7303,7266,7229,7192,7156,7119,7083,7047,
    7011,6976,6940,6905,6870,6835,6801,6767,6732,6698,
    6665,6631,6598,6564,6531,6499,6466,6433,6401,6369,
    6337,6305,6274,6242,6211,6180,6149,6119,6088,6058,
    6028,5998,5968,5938,5909,5879,5850,5821,5792,5763,
    5735,5707,5678,5650,5622,5595,5567,5540,5512,5485,
    5458,5431,5405,5378,5352,5326,5299,5273,5248,5222,
    5196,5171,5146,5121,5096,5071,5046,5022,4997,4973,
    4949,4925,4901,4877,4853,4830,4807,4783,4760,4737,
    4714,4692,4669,4646,4624,4602,4580,4558,4536,4514,
    4492,4471,4449,4428,4407,4386,4365,4344,4323,4303,
    4282,4262,4241,4221,4201,4181,4161,4141,4122,4102,
    4083,4063,4044,4025,4006,3987,3968,3950,3931,3912,
    3894,3876,3857,3839,3821,3803,3785,3768,3750,3732,
    3715,3698,3680,3663,3646,3629,3612,3595,3578,3562,
    3545,3529,3512,3496,3480,3464,3447,3431,3416,3400,
    3384,3368,3353,3337,3322,3307,3291,3276,3261,3246,
    3231,3216,3202,3187,3172,3158,3143,3129,3115,3100,
    3086,3072,3058,3044,3030,3016,3003,2989,2975,2962,
    2948,2935,2922,2908,2895,2882,2869,2856,2843,2830,
    2818,2805,2792,2780,2767,2755,2742,2730,2718,2705,
    2693,2681,2669,2657,2645,2633,2622,2610,2598,2587,
    2575,2564,2552,2541,2530,2518,2507,2496,2485,2474,
    2463,2452,2441,2430,2420,2409,2398,2388,2377,2367,
    2356,2346,2335,2325,2315,2305,2295,2284,2274,2264,
    2255,2245,2235,2225,2215,2206,2196,2186,2177,2167,
    2158,2148,2139,2130,2120,2111,2102,2093,2084,2075,
    2066,2057,2048,2039,2030,2021,2013,2004,1995,1987,
    1978,1970,1961,1953,1944,1936,1928,1919,1911,1903,
    1895,1887,1879,1871,1863,1855,1847,1839,1831,1823,
    1815,1808,1800,1792,1785,1777,1770,1762,1755,1747,
    1740,1732,1725,1718,1711,1703,1696,1689,1682,1675,
    1668,1661,1654,1647,1640,1633,1626,1619,1613,1606,
    1599,1592,1586,1579,1573,1566,1559,1553,1547,1540,
    1534,1527,1521,1515,1508,1502,1496,1490,1484,1477,
    1471,1465,1459,1453,1447,1441,1435,1429,1423,1418,
    1412,1406,1400,1394,1389,1383,1377,1372,1366,1361,
    1355,1350,1344,1339,1333,1328,1322,1317,1312,1306,
    1301,1296,1290,1285,1280,1275,1270,1264,1259,1254,
    1249,1244,1239,1234,1229,1224,1219,1214,1210,1205,
    1200,1195,1190,1185,1181,1176,1171,1167,1162,1157,
    1153,1148,1144,1139,1134,1130,1125,1121,1117,1112,
    1108,1103,1099,1095,1090,1086,1082,1077,1073,1069,
    1065,1061,1056,1052,1048,1044,1040,1036,1032,1028,
    1024,1020,1016,1012,1008,1004,1000, 996, 992, 988,
     984, 981, 977, 973, 969, 965, 962, 958, 954, 950,
     947, 943, 939, 936, 932, 929, 925, 921, 918, 914,
     911, 907, 904, 900, 897, 894, 890, 887, 883, 880,
     877, 873, 870, 867, 863, 860, 857, 853, 850, 847,
     844, 840, 837, 834, 831, 828, 825, 821, 818, 815,
     812
};


void Temperature_Init(void)
{
    IsTempNeedSample = false;
    IsSampleTempCOM = false;
    g_TempChannel = TEMP_COM;
    g_IsTemp_Upload = true;

#ifdef _UPLOAD_TEMP_ADC_
    g_TempUploadFormat = FMT_PACKET;//FMT_ADC_HEX
#else
    g_TempUploadFormat = FMT_UPLOAD_OFF;
#endif

}

inline static u16 temp_adc2ohm(u32 *adcValue)
{
    //R = ADC/8388607*2500/5.2553/0.07069469 - 270
    return (u16)((*adcValue) * 0.0008021689599616 - 270);
}

/**
  * @brief  This function adc value to centigrade(X10)
  *         eg.retval=305 means 30.5'C
  * @param  u32 adcValue,
  * @retval u16,centigrade X 10
  */
inline static u16 temp_adc2centigrade(u32 adcValue)
{
    u16 u16Value = temp_adc2ohm(&adcValue);
    u16 i = 0;
    
    for (i = 0; i < TEMP_TABLE_SIZE; i++)
    {
        if (u16Value > TEMP_TABLE_ohm[i]) break;
    }
    return i;
}

void Temperature_Handler(void)
{
    static unsigned long ulNextChangeTime = 0;
    static char channelStatus = 0;
    if(IsOnTime(ulNextChangeTime)) // is on time or over time
    {
        if (true == IsSampleTempCOM)
        {
            if (0 == channelStatus)
            {
                g_TempChannel = TEMP_COM;
            }
            else
            {
                g_TempChannel = TEMP_COM_R;
            }
        }
        else //处理通道切换，Temp1，Temp2轮流采集
        {
            if (0 == channelStatus)
            {
                g_TempChannel = TEMP_CH1;
            }
            else
            {
                g_TempChannel = TEMP_CH2;
            }
        }

        //处理采集，修改状态
        if (false == IsTempNeedSample)
        {
            AHCT595_SetTempChannel(g_TempChannel);
            IsTempNeedSample = true;
        }
        else//新采集需求已到，但是上次尚未采集完成，弹出告警信息
        {
            AIO_printf("\r\nTemperature_Handler lost sample.warning.");
        }
        channelStatus = ~channelStatus;
        ulNextChangeTime += getTickNumberByMS(TEST_Temp_PeriodMS);
    }
}

static void temp_setChannel(u8 *pChannel)
{
    if (0 == *pChannel)//采集TEMP1和TEMP2
    {
        IsSampleTempCOM = false;
    }
    else//采集COM和COM_R
    {
        IsSampleTempCOM = true;
    }
}

static void temp_setUploadFormat(u8 *pType)
{
    switch(*pType)
    {
    case 0x00:
        g_TempUploadFormat = FMT_PACKET;
        break;
        
    case 0x01:
        g_TempUploadFormat = FMT_CENTIGRADE;
        break;
        
    case 0x02:
        g_TempUploadFormat = FMT_VOLTAGE;
        break;
        
    case 0x03:
        g_TempUploadFormat = FMT_ADC_HEX;
        break;
        
    case 0x04:
        g_TempUploadFormat = FMT_ADC_DEC;
        break;
        
    case 0x05:
        g_TempUploadFormat = FMT_UPLOAD_OFF;
        break;
        
    default:
        break;
    }
}


RETURN_TypeDef TEMP_DebugInterface(UartProtocolPacket *pPacket)
{
    u8 type = pPacket->DataAndCRC[0];
    switch(type)
    {
    case 0x01://通道切换功能
        temp_setChannel(&(pPacket->DataAndCRC[1]));
        break;
    case 0x02://打印和上传格式
        temp_setUploadFormat(&(pPacket->DataAndCRC[1]));
        break;
    default:
        break;
    }
    return RETURN_OK;
}

void TempReslultUpload(void)
{
    u16 result[2];

    if (false == g_IsTemp_Upload) return;
    
    result[0] = temp_adc2centigrade(g_TempADC_Val[0]);//Temp1
    result[1] = temp_adc2centigrade(g_TempADC_Val[1]);//Temp2
    switch(g_TempUploadFormat)
    {
    case FMT_PACKET:
        {
            char pValue[5];
            pValue[0] = 0;//探头状态信息
            pValue[1] = (char)(result[0] >> 8) & 0xFF;      //Temp1 Vaule MSB
            pValue[2] = (char)(result[0] >> 0) & 0xFF;      //Temp1 Vaule LSB   
            pValue[3] = (char)(result[1] >> 8) & 0xFF;      //Temp2 Vaule MSB
            pValue[4] = (char)(result[1] >> 0) & 0xFF;      //Temp2 Vaule LSB
            
            UploadDataByID(AIO_TX_TEMP_REALTIME_ID, TRUE, pValue, sizeof(pValue));
            break;
        }
    case FMT_CENTIGRADE:
        AIO_printf("\r\n[TEMP] %f'C\t%f'C",result[0]/10.0,result[1]/10.0);
        break;
    case FMT_VOLTAGE:
        AIO_printf("\r\n[TEMP] %fV\t%fV", 2.5*g_TempADC_Val[0]/0x7FFFFF,2.5*g_TempADC_Val[1]/0x7FFFFF);
        break;
    case FMT_ADC_HEX:
        AIO_printf("\r\n[TEMP] 0x%08X\t0x%08X", g_TempADC_Val[0], g_TempADC_Val[1]);
        break;
    case FMT_ADC_DEC:
        result[0] = s24_to_s32(g_TempADC_Val[0]);
        result[1] = s24_to_s32(g_TempADC_Val[1]);
        AIO_printf("\r\n[TEMP] %d\t%d", result[0], result[1]);
        break;
    case FMT_UPLOAD_OFF:
        break;
    default :
        break;
    }
}

