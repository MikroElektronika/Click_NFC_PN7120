#include "nfc_demo.h"

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/

sbit NFC_RST_PIN at LATC0_bit;
sbit NFC_INT_PIN at RB0_bit;

/******************************************************************************
* Function Prototypes
*******************************************************************************/

void init_timer( void );
void init_extie( void );
int system_init( void );

/******************************************************************************
* Function Definitions
*******************************************************************************/

void init_timer()                                                               // Timer 1 ms
{
    T0CON	 = 0xC3;
    TMR0L	 = 0x06;
    GIE_bit	 = 1;
    TMR0IE_bit	 = 1;
}

void init_extie()                                                               // Enable external interrupt on PIN logical 1 - rising edge
{

     INTCON.GIE=1; //enable global interrupt
     INTCON.PEIE=1; //enable periphiral interrupts
     INTCON.INT0IE=1; //enable external interrupts
     INTCON.RBIE=1; //enable interrupt change
     INTCON2.INTEDG0=1; //external interrupt on rising edge
}

int system_init()
{
    TRISC0_bit = 0;
    TRISB0_bit = 1;

    UART1_Init( 115200 );
    Delay_ms( 200 );

    I2C1_Init( 100000 );
    Delay_ms( 200 );

    init_extie();
    init_timer();

    example_init();
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

void interrupt()
{
    if( TMR0IF_bit )
    {
        TMR0IF_bit = 0;
        TMR0L	 = 0x06;
        nfc_timer_tick();
    }
}