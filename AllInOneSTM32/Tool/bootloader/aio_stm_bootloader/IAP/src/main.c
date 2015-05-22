/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    IAP/src/main.c 
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   Main program body
  ******************************************************************************
  */
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define  ClockSpeed      400000
#define  OPC_READ       (uint8_t)(0x03)     //IAP I2C Read command
#define  OPC_WREN       (uint8_t)(0x06)     //IAP I2C Write command
#define  OPC_ERPG       (uint8_t)(0x20)     //IAP I2C Erase Page command
#define  OPC_ERUSM      (uint8_t)(0x60)     //IAP I2C Erase User Space Memory command
#define  OPC_USRCD      (uint8_t)(0x77)     //IAP I2C Go To User Code command
#define  DUM_BYTE       (0xFF)

/* Private macro -------------------------------------------------------------*/
#define UART_COM                USART1
#define UART_COM_CLK            RCC_APB2Periph_USART1
#define UART_COM_GPIO_CLK       RCC_APB2Periph_GPIOA
#define UART_COM_RxGPIO         GPIOA
#define UART_COM_TxGPIO         GPIOA
#define UART_COM_RxPin          GPIO_Pin_10
#define UART_COM_TxPin          GPIO_Pin_9
#define UART_COM_IRQn           USART1_IRQn
#define UART_COM_IRQHandler     USART1_IRQHandler
/* Private variables ---------------------------------------------------------*/

EventStatus i2c_event= NOEVENT;
uint8_t opcode;
I2C_InitTypeDef   I2C_InitStructure;
pFunction Jump_To_Application;
uint32_t JumpAddress;
char g_bTryLoopRun;

/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* Private functions ---------------------------------------------------------*/

//#define _DEBUG_

void iap_i2c_loop(void);
void init_uart(void);
void iap_i2c_try_loop(unsigned char timeout_s);
void iap_i2c_init(void);

#ifdef _DEBUG_
void print_all_flash(void)
{
    uint32_t i = 0x08000000+0x4000; //start of user app
    uint32_t flashEnd = 0x08010000;
    printf("\r\n");
    for (; i < flashEnd;i++)
    {
        printf("%02X ",*(__IO uint8_t*) i);
        if (i % 100 == 0) printf("\r\n");
    }
    printf("\r\nprint_all_flash finished!");
}
#endif
/**
  * @brief  Main program.
  * @param  None
  * @retval : None
  */
int main (void)
{
    /* Configures the different system clocks */
    RCC_Configuration();
    /* Configures the different GPIO ports */
    GPIO_Configuration();

    init_uart();
    printf("\r\nAIO-STM-IAP:I2C @CVTER.SeeCare Date:%s %s", __DATE__, __TIME__);
    iap_i2c_init();
    SysTick_Start();
    
#ifdef _DEBUG_
    print_all_flash();
    //while(1);
    iap_i2c_loop();
#endif
    /* Test if upload software by aio send command. */
    if ((uint16_t)0xAA == BKP_ReadBackupRegister(BKP_DR1))
    {
        BKP_WriteBackupRegister(BKP_DR1, 0x00);
        
        /* Execute the IAP i2c routine*/
        iap_i2c_loop();
    }
    else //TODO,Wait 3s for Update Tag
    {
        iap_i2c_try_loop(3);
    }

    /* Keep the user application running */
    /*The user code is called in iap_i2c_loop function when the value of opcode is equal to OPC_USRCD */
    /* Jump to user application */
    JumpAddress = *(__IO uint32_t*) (USER_START_ADDRESS +4);
    Jump_To_Application = (pFunction) JumpAddress;
    /* Initialize user application's Stack Pointer */
    __set_MSP(* ( __IO uint32_t* ) USER_START_ADDRESS);
    Jump_To_Application();

	return (0);



}

void iap_i2c_init(void)
{
     /* Unlock the Flash */
    FLASH_Unlock();
    /* Configures NVIC and Vector Table base location */
    NVIC_Configuration();
    I2C_DeInit(I2C_CH);
    /* I2C_CH configuration ------------------------------------------------------*/
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_SLAVE_ADDRESS7;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = ClockSpeed;
    I2C_Init(I2C_CH, &I2C_InitStructure);
}

/**
  * @brief  In application programming routine.
  * @param  None
  * @retval : None
  */
void iap_i2c_loop(void)
{
    while (1)
    {

        /* Enable I2Cx event and buffer interrupts */
        I2C_ITConfig(I2C_CH, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
        /* Enable I2Cx Error interrupts */
        I2C_ITConfig(I2C_CH, I2C_IT_ERR, ENABLE);

        /* Only when there is an event and the the opcode is not yet read, the value
          of opcode must be tested */
        if (i2c_event == EVENT_OPCOD_NOTYET_READ)
        {
            /* Read opcode */
            opcode = Read_Opcode();
            i2c_event = EVENT_OPCOD_READ; /* The opcode has been already read */
        }

        switch (opcode)
        {
        case OPC_READ:
        
            Read_Memory_Command();
            break;

        case OPC_WREN:
        
            Write_Memory_Command();
            break;

        case OPC_ERPG:
        
            Erase_Page_Command();
            break;

        case OPC_ERUSM:
        
            User_Space_Memory_Erase_Command();
            break;

        case OPC_USRCD:
        
            /* Jump to user application */
            JumpAddress = *(__IO uint32_t*) (USER_START_ADDRESS +4);
            Jump_To_Application = (pFunction) JumpAddress;
            /* Initialize user application's Stack Pointer */
            __set_MSP(*(__IO uint32_t*) USER_START_ADDRESS);
            Jump_To_Application();

            break;
        default:
        break;

        }
    }
}


/**
  */
void iap_i2c_try_loop(unsigned char timeout_s)
{
    u32 curTick = SysTick_Get();
    curTick += timeout_s * SysTickFreq;
    g_bTryLoopRun = 0;
    
    while((0 == IsOnTime(curTick)) || (1 == g_bTryLoopRun))
    {

        /* Enable I2Cx event and buffer interrupts */
        I2C_ITConfig(I2C_CH, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
        /* Enable I2Cx Error interrupts */
        I2C_ITConfig(I2C_CH, I2C_IT_ERR, ENABLE);

        /* Only when there is an event and the the opcode is not yet read, the value
          of opcode must be tested */
        if (i2c_event == EVENT_OPCOD_NOTYET_READ)
        {
            /* Read opcode */
            opcode = Read_Opcode();
            i2c_event = EVENT_OPCOD_READ; /* The opcode has been already read */
        }

        switch (opcode)
        {
        case OPC_READ:
        
            Read_Memory_Command();
            break;

        case OPC_WREN:
        
            Write_Memory_Command();
            break;

        case OPC_ERPG:
        
            Erase_Page_Command();
            break;

        case OPC_ERUSM:
        
            User_Space_Memory_Erase_Command();
            break;

        case OPC_USRCD:
        
            /* Jump to user application */
            JumpAddress = *(__IO uint32_t*) (USER_START_ADDRESS +4);
            Jump_To_Application = (pFunction) JumpAddress;
            /* Initialize user application's Stack Pointer */
            __set_MSP(*(__IO uint32_t*) USER_START_ADDRESS);
            Jump_To_Application();

            break;
        default:
        break;
        }
    }
}

/* USARTx configured as follow:
    - BaudRate = 115200 baud  
    - Word Length = 8 Bits
    - One Stop Bit
    - No parity
    - Hardware flow control disabled (RTS and CTS signals)
    - Receive and transmit enabled
*/
static void init_uart(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(UART_COM_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(UART_COM_CLK, ENABLE);
    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // Configure UART_COM Rx as input floating
    GPIO_InitStructure.GPIO_Pin = UART_COM_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(UART_COM_RxGPIO, &GPIO_InitStructure);
    // Configure UART_COM Tx as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = UART_COM_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(UART_COM_TxGPIO, &GPIO_InitStructure);
    
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(UART_COM, &USART_InitStructure);
    USART_Cmd(UART_COM, ENABLE);
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    USART_SendData(UART_COM, (uint8_t) ch);

    /* Loop until the end of transmission */
    while (USART_GetFlagStatus(UART_COM, USART_FLAG_TC) == RESET)
    {}

    return ch;
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
}
#endif

/************************END OF FILE****/

