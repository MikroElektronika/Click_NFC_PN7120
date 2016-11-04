/*******************************************************************************
* Title                 :   System Initialization
* Filename              :   sys_init.c
* Author                :   RBL
* Origin Date           :   04/23/2012
* Notes                 :   None
*******************************************************************************/
/*************** MODULE REVISION LOG ******************************************
*
*    Date    Software Version    Initials   Description
*  20/01/16        .1            RBL      Module Created.
*
*******************************************************************************/
/** @file nfc.c
 *  @brief This module contains the
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "nfc.h"

#ifdef CARDEMU_SUPPORT
#include "T4T_NDEF_EMU.h"
#endif
#ifdef P2P_SUPPORT
#include "P2P_NDEF.h"
#endif
#ifdef RW_SUPPORT
#include "RW_NDEF.h"
#include "RW_NDEF_T3T.h"
#endif

/******************************************************************************
* Module Preprocessor Constants
*******************************************************************************/

#define ANSWER_MAX_SIZE 258

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/

static bool volatile rx_flag;
static uint32_t volatile timer_tick;

/******************************************************************************
* Function Prototypes
*******************************************************************************/

static int initialize_core
(
        void
);

static int host_rx
(
        uint8_t* p_rx_buff,
        uint16_t rx_buff_size,
        uint16_t *p_nbytes_read,
        uint16_t timeout
);

static int host_tx_rx
(
        uint8_t *p_tx_buff,
        uint16_t tx_buff_len,
        uint8_t *p_rx_buff,
        uint16_t rx_buff_size,
        uint16_t *p_nbytes_read
);

#ifdef CARDEMU_SUPPORT
static void card_mode
(
                nfc_interface_t *rf_intf
);
#endif
#ifdef P2P_SUPPORT
static void p2p_mode
(
                nfc_interface_t *rf_intf
);
#endif
#ifdef RW_SUPPORT

static void reader_mode
(
                nfc_interface_t *rf_intf
);

#endif

/******************************************************************************
* Private Function Definitions
*******************************************************************************/
extern sfr sbit NFC_INT_PIN;

static int host_rx
(
        uint8_t *p_rx_buff,
        uint16_t rx_buff_size,
        uint16_t *p_nbytes_read,
        uint16_t timeout
)
{
    uint8_t *p_rx_buff_ptr = p_rx_buff;
    uint16_t *p_nbytes_read_ptr = p_nbytes_read;

    timer_tick = 0;

#ifndef __GNUC__

    if( NFC_INT_PIN == 0 ) {
      if( timeout == TIMEOUT_INFINITE )
         while( !rx_flag );
      else
        while( !rx_flag ){
            if( timer_tick > ( timeout / 10 ) ){
                *p_nbytes_read = 0;
                return;
            }
        }
    }
#endif

    if( nfc_hal_read( p_rx_buff_ptr, p_nbytes_read_ptr, rx_buff_size ) ){
        UART_Write_Text( "RX error \n" );
        *p_nbytes_read = 0;
        return NFC_ERROR;
    }

    rx_flag = false;

    return NFC_SUCCESS;
}

static int host_tx_rx
(
        uint8_t *p_tx_buff,
        uint16_t tx_buff_len,
        uint8_t *p_rx_buff,
        uint16_t rx_buff_size,
        uint16_t *p_nbytes_read
)
{
    uint8_t *p_tx_buff_ptr = p_tx_buff;
    uint8_t *p_rx_buff_ptr = p_rx_buff;
    uint16_t *p_nbytes_read_ptr = p_nbytes_read;

    if( nfc_hal_write( p_tx_buff_ptr, tx_buff_len ) )
        return NFC_ERROR;
    else if( host_rx( p_rx_buff_ptr, rx_buff_size, p_nbytes_read_ptr, TIMEOUT_1S ) )
        return NFC_ERROR;

    return NFC_SUCCESS;
}

static int initialize_core( void )
{
    uint8_t answer[ ANSWER_MAX_SIZE ]           = { 0 };
    uint16_t answer_size                        = 0;
    int i                                       = 10;
    uint8_t CORE_RESET_CMD[]                    = { 0x20, 0x00, 0x01, 0x01 };
    uint8_t CORE_INIT_CMD[]                     = { 0x20, 0x01, 0x00 };

#if defined( DEBUG )

    UART_Write_Text( "\r\n*****Resetting and Initializing Core*****\r\n" );
#endif
                                                                                /* Reset the core */
    while( host_tx_rx( CORE_RESET_CMD, sizeof( CORE_RESET_CMD ), answer, sizeof( answer ), &answer_size ) ){
        if( !i-- )
            return NFC_ERROR;

        nfc_hal_delay( 500 );
    }
                                                                                /* Once reset, initialize the core */
    if( host_tx_rx( CORE_INIT_CMD, sizeof( CORE_INIT_CMD ), answer, sizeof( answer ), &answer_size ) )
        return NFC_ERROR;
    else if( ( answer[ 0 ] != 0x40 ) || \
             ( answer[ 1 ] != 0x01 ) || \
             ( answer[ 3 ] != 0x00 ) )
        return NFC_ERROR;
}

#ifdef CARDEMU_SUPPORT

static void card_mode( nfc_interface_t *rf_intf )
{
    uint8_t answer[ ANSWER_MAX_SIZE ]           = { 0 };
    uint16_t answer_size                        = 0;

    t4t_ndef_emu_reset();                                                       /* Reset Card emulation state */

    while( !host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_2S ) ) {
        if( ( answer[ 0 ] == 0x61 ) && ( answer[ 1 ] == 0x06 ) ) {              /* is RF_DEACTIVATE_NTF ? */
            break;                                                              /* Come back to discovery state */
        }
        else if( ( answer[ 0 ] == 0x00 ) && ( answer[ 1 ] == 0x00 ) ) {         /* is DATA_PACKET ? */
            uint8_t cmd[ 256 ];
            uint16_t cmd_size = 0;

            t4t_ndef_emu_next( &answer[ 3 ], answer[ 2 ], &cmd[ 3 ], ( uint16_t * )&cmd_size );

            cmd[ 0 ] = 0x00;
            cmd[ 1 ] = ( cmd_size & 0xFF00 ) >> 8;
            cmd[ 2 ] = cmd_size & 0x00FF;

            host_tx_rx( cmd, cmd_size + 3, answer, sizeof( answer ), &answer_size );
        }
    }
}
#endif

#ifdef P2P_SUPPORT

static void p2p_mode( nfc_interface_t *rf_intf )
{
    uint8_t answer[ ANSWER_MAX_SIZE ] = { 0 };
    uint16_t answer_size              = 0;
    uint8_t NCIL_LCP_SYMM[]           = { 0x00, 0x00, 0x02, 0x00, 0x00 };
    uint8_t NCI_RESTART_DISCOVERY[]   = { 0x21, 0x06, 0x01, 0x03 };

    p2p_ndef_reset();                                                           /* Reset P2P_NDEF state */

    if( ( rf_intf->mode_tech & MODE_LISTEN ) != MODE_LISTEN )                   /* Is INITIATOR_MODE ? */ /* Initiate communication ( SYMM PDU ) */
        host_tx_rx( NCIL_LCP_SYMM, sizeof( NCIL_LCP_SYMM ), answer, sizeof( answer ), &answer_size );

    while( !host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_2S ) ){    /* Get frame from remote peer */
        if( ( answer[0] == 0x00 ) && ( answer[1] == 0x00 ) ) {                  /* is DATA_PACKET ? */
            uint8_t cmd[256]  = { 0 };
            uint16_t cmd_size = 0;

            p2p_ndef_next( &answer[ 3 ], answer[ 2 ], &cmd[ 3 ], ( uint16_t * )&cmd_size );

            cmd[ 0 ] = 0x00;                                                    /* Compute DATA_PACKET to answer */
            cmd[ 1 ] = ( cmd_size & 0xFF00 ) >> 8;
            cmd[ 2 ] = cmd_size & 0x00FF;

            host_tx_rx( cmd, cmd_size + 3, answer, sizeof( answer ), &answer_size );
        }
        else if ( ( answer[ 0 ] == 0x60 ) && ( answer[ 1 ] == 0x08 ) ) {        /* is CORE_INTERFACE_ERROR_NTF ?*/
            break;
        }
        else if( ( answer[0] == 0x61 ) && ( answer[1] == 0x06 ) ) {             /* is RF_DEACTIVATE_NTF ? */
            break;                                                              /* Come back to discovery state */
        }
    }

    if( ( rf_intf->mode_tech & MODE_LISTEN ) != MODE_LISTEN ) {                 /* Is Initiator mode ? */ /* Communication ended, restart discovery loop */
        host_tx_rx( NCI_RESTART_DISCOVERY, sizeof( NCI_RESTART_DISCOVERY ), answer, sizeof( answer ), &answer_size );
        host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_2S );
    }
}
#endif

#ifdef RW_SUPPORT
static void reader_mode( nfc_interface_t *rf_intf )
{
    uint8_t answer[ ANSWER_MAX_SIZE ];
    uint8_t cmd[ 256 ]                        = { 0 };
    uint16_t cmd_size                   = 0;
    uint16_t answer_size                = 0;

    uint8_t NCI_PRES_CHECK_T2T[]          = { 0x00, 0x00, 0x02, 0x30, 0x00 };
    uint8_t NCI_POLLING_CMD_T3T[]          = { 0x21, 0x08, 0x04, 0x12, 0xFC, 0x00, 0x01 };
    uint8_t NCI_PRES_CHECK_ISO_DEP[] = { 0x2F, 0x11, 0x00 };
    uint8_t NCI_RESTART_DISCOVERY[]  = { 0x21, 0x06, 0x01, 0x03 };
    uint8_t NCI_PRES_CHECK_T1T[]          = { 0x00, 0x00, 0x07, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    rw_ndef_reset( rf_intf->protocol );

    if ( rf_intf->protocol == PROT_T3T ){                                      /* In case of T3T tag, retrieve card IDm */
        host_tx_rx( NCI_POLLING_CMD_T3T, sizeof( NCI_POLLING_CMD_T3T ), answer, sizeof( answer ), &answer_size );
        host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );

        if ( ( answer[ 0 ] == 0x61 ) && ( answer[ 1 ] == 0x08 ) && ( answer[ 3 ] == 0x00 ) )
            rw_ndef_t3t_set_id_m( &answer[ 6 ] );
        else
            goto restart_discovery;
    }

    while( 1 )
    {
        rw_ndef_read_next( &answer[ 3 ], answer[ 2 ], &cmd[ 3 ], ( uint16_t * ) &cmd_size );

        if( cmd_size == 0 ) {
            break;                                                              /* End of the Read operation */
        }
        else {

            cmd[ 0 ] = 0x00;                                                    /* Compute and send DATA_PACKET */
            cmd[ 1 ] = ( cmd_size & 0xFF00 ) >> 8;
            cmd[ 2 ] = cmd_size & 0x00FF;

            host_tx_rx( cmd, cmd_size + 3, answer, sizeof( answer ), &answer_size );
            host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_2S );
        }
    }

    switch( rf_intf->protocol )                                                 /* Read Operation ended, perform card presence check */
    {
    case PROT_T1T:

        do {
            nfc_hal_delay( 500 );
            host_tx_rx( NCI_PRES_CHECK_T1T, sizeof( NCI_PRES_CHECK_T1T ), answer, sizeof( answer ), &answer_size );
            host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );
        } while ( ( answer[ 0 ] == 0x00 ) && ( answer[ 1 ] == 0x00 ) );
        break;
    case PROT_T2T:

        do {
            nfc_hal_delay( 500 );
            host_tx_rx( NCI_PRES_CHECK_T2T, sizeof( NCI_PRES_CHECK_T2T ), answer, sizeof( answer ), &answer_size );
            host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );
        } while ( ( answer[ 0 ] == 0x00 ) && ( answer[ 1 ] == 0x00 ) && ( answer[ 2 ] == 0x11 ) );

        break;
    case PROT_ISODEP:

        do {
            nfc_hal_delay( 500 );
            host_tx_rx( NCI_PRES_CHECK_ISO_DEP, sizeof( NCI_PRES_CHECK_ISO_DEP ), answer, sizeof( answer ), &answer_size );
            host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );
        } while ( ( answer[ 0 ] == 0x6F ) && ( answer[ 1 ] == 0x11 ) && ( answer[ 2 ] == 0x01 ) && ( answer[ 3 ] == 0x01 ) );
        break;
    default:

        break;                                                                  /* protocol not supported */
    }
                                                                                /* Communication ended, restart discovery loop */
restart_discovery:

    host_tx_rx( NCI_RESTART_DISCOVERY, sizeof( NCI_RESTART_DISCOVERY ), answer, sizeof( answer ), &answer_size );
    host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );
}

int nfc_reader_tag_cmd
(
        uint8_t *command,
        uint8_t command_size,
        uint8_t *answer,
        uint8_t *answer_size
)
{
    int status = NFC_ERROR;
    uint8_t cmd[ 256 ];
    uint8_t ans[ ANSWER_MAX_SIZE ] = { 0 };
    uint16_t ans_size = 0;;
    
    cmd[ 0 ] = 0x00;                                                            /* Compute and send DATA_PACKET */
    cmd[ 1 ] = 0x00;
    cmd[ 2 ] = command_size;

    memcpy( &cmd[ 3 ], command, command_size );
    host_tx_rx( cmd, command_size + 3, ans, sizeof( ans ), &ans_size );         /* Wait for answer */
    host_rx( ans, sizeof( ans ), &ans_size, TIMEOUT_2S );

    if ( ( ans[ 0 ] == 0x0 ) && ( ans[ 1 ] == 0x0 ) )
        status = NFC_SUCCESS;

    *answer_size = ans[ 2 ];

    memcpy( answer, &ans[ 3 ], *answer_size );

    return status;
}
#endif


/******************************************************************************
* Public Function Definitions
*******************************************************************************/

int nfc_init
(
                uint8_t slave_address
)
{    
#if defined( TEST_MODE )

    int i, j;

    uint8_t answer[ANSWER_MAX_SIZE] = {0};
    uint16_t answer_size = 0;

    uint8_t CORE_SET_POWER_MODE_CMD[] = { 0x2f, 0x00, 0x01, 0x00 };
    uint8_t TEST_PRBS_CMD[] = { 0x2f, 0x30, 0x04, 0x00, 0x00, 0x00, 0x1f };
    uint8_t TEST_ANTENNA_CMD[] = { 0x2f, 0x3d, 0x05, 0x20, 0x00, 0x00, 0x00, 0x00 };

#endif
    
    if( slave_address != NFC_I2C_SLAVE && \
        slave_address != NFC_I2C_SLAVE_ALT )
       return NFC_ERROR;

    nfc_hal_init( slave_address );                                              /* Open connection to NFC click */

#if defined( TEST_MODE )

    UART_Write_Text( "\r\n*****Entering Test Mode*****\r\n" );
    initialize_core();                                                          /* Testing needs to disable standby mode.  */
    Delay_ms( 100 );
    UART_Write_Text( "\r\n*****Disabling Standby Mode*****\r\n" );              /* Note - disabling the Standby Mode should be restricted to debug sessions. */
    host_tx_rx( CORE_SET_POWER_MODE_CMD, sizeof( CORE_SET_POWER_MODE_CMD ), answer, sizeof( answer ), &answer_size );
    
    if( answer[ 3 ] == 0x00 )
        UART_Write_Text( "\r\n*****Standby OFF*****\r\n" );
    
    for( i = 0, j = 4; i < 4; i++, j++ )
    {
        UART_Write_Text( "\r\n*****Testing Antenna*****\r\n" );

        TEST_ANTENNA_CMD[ j ] |= 0x01;
        host_tx_rx( TEST_ANTENNA_CMD, sizeof( TEST_ANTENNA_CMD ), answer, sizeof( answer ), &answer_size );
        TEST_ANTENNA_CMD[ j ] &= ~0x01;
        host_tx_rx( TEST_ANTENNA_CMD, sizeof( TEST_ANTENNA_CMD ), answer, sizeof( answer ), &answer_size );
        host_tx_rx( TEST_ANTENNA_CMD, sizeof( TEST_ANTENNA_CMD ), answer, sizeof( answer ), &answer_size );
        TEST_ANTENNA_CMD[ j ] |= 0x01;
        host_tx_rx( TEST_ANTENNA_CMD, sizeof( TEST_ANTENNA_CMD ), answer, sizeof( answer ), &answer_size );
    }

    UART_Write_Text( "\r\n*****Starting Test Procedure*****\r\n" );
    host_tx_rx( TEST_PRBS_CMD, sizeof( TEST_PRBS_CMD ), answer, sizeof( answer ), &answer_size );

    if( answer[ 3 ] == 0x00 )
        UART_Write_Text( "\r\n******Test Successful*****\r\n" );
    else
        UART_Write_Text( "\r\n!!!!!!Test Failed!!!!!\r\n" );
    
    nfc_hal_reset();
    UART_Write_Text( "\r\n*****Testing Concluded Resetting*****\r\n" );
#endif
    
    if( initialize_core() )
        return NFC_ERROR;

    return NFC_SUCCESS;
}

void nfc_rx_ready()
{
    rx_flag = true;
}

void nfc_timer_tick()
{
    timer_tick++;
}

int nfc_configure( nfc_mode_t mode )
{
    uint8_t command[ 256 ];
    uint8_t answer[ ANSWER_MAX_SIZE ] = {0};
    uint16_t answer_size = 0;
    uint8_t item = 0;

    uint8_t NCI_DISCOVER_MAP_CMD[] = { 0x21, 0x00 };

#ifdef CARDEMU_SUPPORT

    uint8_t DM_CARDEMU[] = { 0x4, 0x2, 0x2 };
    uint8_t R_CARDEMU[]  = { 0x1, 0x3, 0x0, 0x1, 0x4 };
#endif
#ifdef P2P_SUPPORT

    uint8_t DM_P2P[] = { 0x5, 0x3, 0x3 };
    uint8_t R_P2P[]  = { 0x1, 0x3, 0x0, 0x1, 0x5 };
    uint8_t NCI_SET_CONFIG_NFC[] = { 0x20, 0x02, 0x1F, 0x02, 0x29, 0x0D, 0x46,
                    0x66, 0x6D, 0x01, 0x01, 0x11, 0x03, 0x02, 0x00, 0x01, 0x04, 0x01,
                    0xFA, 0x61, 0x0D, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x03, 0x02,
                    0x00, 0x01, 0x04, 0x01, 0xFA };
#endif
#ifdef RW_SUPPORT

    uint8_t NCI_PROPRIETARY_ACT_CMD[] = { 0x2F, 0x02, 0x00 };
    uint8_t DM_RW[] = { 0x1, 0x1, 0x1,  0x2, 0x1, 0x1,  0x3, 0x1, 0x1, 0x4, 0x1,
                    0x2, 0x80, 0x01, 0x80 };
#endif
#if defined( P2P_SUPPORT ) || defined( CARDEMU_SUPPORT )

    uint8_t NCI_ROUTING[] = { 0x21, 0x01, 0x07, 0x00, 0x01 };
    uint8_t nci_set_config_nfca_selrsp[] = { 0x20, 0x02, 0x04, 0x01, 0x32, 0x01,
                    0x00 };
#endif

    if( mode == NFC_MODE_NONE )
        return NFC_SUCCESS;

#ifdef RW_SUPPORT
                                                                                /* Enable Proprietary interface for T4T card presence check procedure */
    if( mode == NFC_MODE_RW ){
        if( host_tx_rx( NCI_PROPRIETARY_ACT_CMD, sizeof( NCI_PROPRIETARY_ACT_CMD ), answer, sizeof( answer ), &answer_size ) )
            return NFC_ERROR;
        else if( ( answer[0] != 0x4F ) || ( answer[1] != 0x02 ) || ( answer[3] != 0x00 ) )
            return NFC_ERROR;
    }
#endif

    item = 0;                                                                   /* Building Discovery Map command */

#ifdef CARDEMU_SUPPORT

    if( mode & NFC_MODE_CARDEMU ){
        memcpy( &command[ 4 + ( 3 * item ) ], ( void* )DM_CARDEMU, sizeof( DM_CARDEMU ) );
        item++;
    }
#endif
#ifdef P2P_SUPPORT

    if( mode & NFC_MODE_P2P ){
        memcpy( &command[ 4 + ( 3 * item ) ], ( void* )DM_P2P, sizeof( DM_P2P ) );
        item++;
    }
#endif
#ifdef RW_SUPPORT

    if( mode & NFC_MODE_RW ){
        memcpy( &command[ 4 + ( 3 * item ) ], ( void* )DM_RW, sizeof( DM_RW ) );
        item += sizeof( DM_RW ) / 3;
    }
#endif
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT || defined RW_SUPPORT

    if( item != 0 ){
        memcpy( command, ( void *)NCI_DISCOVER_MAP_CMD, sizeof( NCI_DISCOVER_MAP_CMD ) );
        command[ 2 ] = 1 + ( item * 3 );
        command[ 3 ] = item;
        
        if( host_tx_rx( command, command[ 2 ] + 3, answer, sizeof( answer ), &answer_size ) )
            return NFC_ERROR;
        else if( ( answer[0] != 0x41 ) || ( answer[1] != 0x00 ) || ( answer[3] != 0x00 ) )
            return NFC_ERROR;
    }
#endif

    item = 0;                                                                   /* Configuring routing */

#ifdef CARDEMU_SUPPORT

    if( mode & NFC_MODE_CARDEMU ){
        memcpy( &command[5 + ( 5 * item )], ( void* )R_CARDEMU, sizeof( R_CARDEMU ) );
        item++;
    }
#endif
#ifdef P2P_SUPPORT

    if( mode & NFC_MODE_P2P ){
        memcpy( &command[5 + ( 5 * item )], ( void* )R_P2P, sizeof( R_P2P ) );
        item++;
    }
#endif
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT

    if( item != 0 ) {
        memcpy( command, ( void* )NCI_ROUTING, sizeof( NCI_ROUTING ) );

        command[ 2 ] = 2 + ( item * 5 );
        command[ 4 ] = item;

        if( host_tx_rx( command, command[ 2 ] + 3, answer, sizeof( answer ), &answer_size ) )
            return NFC_ERROR;
        else if( ( answer[ 0 ] != 0x41 ) || ( answer[ 1 ] != 0x01 ) || ( answer[ 3 ] != 0x00 ) )
            return NFC_ERROR;
    }
#endif
                                                                                /* Setting NFCA SEL_RSP */
#ifdef CARDEMU_SUPPORT

    if( mode & NFC_MODE_CARDEMU )
        nci_set_config_nfca_selrsp[ 6 ] += 0x20;
#endif
#ifdef P2P_SUPPORT

    if( mode & NFC_MODE_P2P )
        nci_set_config_nfca_selrsp[ 6 ] += 0x40;
#endif
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT

    if( nci_set_config_nfca_selrsp[ 6 ] != 0x00 ){
        if( host_tx_rx( nci_set_config_nfca_selrsp, sizeof( nci_set_config_nfca_selrsp ), answer, sizeof( answer ), &answer_size ) )
            return NFC_ERROR;
        else if( ( answer[ 0 ] != 0x40 ) || ( answer[ 1 ] != 0x02 ) || ( answer[ 3 ] != 0x00 ) )
            return NFC_ERROR;
    }
#endif
                                                                                /* Setting LLCP support */
#ifdef P2P_SUPPORT

    if( mode & NFC_MODE_P2P ){
        if( host_tx_rx( NCI_SET_CONFIG_NFC, sizeof( NCI_SET_CONFIG_NFC ), answer, sizeof( answer ), &answer_size ) )
            return NFC_ERROR;
        else if( ( answer[0] != 0x40 ) || ( answer[1] != 0x02 ) || ( answer[3] != 0x00 ) )
            return NFC_ERROR;
    }
#endif

    return NFC_SUCCESS;
}

int nfc_start_discovery( uint8_t *p_tech_tab, uint8_t tech_tab_size )
{
    uint8_t command[ 256 ];
    uint8_t answer[ ANSWER_MAX_SIZE ] = { 0 };
    uint16_t answer_size = 0;
    uint8_t i;

    uint8_t NCI_DISCOVER_CMD[] = { 0x21, 0x03 };

    #if defined( DEBUG )
    UART_Write_Text( "*****Discovery Start*****\r\n" );
    #endif

    memcpy( command, ( void* )NCI_DISCOVER_CMD, sizeof( NCI_DISCOVER_CMD ) );
    //command[0] = 0x21;
    //command[1] = 0x03;
    command[ 2 ] = ( tech_tab_size * 2 ) + 1;
    command[ 3 ] = tech_tab_size;

    for ( i = 0; i < tech_tab_size; i++ )
    {
        command[ ( i * 2 ) + 4 ] = p_tech_tab[ i ];
        command[ ( i * 2 ) + 5 ] = 0x01;
    }

    if( host_tx_rx( command, ( tech_tab_size * 2 ) + 4, answer, sizeof( answer ), &answer_size ) )
        return NFC_ERROR;
    else if ( ( answer[ 0 ] != 0x41 ) || ( answer[ 1 ] != 0x03 ) || ( answer[ 3 ] != 0x00 ) )
        return NFC_ERROR;

    return NFC_SUCCESS;
}

int nfc_restart_discovery( void )
{
    uint8_t answer[ ANSWER_MAX_SIZE ] = { 0 };
    uint16_t answer_size = 0;
    uint8_t NCI_RESTART_DISCOVERY[] = { 0x21, 0x06, 0x01, 0x03 };

    if( host_tx_rx( NCI_RESTART_DISCOVERY, sizeof( NCI_RESTART_DISCOVERY ), answer, sizeof( answer ), &answer_size ) )
        return NFC_ERROR;
    else if( host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S ) )
        return NFC_ERROR;

    return NFC_SUCCESS;
}

int nfc_stop_discovery( void )
{
    uint8_t answer[ ANSWER_MAX_SIZE ] = {0};
    uint16_t answer_size = 0;
    uint8_t NCI_STOP_DISCOVERY[] = { 0x21, 0x06, 0x01, 0x00 };

    if( host_tx_rx( NCI_STOP_DISCOVERY, sizeof( NCI_STOP_DISCOVERY ), answer, sizeof( answer ), &answer_size ) )
        return NFC_ERROR;
    else if( host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S ) )
        return NFC_ERROR;

    return NFC_SUCCESS;
}

void nfc_wait_for_discovery_notification( nfc_interface_t *prf_intf )
{
    uint8_t NCIRfDiscoverSelectISO[] = {0x21, 0x04, 0x03, 0x01, PROT_ISODEP, INTF_ISODEP};
    uint8_t NCIRfDiscoverSelectNFC[] = {0x21, 0x04, 0x03, 0x01, PROT_NFCDEP, INTF_NFCDEP};
    uint8_t answer[ ANSWER_MAX_SIZE ] = { 0 };
    uint16_t answer_size = 0;

    do{
        host_rx( answer, sizeof( answer), &answer_size, TIMEOUT_INFINITE );
    } while( ( answer[ 0 ] != 0x61 ) || ( ( answer[ 1 ] != 0x05 ) && ( answer[ 1 ] != 0x03 ) ) );

    if( answer[ 1 ] == 0x05 ){                                                  /* Is RF_INTF_ACTIVATED_NTF ? */
        prf_intf->interface = answer[ 4 ];
        prf_intf->protocol = answer[ 5 ];
        prf_intf->mode_tech = answer[ 6 ];
    }
    else{
        prf_intf->interface = INTF_UNDETERMINED;                                /* RF_DISCOVER_NTF */
        prf_intf->protocol = answer[ 4 ];
        prf_intf->mode_tech = answer[ 5 ];

        while((answer[0] != 0x61) || (answer[1] != 0x03)  || (answer[ answer_size - 1 ] == 0x02 ))
            host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_1S );      /* Remaining NTF ? */

        if(prf_intf->protocol == PROT_ISODEP){                                  /* In case of multi-protocol card, select the ISODEP protocol */
           host_tx_rx(NCIRfDiscoverSelectISO, sizeof(NCIRfDiscoverSelectISO), answer, sizeof(answer), &answer_size);
           if ((answer[0] == 0x41) || (answer[1] == 0x04) || (answer[3] == 0x00)) {
                host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_100MS);
                if ((answer[0] == 0x61) || (answer[1] == 0x05)){
                    prf_intf->interface = answer[4];
                    prf_intf->protocol = answer[5];
                    prf_intf->mode_tech = answer[6];
                }
            }
        }
        else if (prf_intf->protocol == PROT_NFCDEP){                            /* In case of NFCDEP + ISODEP, select the P2P protocol */
            host_tx_rx(NCIRfDiscoverSelectNFC, sizeof(NCIRfDiscoverSelectNFC), answer, sizeof(answer), &answer_size);
            if ((answer[0] == 0x41) || (answer[1] == 0x04) || (answer[3] == 0x00)){
                host_rx( answer, sizeof( answer ), &answer_size, TIMEOUT_100MS);
                if ((answer[0] == 0x61) || (answer[1] == 0x05)){
                    prf_intf->interface = answer[4];
                    prf_intf->protocol = answer[5];
                    prf_intf->mode_tech = answer[6];
                }
            }
        }
    }
}

void nfc_process( uint8_t mode, nfc_interface_t *rf_intf )
{
    switch ( mode )
    {
#ifdef CARDEMU_SUPPORT

    case NFC_MODE_CARDEMU:
        card_mode( rf_intf );
        break;
#endif
#ifdef P2P_SUPPORT

    case NFC_MODE_P2P:
        p2p_mode( rf_intf );
        break;
#endif
#ifdef RW_SUPPORT

    case NFC_MODE_RW:
        reader_mode( rf_intf );
        break;
#endif

    default:
        break;
    }
}

/*************** END OF FUNCTIONS *******************************************/