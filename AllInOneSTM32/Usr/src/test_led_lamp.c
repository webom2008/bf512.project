#include "test_led_lamp.h"

#define TEST_LED_TIME_MS   500 //500ms

#define LED_GPIO_CLK        RCC_APB2Periph_GPIOA
#define LED_GPIO_PORT       GPIOA
#define LED_GPIO_PIN        GPIO_Pin_0

static void LED_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(LED_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);  
}

void LED_Init(void)
{
    LED_GPIO_Init();
}

void LED_Handler(void)
{
    static u32 u32NextChangeTime = 0;
    static u8 u8LEDStatus = 0;
    
    if(IsOnTime(u32NextChangeTime)) // is on time or over time
    {
        u8LEDStatus = !u8LEDStatus;
        if(u8LEDStatus)
        {
            GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);    // turn off LED
        }
        else
        {
            GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);  // turn on LED
        }
        
        u32NextChangeTime += getNextTimeByMS(TEST_LED_TIME_MS);
    }
}


