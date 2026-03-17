/*
 *  send_email.c: Send PDF certificate via email after wipe completion (wype)
 *
 *  Implements a minimal raw SMTP client (no auth, no TLS) for sending
 *  PDF certificates as email attachments on internal networks.
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2.
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <libconfig.h>

#include "wype.h"
#include "context.h"
#include "logging.h"
#include "conf.h"
#include "send_email.h"

/* Base64 encoding table */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Base64 encode a block of data.
 * Caller must free() the returned string.
 */
static char* base64_encode( const unsigned char* data, size_t input_length, size_t* output_length )
{
    *output_length = 4 * ( ( input_length + 2 ) / 3 );

    /* Add space for line breaks every 76 chars (MIME requirement) and null terminator */
    size_t line_breaks = *output_length / 76;
    char* encoded = malloc( *output_length + line_breaks * 2 + 1 );
    if( !encoded )
        return NULL;

    size_t i, j, col;
    for( i = 0, j = 0, col = 0; i < input_length; )
    {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = ( octet_a << 16 ) | ( octet_b << 8 ) | octet_c;

        encoded[j++] = b64_table[( triple >> 18 ) & 0x3F];
        col++;
        encoded[j++] = b64_table[( triple >> 12 ) & 0x3F];
        col++;
        encoded[j++] = b64_table[( triple >> 6 ) & 0x3F];
        col++;
        encoded[j++] = b64_table[triple & 0x3F];
        col++;

        if( col >= 76 )
        {
            encoded[j++] = '\r';
            encoded[j++] = '\n';
            col = 0;
        }
    }

    /* Add padding */
    size_t mod = input_length % 3;
    if( mod == 1 )
    {
        encoded[j - 1] = '=';
        encoded[j - 2] = '=';
    }
    else if( mod == 2 )
    {
        encoded[j - 1] = '=';
    }

    encoded[j] = '\0';
    *output_length = j;
    return encoded;
}

/**
 * Read SMTP response and check status code.
 * Returns 0 if response starts with expected code, -1 otherwise.
 */
static int smtp_check_response( int sockfd, const char* expected_code )
{
    char response[1024];
    ssize_t n = recv( sockfd, response, sizeof( response ) - 1, 0 );
    if( n <= 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: No response from SMTP server" );
        return -1;
    }
    response[n] = '\0';

    if( strncmp( response, expected_code, strlen( expected_code ) ) != 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Unexpected SMTP response: %s", response );
        return -1;
    }
    return 0;
}

/**
 * Send a string over the socket.
 */
static int smtp_send( int sockfd, const char* data, size_t len )
{
    ssize_t sent = send( sockfd, data, len, 0 );
    if( sent < 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Failed to send data: %s", strerror( errno ) );
        return -1;
    }
    return 0;
}

/**
 * Send a null-terminated string.
 */
static int smtp_send_str( int sockfd, const char* str )
{
    return smtp_send( sockfd, str, strlen( str ) );
}

/**
 * Extract basename from a file path.
 */
static const char* get_basename( const char* path )
{
    const char* p = strrchr( path, '/' );
    return p ? p + 1 : path;
}

/**
 * Read SMTP settings from wype.conf.
 * Returns 0 on success, -1 if email is disabled or misconfigured.
 */
static int read_email_settings( const char** smtp_server,
                                const char** smtp_port_str,
                                const char** sender,
                                const char** recipient )
{
    extern config_t wype_cfg;
    extern char wype_config_file[];

    const char* email_enable = NULL;

    config_setting_t* setting = config_lookup( &wype_cfg, "Email_Settings" );
    if( setting == NULL )
    {
        wype_log( WYPE_LOG_WARNING, "Email: Cannot locate [Email_Settings] in %s", wype_config_file );
        return -1;
    }

    config_setting_lookup_string( setting, "Email_Enable", &email_enable );
    if( email_enable == NULL || strcasecmp( email_enable, "ENABLED" ) != 0 )
    {
        return -1; /* Email disabled, silently skip */
    }

    config_setting_lookup_string( setting, "SMTP_Server", smtp_server );
    config_setting_lookup_string( setting, "SMTP_Port", smtp_port_str );
    config_setting_lookup_string( setting, "Sender_Address", sender );
    config_setting_lookup_string( setting, "Recipient_Address", recipient );

    if( !*smtp_server || !*sender || !*recipient || strlen( *recipient ) == 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Missing SMTP configuration (server/sender/recipient)" );
        return -1;
    }

    return 0;
}

/**
 * Connect to SMTP server. Returns socket fd or -1 on failure.
 */
static int smtp_connect( const char* smtp_server, const char* smtp_port_str )
{
    int port = smtp_port_str ? atoi( smtp_port_str ) : 25;

    struct addrinfo hints, *res;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf( port_str, sizeof( port_str ), "%d", port );

    int gai_err = getaddrinfo( smtp_server, port_str, &hints, &res );
    if( gai_err != 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Cannot resolve SMTP server %s: %s", smtp_server, gai_strerror( gai_err ) );
        return -1;
    }

    int sockfd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
    if( sockfd < 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Socket creation failed: %s", strerror( errno ) );
        freeaddrinfo( res );
        return -1;
    }

    /* Set socket timeout (10 seconds) */
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) );
    setsockopt( sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof( tv ) );

    if( connect( sockfd, res->ai_addr, res->ai_addrlen ) < 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Cannot connect to %s:%d: %s", smtp_server, port, strerror( errno ) );
        close( sockfd );
        freeaddrinfo( res );
        return -1;
    }
    freeaddrinfo( res );

    wype_log( WYPE_LOG_INFO, "Email: Connected to SMTP server %s:%d", smtp_server, port );
    return sockfd;
}

/**
 * Perform SMTP handshake (greeting + EHLO + MAIL FROM + RCPT TO + DATA).
 * Returns 0 on success, -1 on failure.
 */
static int smtp_handshake( int sockfd, const char* sender, const char* recipient )
{
    char cmd[512];

    if( smtp_check_response( sockfd, "220" ) != 0 )
        return -1;

    snprintf( cmd, sizeof( cmd ), "EHLO wype\r\n" );
    if( smtp_send_str( sockfd, cmd ) != 0 )
        return -1;
    if( smtp_check_response( sockfd, "250" ) != 0 )
        return -1;

    snprintf( cmd, sizeof( cmd ), "MAIL FROM:<%s>\r\n", sender );
    if( smtp_send_str( sockfd, cmd ) != 0 )
        return -1;
    if( smtp_check_response( sockfd, "250" ) != 0 )
        return -1;

    snprintf( cmd, sizeof( cmd ), "RCPT TO:<%s>\r\n", recipient );
    if( smtp_send_str( sockfd, cmd ) != 0 )
        return -1;
    if( smtp_check_response( sockfd, "250" ) != 0 )
        return -1;

    if( smtp_send_str( sockfd, "DATA\r\n" ) != 0 )
        return -1;
    if( smtp_check_response( sockfd, "354" ) != 0 )
        return -1;

    return 0;
}

int wype_send_summary_notification( wype_context_t** c, int count )
{
    const char* smtp_server = NULL;
    const char* smtp_port_str = NULL;
    const char* sender = NULL;
    const char* recipient = NULL;

    if( read_email_settings( &smtp_server, &smtp_port_str, &sender, &recipient ) != 0 )
        return -1;

    /* Count success/fail by checking the actual result fields, because
     * wipe_status_txt may not be populated yet at this point. */
    int success = 0;
    int failed = 0;
    int aborted = 0;
    extern int user_abort;
    for( int i = 0; i < count; i++ )
    {
        if( c[i]->pass_errors != 0 || c[i]->verify_errors != 0 || c[i]->fsyncdata_errors != 0 )
            failed++;
        else if( c[i]->wipe_status == 0 )
            success++;
        else if( c[i]->wipe_status == 1 && user_abort == 1 )
            aborted++;
        else
            failed++;
    }

    int sockfd = smtp_connect( smtp_server, smtp_port_str );
    if( sockfd < 0 )
        return -1;

    int result = -1;

    do
    {
        if( smtp_handshake( sockfd, sender, recipient ) != 0 )
            break;

        /* Build plain text email */
        char header[2048];
        time_t now = time( NULL );
        struct tm* tm_info = localtime( &now );
        char date_str[64];
        strftime( date_str, sizeof( date_str ), "%a, %d %b %Y %H:%M:%S %z", tm_info );

        /* Get hostname */
        char hostname[256];
        if( gethostname( hostname, sizeof( hostname ) ) != 0 )
            strncpy( hostname, "unknown", sizeof( hostname ) );

        snprintf( header,
                  sizeof( header ),
                  "From: <%s>\r\n"
                  "To: <%s>\r\n"
                  "Date: %s\r\n"
                  "Subject: Wype - Wipe completed (%d OK, %d failed, %d aborted)\r\n"
                  "Content-Type: text/plain; charset=utf-8\r\n"
                  "\r\n"
                  "Wype wipe process completed.\r\n"
                  "\r\n"
                  "Host: %s\r\n"
                  "Total drives: %d\r\n"
                  "Successful: %d\r\n"
                  "Failed: %d\r\n"
                  "Aborted: %d\r\n"
                  "\r\n"
                  "Please confirm on device by pressing Enter.\r\n"
                  "PDF certificates will be sent by email after confirmation.\r\n"
                  "\r\n",
                  sender,
                  recipient,
                  date_str,
                  success,
                  failed,
                  aborted,
                  hostname,
                  count,
                  success,
                  failed,
                  aborted );

        if( smtp_send_str( sockfd, header ) != 0 )
            break;

        /* End DATA */
        if( smtp_send_str( sockfd, ".\r\n" ) != 0 )
            break;
        if( smtp_check_response( sockfd, "250" ) != 0 )
            break;

        smtp_send_str( sockfd, "QUIT\r\n" );

        result = 0;
        wype_log( WYPE_LOG_INFO,
                   "Email: Summary notification sent to %s (%d OK, %d failed, %d aborted)",
                   recipient,
                   success,
                   failed,
                   aborted );

    } while( 0 );

    if( result != 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Failed to send summary notification" );
    }

    close( sockfd );
    return result;
}

int wype_send_all_certificates( wype_context_t** c, int count )
{
    const char* smtp_server = NULL;
    const char* smtp_port_str = NULL;
    const char* sender = NULL;
    const char* recipient = NULL;

    if( read_email_settings( &smtp_server, &smtp_port_str, &sender, &recipient ) != 0 )
        return -1;

    /* Count how many unique PDFs we have */
    int pdf_count = 0;
    for( int i = 0; i < count; i++ )
    {
        if( c[i]->PDF_filename[0] == '\0' )
            continue;
        int dup = 0;
        for( int k = 0; k < i; k++ )
        {
            if( strcmp( c[k]->PDF_filename, c[i]->PDF_filename ) == 0 )
            {
                dup = 1;
                break;
            }
        }
        if( !dup )
            pdf_count++;
    }

    if( pdf_count == 0 )
    {
        wype_log( WYPE_LOG_WARNING, "Email: No PDF certificates to send" );
        return -1;
    }

    int sockfd = smtp_connect( smtp_server, smtp_port_str );
    if( sockfd < 0 )
        return -1;

    int result = -1;

    do
    {
        if( smtp_handshake( sockfd, sender, recipient ) != 0 )
            break;

        /* Build MIME header */
        char header[4096];
        time_t now = time( NULL );
        struct tm* tm_info = localtime( &now );
        char date_str[64];
        strftime( date_str, sizeof( date_str ), "%a, %d %b %Y %H:%M:%S %z", tm_info );

        /* Count success/fail for subject line */
        int success = 0;
        int failed = 0;
        for( int i = 0; i < count; i++ )
        {
            if( strcmp( c[i]->wipe_status_txt, "ERASED" ) == 0 )
                success++;
            else
                failed++;
        }

        /* Build summary text for the email body */
        char body_text[4096];
        int body_offset = 0;
        body_offset += snprintf( body_text + body_offset,
                                 sizeof( body_text ) - body_offset,
                                 "Wype Erasure Certificates\r\n"
                                 "\r\n"
                                 "Total drives: %d\r\n"
                                 "Successful: %d\r\n"
                                 "Failed: %d\r\n"
                                 "\r\n"
                                 "Details:\r\n",
                                 count,
                                 success,
                                 failed );

        for( int i = 0; i < count; i++ )
        {
            body_offset += snprintf( body_text + body_offset,
                                     sizeof( body_text ) - body_offset,
                                     "  - %s (S/N: %s): %s\r\n",
                                     c[i]->device_model ? c[i]->device_model : "Unknown",
                                     c[i]->device_serial_no,
                                     c[i]->wipe_status_txt );
        }

        snprintf( header,
                  sizeof( header ),
                  "From: <%s>\r\n"
                  "To: <%s>\r\n"
                  "Date: %s\r\n"
                  "Subject: Wype Erasure Certificates - %d OK, %d failed\r\n"
                  "MIME-Version: 1.0\r\n"
                  "Content-Type: multipart/mixed; boundary=\"wype-cert-boundary\"\r\n"
                  "\r\n"
                  "--wype-cert-boundary\r\n"
                  "Content-Type: text/plain; charset=utf-8\r\n"
                  "\r\n"
                  "%s"
                  "\r\n",
                  sender,
                  recipient,
                  date_str,
                  success,
                  failed,
                  body_text );

        if( smtp_send_str( sockfd, header ) != 0 )
            break;

        /* Attach each unique PDF (skip duplicates from grouped certificates) */
        int attach_ok = 1;
        for( int i = 0; i < count; i++ )
        {
            if( c[i]->PDF_filename[0] == '\0' )
                continue;

            /* Skip if this PDF was already attached by an earlier disk */
            int already_attached = 0;
            for( int k = 0; k < i; k++ )
            {
                if( strcmp( c[k]->PDF_filename, c[i]->PDF_filename ) == 0 )
                {
                    already_attached = 1;
                    break;
                }
            }
            if( already_attached )
                continue;

            FILE* pdf_file = fopen( c[i]->PDF_filename, "rb" );
            if( !pdf_file )
            {
                wype_log( WYPE_LOG_ERROR, "Email: Cannot open PDF %s: %s", c[i]->PDF_filename, strerror( errno ) );
                continue; /* Skip this PDF but try others */
            }

            fseek( pdf_file, 0, SEEK_END );
            long pdf_size = ftell( pdf_file );
            fseek( pdf_file, 0, SEEK_SET );

            unsigned char* pdf_data = malloc( pdf_size );
            if( !pdf_data )
            {
                fclose( pdf_file );
                wype_log( WYPE_LOG_ERROR, "Email: Failed to allocate memory for PDF (%ld bytes)", pdf_size );
                continue;
            }

            if( fread( pdf_data, 1, pdf_size, pdf_file ) != (size_t) pdf_size )
            {
                free( pdf_data );
                fclose( pdf_file );
                wype_log( WYPE_LOG_ERROR, "Email: Failed to read PDF %s", c[i]->PDF_filename );
                continue;
            }
            fclose( pdf_file );

            size_t b64_len;
            char* b64_data = base64_encode( pdf_data, pdf_size, &b64_len );
            free( pdf_data );

            if( !b64_data )
            {
                wype_log( WYPE_LOG_ERROR, "Email: Failed to base64 encode PDF" );
                continue;
            }

            const char* pdf_basename = get_basename( c[i]->PDF_filename );

            /* MIME part header for this attachment */
            char part_header[512];
            snprintf( part_header,
                      sizeof( part_header ),
                      "--wype-cert-boundary\r\n"
                      "Content-Type: application/pdf\r\n"
                      "Content-Disposition: attachment; filename=\"%s\"\r\n"
                      "Content-Transfer-Encoding: base64\r\n"
                      "\r\n",
                      pdf_basename );

            if( smtp_send_str( sockfd, part_header ) != 0 )
            {
                free( b64_data );
                attach_ok = 0;
                break;
            }

            /* Send base64 encoded PDF in chunks */
            size_t offset = 0;
            size_t chunk_size = 4096;
            while( offset < b64_len )
            {
                size_t remaining = b64_len - offset;
                size_t to_send = remaining < chunk_size ? remaining : chunk_size;
                if( smtp_send( sockfd, b64_data + offset, to_send ) != 0 )
                    break;
                offset += to_send;
            }

            free( b64_data );

            if( offset < b64_len )
            {
                attach_ok = 0;
                break;
            }

            if( smtp_send_str( sockfd, "\r\n" ) != 0 )
            {
                attach_ok = 0;
                break;
            }
        }

        if( !attach_ok )
            break;

        /* End MIME and DATA */
        if( smtp_send_str( sockfd, "--wype-cert-boundary--\r\n.\r\n" ) != 0 )
            break;
        if( smtp_check_response( sockfd, "250" ) != 0 )
            break;

        smtp_send_str( sockfd, "QUIT\r\n" );

        result = 0;
        wype_log( WYPE_LOG_INFO,
                   "Email: All %d certificates sent successfully to %s",
                   pdf_count,
                   recipient );

    } while( 0 );

    if( result != 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Email: Failed to send certificates" );
    }

    close( sockfd );

    /* Delete local PDFs on success, keep on failure (skip duplicates from grouped certs) */
    for( int i = 0; i < count; i++ )
    {
        if( c[i]->PDF_filename[0] == '\0' )
            continue;

        /* Skip if already handled by an earlier disk entry */
        int already_handled = 0;
        for( int k = 0; k < i; k++ )
        {
            if( strcmp( c[k]->PDF_filename, c[i]->PDF_filename ) == 0 )
            {
                already_handled = 1;
                break;
            }
        }
        if( already_handled )
            continue;

        if( result == 0 )
        {
            if( unlink( c[i]->PDF_filename ) == 0 )
            {
                wype_log( WYPE_LOG_INFO, "Email: Deleted local PDF %s", c[i]->PDF_filename );
            }
            else
            {
                wype_log( WYPE_LOG_WARNING, "Email: Could not delete PDF %s: %s", c[i]->PDF_filename, strerror( errno ) );
            }
        }
        else
        {
            wype_log( WYPE_LOG_WARNING, "Email: Keeping local PDF %s (email send failed)", c[i]->PDF_filename );
        }
    }

    return result;
}
