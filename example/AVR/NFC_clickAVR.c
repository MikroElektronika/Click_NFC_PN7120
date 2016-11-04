 /*******************************************************************************
* Title                 :   NFC Example
* Filename              :   NFC_clickAVR.c
* Author                :   RBL
* Origin Date           :   20/02/2016
* Notes                 :   None
*******************************************************************************/
/*************** MODULE REVISION LOG ******************************************
*
*    Date    Software Version    Initials   Description
*  20/02/16           .1         RBL        Module Created.
*
*******************************************************************************/
/** @file NFC_clickAVR.c
 *  @brief
 *  @note
 *  Having issue on line 407 - 411
 */
/******************************************************************************
* Includes
*******************************************************************************/

#include "nfc_demo.h"

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
sbit NFC_RST_PIN at PORTA.B6;
sbit NFC_RST_PIN_DIR at DDRA.B6;
sbit NFC_INT_PIN at PIND.B2;
sbit NFC_INT_PIN_DIR at DDRD.B2;

void init_timer2()
{
     TCCR1A = 0x80;
     TCCR1B = 0x0B;
     OCR1AH = 0x3D;
     OCR1AL = 0x08;
     OCIE1A_bit = 1; ;
}

int system_init()
{
    NFC_RST_PIN_DIR = 1;
    NFC_INT_PIN_DIR = 0;
    
    // UART
    UART1_Init( 57600 );
    Delay_ms( 100 );
    UART_Write_Text( "Start Testing Loop\r\n" );
    TWI_Init( 100000 );
    Delay_ms( 100 );
    
    MCUCR |= ( 1 << ISC00 ) | ( 1 << ISC01 );
    GICR |= ( 1 << INT0 );
    init_timer2();
    example_init();
    SREG_I_bit = 1;
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

void card_rx_ISR() iv IVT_ADDR_INT0
{
    nfc_rx_ready();
}

void timer2_interrupt() iv IVT_ADDR_TIMER1_COMPA
{
    nfc_timer_tick();
}


/*************** END OF FUNCTIONS ***************************************************************************/