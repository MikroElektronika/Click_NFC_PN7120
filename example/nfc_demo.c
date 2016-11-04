/*******************************************************************************
* Title                 :   System Initialization
* Filename              :   sys_init.c
* Author                :   JWB
* Origin Date           :   04/23/2012
* Notes                 :   None
*******************************************************************************/
/*************** MODULE REVISION LOG ******************************************
*
*    Date    Software Version    Initials   Description
*  XX/XX/XX    XXXXXXXXXXX         JWB      Module Created.
*
*******************************************************************************/
/** @file XXX.c
 *  @brief This module contains the
 */
/******************************************************************************
* Includes
*******************************************************************************/

#include "nfc_demo.h"

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static uint8_t mode;
volatile bool incoming_flag;

#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
static char NDEF_RECORD[] = { 0xD1,                                              // MB / ME / CF / 1 / IL / TNF
                             0x01,                                              // TYPE LENGTH
                             51,                                                // PAYLOAD LENTGH
                             'T',                                               // TYPE
                             0x02,                                              // Status
                             'e', 'n',                                          // Language
13, 10, 13, 10, 32, 32, 32, 32, 32, 
'M', 'i', 'k', 'r', 'o', 'E', 'l', 'e', 'k', 't', 'r', 'o', 'n', 'i', 'k', 'a', 
13, 10, 13, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
'N', 'F', 'C', ' ', 'c', 'l', 'i', 'c', 'k'
};
#endif

uint8_t discovery_technologies[] = { MODE_POLL | TECH_PASSIVE_NFCA
                                   , MODE_POLL | TECH_PASSIVE_NFCB
                                   , MODE_POLL | TECH_PASSIVE_NFCF
#ifdef P2P_SUPPORT
                                   , MODE_POLL | TECH_ACTIVE_NFCF
#endif
                                   , MODE_LISTEN | TECH_PASSIVE_NFCA
#ifdef P2P_SUPPORT
                                   , MODE_LISTEN | TECH_PASSIVE_NFCF
                                   , MODE_LISTEN | TECH_ACTIVE_NFCA
                                   , MODE_LISTEN | TECH_ACTIVE_NFCF
#endif
                                   };

/******************************************************************************
* Function Definitions
*******************************************************************************/

#if defined P2P_SUPPORT || defined RW_SUPPORT

char *auth( uint8_t x )
{
    switch( x )
    {
        case 0x01:
            return "Open";
        case 0x02:
            return "WPA-Personal";
        case 0x04:
            return "Shared";
        case 0x08:
            return "WPA-Entreprise";
        case 0x10:
            return "WPA2-Entreprise";
        case 0x20:
            return "WPA2-Personal";
        default:
            return "unknown";
    }
}

char *encrypt( uint8_t x )
{
    switch( x )
    {
        case 0x01:
            return "None";
        case 0x02:
            return "WEP";
        case 0x04:
            return "TKIP";
        case 0x08:
            return "AES";
        case 0x10:
            return "AES/TKIP";
        default:
            return "unknown";
    }
}

void ndef_pull_cb( uint8_t *p_ndef_record, uint16_t ndef_record_size )
{
    char tmp_txt[ 80 ];                                                         /* The callback only print out the received message */
    UART_Write_Text( "--- NDEF Record received:\n" );

    if( p_ndef_record[ 0 ] == 0xD1 )                                            /* Only short, not fragmented and well-known type records are supported here */
    {
        switch( p_ndef_record[ 3 ] )
        {
            case 'T':
                p_ndef_record[ 7 + p_ndef_record[ 2 ] ] = '\0';
                sprinti( tmp_txt,
                         "   Text record (language = %c%c): %s\n",
                         p_ndef_record[ 5 ],
                         p_ndef_record[ 6 ],
                         &p_ndef_record[ 7 ] );
                UART_Write_Text( tmp_txt );
                break;

            case 'U':
                UART_Write_Text( "   URI record: " );
                NDEF_PRINT_URI_CODE( p_ndef_record[ 4 ] )
                p_ndef_record[ 4 + p_ndef_record[ 2 ] ] = '\0';
                sprinti( tmp_txt, "%s\n", &p_ndef_record[ 5 ] );
                UART_Write_Text( tmp_txt );
                break;

            default:
                UART_Write_Text( "   Unsupported NDEF record, only 'T' and 'U' types are supported\n" );
                break;
        }
    }                                                                           /* Only short, not fragmented and WIFI handover type are supported here */
    else if( ( p_ndef_record[ 0 ] == 0xD2 ) &&
             ( memcmp( &p_ndef_record[ 3 ], "application/vnd.wfa.wsc",
                       sizeof( "application/vnd.wfa.wsc" ) ) ) )
    {
        uint8_t index = 26, i;

        UART_Write_Text( "--- Received WIFI credentials:\n" );

        if( ( p_ndef_record[ index ] == 0x10 ) &&
                ( p_ndef_record[ index + 1 ] == 0x0E ) )
            index += 4;

        while( index < ndef_record_size )
        {
            if( p_ndef_record[ index ] == 0x10 )
            {
                if ( p_ndef_record[ index + 1 ] == 0x45 )
                {
                    UART_Write_Text ( "- SSID = " );

                    for( i = 0; i < p_ndef_record[ index + 3 ]; i++ )
                    {
                        sprinti( tmp_txt, "%c", p_ndef_record[ index + 4 + i ] );
                        UART_Write_Text( tmp_txt );
                    }
                    UART_Write_Text ( "\n" );
                }
                else if ( p_ndef_record[ index + 1 ] == 0x03 )
                {
                    sprinti( tmp_txt, "- Authenticate Type = %s\n",
                             auth( p_ndef_record[ index + 5 ] ) );
                    UART_Write_Text( tmp_txt );
                }
                else if( p_ndef_record[index + 1] == 0x0f )
                {
                    sprinti( tmp_txt, "- Encryption Type = %s\n",
                             encrypt( p_ndef_record[index + 5] ) );
                    UART_Write_Text( tmp_txt );
                }
                else if ( p_ndef_record[ index + 1 ] == 0x27 )
                {
                    UART_Write_Text ( "- Network key = " );

                    for( i = 0; i < p_ndef_record[ index + 3 ]; i++ )
                        UART_Write_Text( "#" );
                    UART_Write_Text ( "\n" );
                }

                index += 4 + p_ndef_record[ index + 3 ];
            }
            else
                continue;
        }
    }
    else
        UART_Write_Text( "   Unsupported NDEF record, cannot parse\n" );

    UART_Write_Text( "\n" );
}
#endif

#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
void ndef_push_cb( uint8_t *p_ndef_record, uint16_t ndef_record_size )
{
    UART_Write_Text( "--- NDEF Record sent\n\n" );
}
#endif


void mi_fare_scenario()
{
    bool status;
    uint8_t i;
    uint8_t resp[256];
    uint8_t resp_size;
    char tmp_txt[ 80 ] = { 0 };
    uint8_t auth[] = { 0x40, 0x01, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  /* Authenticate sector 1 with generic keys */
    uint8_t read[] = { 0x10, 0x30, 0x04 };                                      /* Read block 4 */
    uint8_t write_part1[] = { 0x10, 0xA0, 0x04 };                                                                                                                                                        // Write block 4
    uint8_t write_part2[] = { 0x10, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                    0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
                                                                                /* Authenticate */
    status = nfc_reader_tag_cmd( auth,
                                 sizeof( auth ),
                                 resp,
                                 &resp_size );

    if( ( status == 1 ) || ( resp[ resp_size - 1 ] != 0 ) ){
        sprinti( tmp_txt, " Authenticate sector %d failed with error 0x%02x\n", auth[ 1 ], resp[ resp_size - 1 ] );
        UART_Write_Text( tmp_txt );/* Read block */
        return;
    }
    sprinti( tmp_txt, " Authenticate sector %d succeed\n", auth[ 1 ] );
    UART_Write_Text( tmp_txt );
                                                                                /* Read block */
    status = nfc_reader_tag_cmd( read, sizeof( read ), resp, &resp_size );

    if( ( status == 1 ) || ( resp[ resp_size - 1 ] != 0 ) ){
        sprinti( tmp_txt, " Read block %d failed with error 0x%02x\n", read[ 2 ], resp[ resp_size - 1 ] );
        UART_Write_Text( tmp_txt );
        return;
    }

    sprinti( tmp_txt, " Read block %d: ", read[2] );
    UART_Write_Text( tmp_txt );

    for( i = 0; i < resp_size - 2; i++ )
    {
        sprinti( tmp_txt, "0x%02X ", resp[i + 1] );
        UART_Write_Text( tmp_txt );
    }
    UART_Write_Text( "\n" );
                                                                                /* Write block */
    status = nfc_reader_tag_cmd( write_part1, sizeof( write_part1 ), resp, &resp_size );

    if( ( status == 1 ) || ( resp[resp_size - 1] != 0 ) ){
        sprinti( tmp_txt, " Write block %d failed with error 0x%02x\n", write_part1[2], resp[resp_size - 1] );
        UART_Write_Text( tmp_txt );
        return;
    }

    status = nfc_reader_tag_cmd( write_part2, sizeof( write_part2 ), resp, &resp_size );

    if( ( status == 1 ) || ( resp[resp_size - 1] != 0 ) ){
        sprinti( tmp_txt, " Write block %d failed with error 0x%02x\n", write_part1[2], resp[resp_size - 1] );
        UART_Write_Text( tmp_txt );
        return;
    }

    sprinti( tmp_txt, " Block %d written\n", write_part1[2] );
    UART_Write_Text( tmp_txt );
                                                                                /* Read block */
    status = nfc_reader_tag_cmd( read, sizeof( read ), resp, &resp_size );

    if( ( status == 1 ) || ( resp[resp_size - 1] != 0 ) )
    {
        sprinti( tmp_txt, " Read failed with error 0x%02x\n", resp[ resp_size - 1 ] );
        UART_Write_Text( tmp_txt );
        return;
    }

    sprinti( tmp_txt, " Read block %d: ", read[2] );
    UART_Write_Text( tmp_txt );

    for( i = 0; i < resp_size - 2; i++ )
    {
        sprinti( tmp_txt, "0x%02X ", resp[i + 1] );
        UART_Write_Text( tmp_txt );
    }
    UART_Write_Text( "\n" );

    while( 1 )                                                                  /* Perform presence check */
    {
        Delay_ms( 500 );
        status = nfc_reader_tag_cmd( read, sizeof( read ), resp, &resp_size );

        if( ( status == 1 ) || ( resp[resp_size - 1] == 0xb2 ) )
            break;
    }
}

void process_radio
(
                nfc_interface_t *radio
)
{
    char tmp_txt[ 80 ];

#ifdef CARDEMU_SUPPORT

    if( ( radio->interface == INTF_ISODEP ) && ( radio->mode_tech == ( MODE_LISTEN | TECH_PASSIVE_NFCA ) ) ){
        UART_Write_Text( " - LISTEN MODE: Activated from remote Reader\r\n" );
        nfc_process( NFC_MODE_CARDEMU, radio );
        UART_Write_Text( "READER DISCONNECTED\r\n" );
    }
    else
#endif
#ifdef P2P_SUPPORT

        if( radio->interface == INTF_NFCDEP ){
            if( ( radio->mode_tech & MODE_LISTEN ) == MODE_LISTEN )             /* Is target mode ? */
                UART_Write_Text( " - P2P TARGET MODE: Remote Initiator\r\n" );
            else
                UART_Write_Text( " - P2P INITIATOR MODE: Remote Target activated\r\n" );

            nfc_process( NFC_MODE_P2P, radio );
            UART_Write_Text( "PEER LOST\r\n" );
        }
        else
#endif
#ifdef RW_SUPPORT

            if( ( radio->mode_tech & MODE_MASK ) == MODE_POLL )
            {
                if( radio->protocol == PROT_MIFARE )                            /* Is card detected MIFARE ?*/
                {
                    UART_Write_Text( " - POLL MODE: Remote MIFARE card activated\r\n" );
                    mi_fare_scenario();
                    nfc_restart_discovery();                                    /* Restart discovery loop */
                }
                else if( ( radio->protocol != PROT_NFCDEP ) && \
                                 ( radio->interface != INTF_UNDETERMINED ) )    /* Is Undetermined target ?*/
                {
                    sprinti( tmp_txt, " - POLL MODE: Remote T%dT activated\r\n",
                             radio->protocol );
                    UART_Write_Text( tmp_txt );
                    nfc_process( NFC_MODE_RW, radio );
                }
                else
                {
                    UART_Write_Text( " - POLL MODE: Undetermined target\r\n" );
                    nfc_stop_discovery();                                       /* Restart discovery loop */
                    nfc_start_discovery( discovery_technologies,
                                         sizeof( discovery_technologies ) );
                }

                UART_Write_Text( "CARD DISCONNECTED\r\n" );
            }
            else
#endif
            {
                UART_Write_Text( "WRONG DISCOVERY\r\n" );
            }

}

int example_init()
{
    mode = 0;
    
#ifdef CARDEMU_SUPPORT
                                                                                /* Register NDEF message to be sent to remote reader */
    t4t_ndef_emu_set_record( ( uint8_t * )NDEF_RECORD,
                             sizeof( NDEF_RECORD ),
                             ( void* )ndef_push_cb );
#endif

#ifdef P2P_SUPPORT
                                                                                /* Register NDEF message to be sent to remote peer */
    p2p_ndef_set_record( ( uint8_t * )NDEF_RECORD,
                         sizeof( NDEF_RECORD ),
                         ( void* )ndef_push_cb );

    p2p_ndef_register_pull_callback( ( void* )ndef_pull_cb );                  /* Register callback for reception of NDEF message from remote peer */
#endif

#ifdef RW_SUPPORT

    rw_ndef_register_pull_callback( ( void* )ndef_pull_cb );                   /* Register callback for reception of NDEF message from remote cards */
#endif

#ifdef CARDEMU_SUPPORT

    mode |= NFC_MODE_CARDEMU;                                                   /* Set NXPNCI in all modes */
#endif
#ifdef P2P_SUPPORT

    mode |= NFC_MODE_P2P;
#endif
#ifdef RW_SUPPORT

    mode |= NFC_MODE_RW;
#endif

    if( nfc_init( NFC_I2C_SLAVE ) )                                             /* Open connection to NXPNCI device */
    {
        UART_Write_Text( "Error: cannot connect to NFC click\r\n" );
        return -1;
    }
    else if( nfc_configure( mode ) )
    {
        UART_Write_Text( "Error: cannot configure NFC click\r\n" );
        return -1;
    }
    else if( nfc_start_discovery( discovery_technologies,
                                  sizeof( discovery_technologies ) ) )
    {
        UART_Write_Text( "Error: cannot start discovery\n" );
        return -1;
    }

    UART_Write_Text( "\r\nNFC Successfully Initialized\r\n" );
    incoming_flag = false;
}