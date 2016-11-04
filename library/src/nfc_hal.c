/*******************************************************************************
* Title                 :   CLICKNAME click
* Filename              :   CLICKNAME_hal.c
* Author                :   MSV
* Origin Date           :   28/01/2016
* Notes                 :   None
*******************************************************************************/
/*************** MODULE REVISION LOG ******************************************
*
*    Date    Software Version    Initials   Description
*  28/01/16    XXXXXXXXXXX         MSV      Interface Created.
*
*******************************************************************************/
/**
 * @file nfc_hal.c
 * @brief This module contains the
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include "nfc_hal.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

/******************************************************************************
* Module Preprocessor Constants
*******************************************************************************/

#define I2C_READ                    1
#define I2C_WRITE                   0

#if defined( __MIKROC_PRO_FOR_PIC32__ ) || \
    defined( __MIKROC_PRO_FOR_DSPIC__ )
static uint8_t NACK_BIT   = 0x01;
static uint8_t ACK_BIT    = 0x00;
#elif defined( __MIKROC_PRO_FOR_PIC__ ) || \
      defined( __MIKROC_PRO_FOR_AVR__ )
static uint8_t NACK_BIT   = 0x00;
static uint8_t ACK_BIT    = 0x01;
#endif

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/

/******************************************************************************
* Module Typedefs
*******************************************************************************/

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static uint8_t _i2c_hw_address;

#if defined( __MIKROC_PRO_FOR_ARM__ )
#if defined( STM32 ) || defined ( KINETIS )
static unsigned int( *start_i2c_p )          ( void );
static unsigned int( *write_i2c_p )          ( unsigned char slave_address,
                                               unsigned char *buffer,
                                               unsigned long count,
                                               unsigned long end_mode );
static void( *read_i2c_p )                   ( unsigned char slave_address,
                                               unsigned char *buffer,
                                               unsigned long count,
                                               unsigned long end_mode );
#elif defined( TI )
static void( *enable_i2c_p )                  ( void );
static void( *disable_i2c_p )                 ( void );
static void( *set_slave_address_i2c_p )       ( unsigned char slave_address,
                                                unsigned char dir );
static void( *write_i2c_p )                   ( unsigned char data_out,
                                                unsigned char mode );
static void( *read_i2c_p )                    ( unsigned char *data,
                                                unsigned char mode );
#endif
#elif  defined( __MIKROC_PRO_FOR_AVR__ )
static unsigned char( *busy_i2c_p )           ( void );
static unsigned char( *status_i2c_p )         ( void );
static unsigned char( *start_i2c_p )          ( void );
static void( *stop_i2c_p )                    ( void );
static void( *close_i2c_p )                   ( void );
static void( *write_i2c_p )                   ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned char ack );

#elif  defined( __MIKROC_PRO_FOR_PIC__ )
static unsigned char( *is_idle_i2c_p )        ( void );
static unsigned char( *start_i2c_p )          ( void );
static void( *stop_i2c_p )                    ( void );
static void( *restart_i2c_p )                 ( void );
static unsigned char( *write_i2c_p )          ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned char ack );

#elif defined( __MIKROC_PRO_FOR_PIC32__ )
static unsigned int( *is_idle_i2c_p )         ( void );
static unsigned int( *start_i2c_p )           ( void );
static void( *stop_i2c_p )                    ( void );
static unsigned int( *restart_i2c_p )         ( void );
static unsigned int( *write_i2c_p )           ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned int ack );

#elif defined( __MIKROC_PRO_FOR_DSPIC__ )
static unsigned int( *is_idle_i2c_p )         ( void );
static unsigned int( *start_i2c_p )           ( void );
static void( *stop_i2c_p )                    ( void );
static void( *restart_i2c_p )                 ( void );
static unsigned int( *write_i2c_p )           ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned int ack );

#elif defined( __MIKROC_PRO_FOR_8051__ )
static unsigned char ( *status_i2c_p )        ( void );
static unsigned char( *start_i2c_p )          ( void );
static void( *stop_i2c_p )                    ( void );
static void( *close_i2c_p )                   ( void );
static void( *write_i2c_p )                   ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned char ack );

#elif defined( __MIKROC_PRO_FOR_FT90x__ )
static void( *soft_reset_i2c_p )              ( void );
static void( *set_slave_address_i2c_p )       ( unsigned char slave_address );
static unsigned char( *write_i2c_p )          ( unsigned char data_out );
static unsigned char( *read_i2c_p )           ( unsigned char *data_in );
static unsigned char( *write_bytes_i2c_p )    ( unsigned char *buffer,
                                                unsigned int count );
static unsigned char( *read_bytes_i2c_p )     ( unsigned char *buffer,
                                                unsigned int count );
static unsigned char( *write_10bit_i2c_p )    ( unsigned char data_out,
                                                unsigned int address_10bit );
static unsigned char( *read_10bit_i2c_p )     ( unsigned char *data_in,
                                                unsigned int address_10bit );
#elif defined ( __GNUC__ )

#endif

#if defined( __MIKROC_PRO_FOR_ARM__ )   || \
    defined( __MIKROC_PRO_FOR_AVR__ )   || \
    defined( __MIKROC_PRO_FOR_PIC__ )   || \
    defined( __MIKROC_PRO_FOR_PIC32__ ) || \
    defined( __MIKROC_PRO_FOR_DSPIC__ ) || \
    defined( __MIKROC_PRO_FOR_8051__ )  || \
    defined( __MIKROC_PRO_FOR_FT90x__ )
extern sfr sbit NFC_RST_PIN;
extern sfr sbit NFC_INT_PIN;
#endif

/******************************************************************************
* Function Definitions
*******************************************************************************/

/***********************************************
 *      Implementations
 **********************************************/
void nfc_hal_init( uint8_t address_id )
{
#if defined( __MIKROC_PRO_FOR_ARM__ )
#if defined( STM32 ) || defined( KINETIS )
    start_i2c_p             = I2C_Start_Ptr;
    write_i2c_p             = I2C_Write_Ptr;
    read_i2c_p              = I2C_Read_Ptr;
#elif defined( TI )
    enable_i2c_p            = I2C_Enable_Ptr;
    disable_i2c_p           = I2C_Disable_Ptr;
    set_slave_address_i2c_p = I2C_Master_Slave_Addr_Set_Ptr;
    write_i2c_p             = I2C_Write_Ptr;
    read_i2c_p              = I2C_Read_Ptr;
#endif

#elif defined( __MIKROC_PRO_FOR_AVR__ )
#if defined( LT64 )
    busy_i2c_p              = TWI_Busy;
    status_i2c_p            = TWI_Status;
    close_i2c_p             = TWI_Close;
    start_i2c_p             = TWI_Start;
    stop_i2c_p              = TWI_Stop;
    write_i2c_p             = TWI_Write;
    read_i2c_p              = TWI_Read;
#elif defined( GT64 )
    busy_i2c_p              = TWIC_Busy;
    status_i2c_p            = TWIC_Status;
    close_i2c_p             = TWIC_Close;
    start_i2c_p             = TWIC_Start;
    stop_i2c_p              = TWIC_Stop;
    write_i2c_p             = TWIC_Write;
    read_i2c_p              = TWIC_Read;
#endif

#elif defined( __MIKROC_PRO_FOR_PIC__ )
    is_idle_i2c_p           = I2C1_Is_Idle;
    start_i2c_p             = I2C1_Start;
    stop_i2c_p              = I2C1_Stop;
    restart_i2c_p           = I2C1_Repeated_Start;
    write_i2c_p             = I2C1_Wr;
    read_i2c_p              = I2C1_Rd;

#elif defined( __MIKROC_PRO_FOR_PIC32__ )
    is_idle_i2c_p           = I2C_Is_Idle_Ptr;
    start_i2c_p             = I2C_Start_Ptr;
    stop_i2c_p              = I2C_Stop_Ptr;
    restart_i2c_p           = I2C_Restart_Ptr;
    write_i2c_p             = I2C_Write_Ptr;
    read_i2c_p              = I2C_Read_Ptr;

#elif defined( __MIKROC_PRO_FOR_DSPIC__ )
    is_idle_i2c_p           = I2C2_Is_Idle;
    start_i2c_p             = I2C2_Start;
    stop_i2c_p              = I2C2_Stop;
    restart_i2c_p           = I2C2_Restart;
    write_i2c_p             = I2C2_Write;
    read_i2c_p              = I2C2_Read;

#elif defined( __MIKROC_PRO_FOR_8051__ )
    status_i2c_p            = TWI_Status;
    close_i2c_p             = TWI_Close;
    start_i2c_p             = TWI_Start;
    stop_i2c_p              = TWI_Stop;
    write_i2c_p             = TWI_Write;
    read_i2c_p              = TWI_Read;

#elif defined( __MIKROC_PRO_FOR_FT90x__ )
    soft_reset_i2c_p        = I2CM_Soft_Reset_Ptr;
    set_slave_address_i2c_p = I2CM_Set_Slave_Address_Ptr;
    write_i2c_p             = I2CM_Write_Ptr;
    read_i2c_p              = I2CM_Read_Ptr;
    write_bytes_i2c_p       = I2CM_Write_Bytes_Ptr;
    read_bytes_i2c_p        = I2CM_Read_Bytes_Ptr;
    write_10bit_i2c_p       = I2CM_Write_10Bit_Ptr;
    read_10bit_i2c_p        = I2CM_Read_10Bit_Ptr;
#endif

#if defined( __MIKROC_PRO_FOR_ARM__ )   ||  \
    defined( __MIKROC_PRO_FOR_FT90x__ )
    _i2c_hw_address         = address_id;
#else
    _i2c_hw_address         = ( address_id << 1 );
#endif
    nfc_hal_reset();
}


void nfc_hal_reset()
{
#if defined( __MIKROC_PRO_FOR_ARM__ )   || \
    defined( __MIKROC_PRO_FOR_AVR__ )   || \
    defined( __MIKROC_PRO_FOR_PIC__ )   || \
    defined( __MIKROC_PRO_FOR_PIC32__ ) || \
    defined( __MIKROC_PRO_FOR_DSPIC__ ) || \
    defined( __MIKROC_PRO_FOR_8051__ )  || \
    defined( __MIKROC_PRO_FOR_FT90x__ )
    NFC_RST_PIN = 1;
    Delay_10ms();
    NFC_RST_PIN = 0;
    Delay_10ms();
    NFC_RST_PIN = 1;
    Delay_10ms();
#endif

}

void nfc_hal_delay( uint16_t ms )
{
#if defined( __MIKROC_PRO_FOR_ARM__ )   || \
    defined( __MIKROC_PRO_FOR_AVR__ )   || \
    defined( __MIKROC_PRO_FOR_PIC__ )   || \
    defined( __MIKROC_PRO_FOR_PIC32__ ) || \
    defined( __MIKROC_PRO_FOR_DSPIC__ ) || \
    defined( __MIKROC_PRO_FOR_8051__ )  || \
    defined( __MIKROC_PRO_FOR_FT90x__ )
    while( ms--)
        Delay_1ms();
#endif

}

int nfc_hal_write( uint8_t *data_out, uint16_t count )
{

#ifdef HAL_LOG
    char hex[ 10 ];
    char *ptr = data_out;
    uint16_t len = count;
    int i;
#endif
    uint8_t *buffer = data_out;

    if( buffer == NULL || count < 2 )
        return -1;

#if defined(__MIKROC_PRO_FOR_ARM__)
    #if defined( STM32 ) || defined( KINETIS )
    start_i2c_p();
    write_i2c_p( _i2c_hw_address, buffer, count, END_MODE_STOP );
    #elif defined( TI )
    set_slave_address_i2c_p( _i2c_hw_address, _I2C_DIR_MASTER_TRANSMIT );
    if( count == 2 ) {
        write_i2c_p( *buffer++, _I2C_MASTER_MODE_BURST_SEND_START );
        write_i2c_p( *buffer, _I2C_MASTER_MODE_BURST_SEND_STOP );
    }
    else {
        write_i2c_p( *buffer++, _I2C_MASTER_MODE_BURST_SEND_START );
        while( i-- > 1 )
            write_i2c_p( *buffer++, _I2C_MASTER_MODE_BURST_SEND_CONT );
        write_i2c_p( *buffer, _I2C_MASTER_MODE_BURST_SEND_FINISH );
    }
    #endif
#elif defined(__MIKROC_PRO_FOR_FT90x__)
    set_slave_address_i2c_p( _i2c_hw_address );
    write_bytes_i2c_p( buffer, count );
#elif defined(__MIKROC_PRO_FOR_AVR__)   || \
      defined(__MIKROC_PRO_FOR_8051__)  || \
      defined(__MIKROC_PRO_FOR_DSPIC__) || \
      defined(__MIKROC_PRO_FOR_PIC32__) || \
      defined(__MIKROC_PRO_FOR_PIC__)
    start_i2c_p();
    write_i2c_p( _i2c_hw_address | I2C_WRITE );
    while( count-- )
        write_i2c_p( *buffer++ );
    stop_i2c_p();
#elif defined( __GNUC__ )

#endif
#ifdef HAL_LOG
    UART_Write_Text( "TX ( " );
    IntToStr( len, hex );
    UART_Write_Text( Ltrim( hex ) );
    UART_Write_Text( " ) : " );
    for( i = 0; i < len; i++ ){
        ByteToHex( *ptr++, hex );
        UART_Write_Text( hex );
        UART_Write( 32 );
    }
    UART_Write_Text( "\r\n" );
#endif
    nfc_hal_delay( 10 );
    return 0;
}

int nfc_hal_read( uint8_t *data_in, uint16_t *nbytes_read, uint16_t count )
{
#ifdef HAL_LOG
    char hex[ 10 ];
    char *ptr = data_in;
    int i;
#endif
    uint16_t cnt = 0;
    uint8_t *buffer = data_in;

#if defined(__MIKROC_PRO_FOR_ARM__)
    #if defined( STM32 ) || defined( KINETIS )
    start_i2c_p();
    read_i2c_p( _i2c_hw_address, buffer, 3, END_MODE_STOP );
    if( *( buffer + 2 ) != 0 ) {
        start_i2c_p();
        read_i2c_p( _i2c_hw_address, buffer + 3, *( buffer + 2 ), END_MODE_STOP );
        *nbytes_read = *( buffer + 2 ) + 3;
    }
    else {
        *nbytes_read = 3;
    }
    #elif defined( TI )
    set_slave_address_i2c_p( _i2c_hw_address, _I2C_DIR_MASTER_RECEIVE );
    read_i2c_p( *buffer++, _I2C_MASTER_MODE_BURST_RECEIVE_START );
    read_i2c_p( *buffer++, _I2C_MASTER_MODE_BURST_RECEIVE_CONT );
    read_i2c_p( *buffer, _I2C_MASTER_MODE_BURST_RECEIVE_FINISH );

    if( ( cnt = *( buffer + 2 ) ) != 0 ) {
        set_slave_address_i2c_p( _i2c_hw_address, _I2C_DIR_MASTER_RECEIVE );

        if( cnt == 1 ) {
            read_i2c_p( buffer + 3, _I2C_MASTER_MODE_BURST_SINGLE_RECEIVE );
        }
        else {
            read_i2c_p( buffer++ , _I2C_MASTER_MODE_BURST_RECEIVE_START );
            count--;
            while( count-- > 1 )
                read_i2c_p( buffer++ , _I2C_MASTER_MODE_BURST_RECEIVE_CONT );
            read_i2c_p( buffer, _I2C_MASTER_MODE_BURST_RECEIVE_FINISH );
        }
        *nbytes_read = *( data_in + 2 ) + 3;
    }
    else {
        *nbytes_read = 3;
    }
    #endif
#elif defined(__MIKROC_PRO_FOR_FT90x__)
    set_slave_address_i2c_p( _i2c_hw_address );
    read_bytes_i2c_p( buffer, 3 );
    
    if( *( buffer + 2 ) != 0 ) {
        set_slave_address_i2c_p( _i2c_hw_address );
        read_bytes_i2c_p( buffer + 3, *( buffer + 2 ) );
        *nbytes_read = *( buffer + 2 ) + 3;
    } else {
        *nbytes_read = 3;
    }
#elif defined( __MIKROC_PRO_FOR_AVR__ )    || \
      defined( __MIKROC_PRO_FOR_PIC32__ )  || \
      defined( __MIKROC_PRO_FOR_8051__ )   || \
      defined( __MIKROC_PRO_FOR_PIC__ )    || \
      defined( __MIKROC_PRO_FOR_DSPIC__ )
    start_i2c_p();
    write_i2c_p( _i2c_hw_address | I2C_READ );
    *buffer++ = read_i2c_p( ACK_BIT );
    *buffer++ = read_i2c_p( ACK_BIT );
    *buffer++ = read_i2c_p( NACK_BIT );
    stop_i2c_p();
        
    if( ( cnt = *( data_in + 2 ) ) != 0 ) {
        start_i2c_p();
        write_i2c_p( _i2c_hw_address | I2C_READ );
        while( --cnt )
             *buffer++ = read_i2c_p( ACK_BIT );
        *buffer = read_i2c_p( NACK_BIT );
        stop_i2c_p();
        *nbytes_read = *( data_in + 2 ) + 3;
    } else {
        *nbytes_read = 3;
    }
#elif defined( __GNUC__ )

#endif
#ifdef HAL_LOG
    UART_Write_Text( "RX ( " );
    IntToStr( *nbytes_read, hex );
    UART_Write_Text( Ltrim( hex ) );
    UART_Write_Text( " ) : " );

    for( i = 0; i < *nbytes_read; i++ )
    {
        ByteToHex( *ptr++, hex );
        UART_Write_Text( hex );
        UART_Write( 32 );
    }

    UART_Write_Text( "\r\n" );
#endif
    return 0;
}

/*************** END OF FUNCTIONS *********************************************/