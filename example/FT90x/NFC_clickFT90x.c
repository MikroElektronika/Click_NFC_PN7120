/**
 *  Example for :
 *          - Clicker 2 for Kintetis
 *          - EasyMX development board
 */

#include "nfc_demo.h"

/*
 *  PIN DEFINITIONS
 ******************************************************************************/
sbit NFC_RST_PIN at GPIO_PIN1_bit;
sbit NFC_INT_PIN at GPIO_PIN3_bit;

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
    TIMER_CONTROL_0 = 2;
    TIMER_SELECT = 0;
    TIMER_PRESC_LS = 36;
    TIMER_PRESC_MS = 244;
    TIMER_WRITE_LS = 15;
    TIMER_WRITE_MS = 0;
    TIMER_CONTROL_3 = 0;
    TIMER_CONTROL_4 |= 17;
    TIMER_CONTROL_2 |= 16;
    TIMER_INT |= 2;
    TIMER_CONTROL_1 |= 1;
}

void init_extie()
{
    IRQ_CTRL.B31 = 0;
    GPIO03_CFG0_bit = 1;
    GPIO03_CFG1_bit = 1;
    GPIO03_CFG2_bit = 0;
    GPIO03_CFG3_bit = 0;
    GPIO_INT_EN3_bit = 1;
}

int system_init()
{
    GPIO_Digital_Input( &GPIO_PORT_00_07, _GPIO_PINMASK_3 );
    GPIO_Digital_Output( &GPIO_PORT_00_07, _GPIO_PINMASK_1 );

    UART1_Init( 115200 );
    Delay_ms( 200 );

    I2CM1_Init( _I2CM_SPEED_MODE_FAST, _I2CM_SWAP_DISABLE );
    Delay_ms( 100 );

    //GLOBAL_INTERRUPT_MASK_bit = 0;
    init_extie();
    init_timer();
    example_init();
    //GLOBAL_INTERRUPT_MASK_bit = 1;

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
void card_rx_ISR() iv IVT_GPIO_IRQ ics ICS_AUTO
{
    GPIO_INT_PEND3_bit = 1;
    nfc_rx_ready();
}

void timer_interrupt() iv IVT_TIMERS_IRQ
{
     nfc_timer_tick();

     if( TIMER_INT_A_bit )
     {
         TIMER_INT = (TIMER_INT & 0xAA) | (1 << 0);
     }
}