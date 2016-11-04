#include "nfc_demo.h"

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/

sbit NFC_RST_PIN at LATC3_bit;
sbit NFC_INT_PIN at RE9_bit;

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
    T1CON         = 0x8010;
    T1IE_bit         = 1;
    T1IF_bit         = 0;
    T1IP0_bit         = 1;
    T1IP1_bit         = 1;
    T1IP2_bit         = 1;
    PR1                 = 10000;
    TMR1         = 0;
}

void init_extie()                                                               // Enable external interrupt on PIN logical 1 - rising edge
{
    AD1PCFG     = 0xFFFF;              // Turning all analog pins to digital
    INT2IP0_bit = 0;                   // Set INT1 interrupt
    INT2IP1_bit = 0;                   // Set interrupt priorities
    INT2IP2_bit = 1;                   // Set inrrupt priority to 4
    INT2EP_bit  = 1;
    INT2IE_bit  = 1;                   // Set interrupt on INT2 (RE9) to be enabled
}

int system_init()
{
    TRISC3_bit = 0;
    TRISE9_bit = 1;                    // Set RE9 pin as input
    
    UART5_Init( 115200 );
    Delay_ms( 200 );
    
    I2C2_Init( 100000 );
    Delay_ms( 200 );

    init_extie();
    init_timer();
    EnableInterrupts();
    
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

void int2_interrupt() iv IVT_EXTERNAL_2 ilevel 4 ics ICS_AUTO
{
    INT2IF_bit = 0;
    nfc_rx_ready();
}

void Timer1Interrupt() iv IVT_TIMER_1 ilevel 7 ics ICS_SRS
{
    T1IF_bit         = 0;
    nfc_timer_tick();
}