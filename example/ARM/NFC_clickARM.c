/**
 *  Example for : 
 *          - Clicker 2 for Kintetis
 *          - EasyMX development board
 */

#include "nfc_demo.h"

/*
 *  PIN DEFINITIONS
 ******************************************************************************/
#ifdef STM32
sbit NFC_RST_PIN at GPIOC_ODR.B2;
sbit NFC_INT_PIN at GPIOD_IDR.B10;
#endif
#ifdef KINETIS
sbit NFC_RST_PIN at PTB_PDOR.B19;
sbit NFC_INT_PIN at PTB_PDIR.B8;
#endif

/*
 * Function Prototypes
 ******************************************************************************/

void init_timer( void );
void init_extie( void );
int system_init( void );

/*
 * Function Definitions
 ******************************************************************************/

void init_timer()
{
#ifdef STM32
    RCC_APB1ENR.TIM2EN = 1;
    TIM2_CR1.CEN = 0;
    TIM2_PSC = 11;
    TIM2_ARR = 59999;
    NVIC_IntEnable( IVT_INT_TIM2 );
    TIM2_DIER.UIE = 1;
    TIM2_CR1.CEN = 1;
#endif
#ifdef KINETIS
    SIM_SCGC6 |= (1 << PIT);
    NVIC_IntEnable(IVT_INT_PIT0);
    PIT_MCR = 0x00;
    PIT_LDVAL0 = 59999;
    PIT_TCTRL0 |= 2;
    PIT_TCTRL0 |= 1;
#endif
}

void init_extie()
{
#ifdef STM32
    RCC_APB2ENR.AFIOEN = 1;
    AFIO_EXTICR3 = 0x0300;
    EXTI_RTSR |= ( 1 << TR10 );
    EXTI_IMR |= ( 1 << MR10 );
    NVIC_IntEnable( IVT_INT_EXTI15_10 );
#endif
#ifdef KINETIS
    PORTB_PCR8 |= 0x00090000;
    NVIC_IntEnable( IVT_INT_PORTB );
#endif
}

int system_init()
{
#ifdef STM32
    GPIO_Digital_Output( &GPIOC_BASE, _GPIO_PINMASK_2 );
    GPIO_Digital_Input( &GPIOD_BASE, _GPIO_PINMASK_10 );
    Delay_ms( 200 );

    UART1_Init_Advanced( 115200,
                         _UART_8_BIT_DATA,
                         _UART_NOPARITY,
                         _UART_ONE_STOPBIT,
                         &_GPIO_MODULE_USART1_PA9_10 );
    Delay_ms( 200 );

    I2C1_Init_Advanced( 400000, &_GPIO_MODULE_I2C1_PB67 );
    Delay_ms( 200 );
#endif
#ifdef KINETIS
    GPIO_Digital_Output( &PTB_PDOR, _GPIO_PINMASK_19 );
    GPIO_Digital_Input( &PTB_PDIR, _GPIO_PINMASK_8 );
    Delay_ms( 200 );

    UART2_Init( 115200 );
    Delay_ms( 200 );

    I2C1_Init_Advanced( 400000, &_GPIO_Module_I2C1_PC10_11 );
    Delay_ms( 200 );
#endif

    init_extie();
    init_timer();
    example_init();
    EnableInterrupts();

    return 0;
}

void main()
{
    nfc_interface_t r_interface;

    if( system_init() )
        while( 1 );

    UART_Write_Text( "\r\nWAITING FOR DEVICE DISCOVERY\r\n" );

    while( 1 )
    {
        nfc_wait_for_discovery_notification( &r_interface );
        UART_Write_Text( "Device Discovered\r\n" );
        process_radio( &r_interface );
    }
}

/*
 * Interrupt routines
 ******************************************************************************/
#ifdef STM32
void card_rx_ISR() iv IVT_INT_EXTI15_10 ics ICS_AUTO
{
    EXTI_PR |= ( 1 << PR10 );
    nfc_rx_ready();
}

void timer2_interrupt() iv IVT_INT_TIM2
{
    TIM2_SR.UIF = 0;
    nfc_timer_tick();
}
#endif
#ifdef KINETIS
void card_rx_ISR() iv IVT_INT_PORTB ics ICS_AUTO
{
        PORTB_PCR8 |= 0x01000000;
        nfc_rx_ready();
}

void Timer0_interrupt() iv IVT_INT_PIT0
{
    PIT_TFLG0.TIF = 1;
    nfc_timer_tick();
}
#endif
