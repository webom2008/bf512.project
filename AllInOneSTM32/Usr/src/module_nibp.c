#include "module_nibp.h"

#define NIBP_HANDLE_TIME_MS             100 //100ms

#define NIBP_PRESS_RELE_PIN             GPIO_Pin_5
#define NIBP_PRESS_RELE_PORT            GPIOA
#define NIBP_PRESS_RELE_GPIO_CLK        RCC_APB2Periph_GPIOA

#define NIBP_MAX_PRESS                  310//mmHg
#define NIBP_MIN_PRESS                  12//mmHg < 15mmHg

//#define NIBP_DEBUG_INFO
#ifdef NIBP_DEBUG_INFO
#define NIBP_DBG_INFO(fmt, arg...) printf("\r\n[NIBP] "fmt, ##arg)
#else
#define NIBP_DBG_INFO(fmt, arg...)
#endif

NIBP_CtrlStatus g_nibp_protect;
NIBP_ProtectDef g_nibp_press;
NIBP_ProtectDef* p_nibp_press = &g_nibp_press;
static char g_nibp_verify_sw;
char g_nibp_fast_rele_state;//1 -- stm32 fast-rele on; 0 -- stm32 fast-rele off

static void NIBP_PressRelease_IO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(NIBP_PRESS_RELE_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = NIBP_PRESS_RELE_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(NIBP_PRESS_RELE_PORT, &GPIO_InitStructure);
}

static void NIBP_SetPressRelease(FunctionalState state)
{
    if (ENABLE == state)
    {
        g_nibp_fast_rele_state = 1;
        GPIO_ResetBits(NIBP_PRESS_RELE_PORT, NIBP_PRESS_RELE_PIN);
    }
    else
    {
        GPIO_SetBits(NIBP_PRESS_RELE_PORT, NIBP_PRESS_RELE_PIN);
    }
}

int NIBP_SetSafePress(u8 *pBuf, u8 nLen)
{
    if (3 != nLen) return -1;
    g_nibp_press.Index = pBuf[0];
    g_nibp_press.u16PressADC_Max = (pBuf[1] << 8) | pBuf[2];
	return 0;
}

int NIBP_SetNIBPVerify(u8 state)
{
    if (!state)
    {
        g_nibp_verify_sw = 0;
    }
    else
    {
        g_nibp_verify_sw = 1;
    }
	return 0;
}

int NIBP_SetNIBPFastReleState(u8 state)
{
    if (!state)
    {
        g_nibp_fast_rele_state = 0;
        NIBP_SetPressRelease(DISABLE);
    }
	return 0;
}

static void NIBP_SafePressInit(void)
{
    g_nibp_press.u16PressADC_Max = 7.31 * NIBP_MAX_PRESS + 346.73;
    g_nibp_press.u16PressADC_Min = 7.31 * NIBP_MIN_PRESS + 346.73;
    g_nibp_press.Index = 0;
}

void NIBP_Init(void)
{
    g_nibp_protect = Ctrl_Idle;
    g_nibp_verify_sw = 0;
    g_nibp_fast_rele_state = 0;
    
    NIBP_PressRelease_IO_Init();
    NIBP_SetPressRelease(DISABLE);
    NIBP_SafePressInit();
}

void NIBP_Handler(void)
{
    static u32 u32NextChangeTime = 0;
    
    if(IsOnTime(u32NextChangeTime)) // is on time or over time
    {
        u32NextChangeTime += getNextTimeByMS(NIBP_HANDLE_TIME_MS);
        if (1 == g_nibp_verify_sw) return;
        
        if ((g_SysADCResult.DET_NIBP > g_nibp_press.u16PressADC_Max) \
            && (Ctrl_Idle == g_nibp_protect))
        {
            g_nibp_protect = Ctrl_Busy;
            NIBP_SetPressRelease(ENABLE);
        }
        else if ((Ctrl_Busy == g_nibp_protect) \
            && (g_SysADCResult.DET_NIBP < g_nibp_press.u16PressADC_Min))
        {
            g_nibp_protect = Ctrl_Idle;
            NIBP_SetPressRelease(DISABLE);
        }
        NIBP_DBG_INFO("MAX=%d MIN=%d",
                        g_nibp_press.u16PressADC_Max,
                        g_nibp_press.u16PressADC_Min);
    }
}

