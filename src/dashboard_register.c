/*
 *  dashboard_register.c: Self-registration with a central wype-dashboard.
 *
 *  Spawns a background thread that POSTs to the dashboard's
 *  /api/v1/register endpoint every 30 seconds.  Uses a raw TCP socket
 *  (same pattern as send_email.c) so that libcurl is not required.
 *
 *  Guarded by HAVE_API_SERVER – when the flag is absent the public
 *  functions compile as harmless no-ops.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dashboard_register.h"

#ifdef HAVE_API_SERVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include <cjson/cJSON.h>

#include "context.h"
#include "logging.h"
#include "version.h"

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */

#define REGISTER_INTERVAL_SEC 30
#define REGISTER_TIMEOUT_SEC  5
#define MAX_HOST_LEN          256
#define HTTP_BUF_SIZE         2048

/* ------------------------------------------------------------------ */
/*  Module state                                                       */
/* ------------------------------------------------------------------ */

static pthread_t  reg_thread;
static int        reg_running = 0;
static volatile int reg_stop  = 0;

static char reg_host[MAX_HOST_LEN]  = "";
static char reg_port_str[8]         = "8080";
static char reg_api_key[256]        = "";
static int  reg_local_api_port      = 5000;

/* ------------------------------------------------------------------ */
/*  TCP helpers (mirroring send_email.c)                               */
/* ------------------------------------------------------------------ */

static int tcp_connect( const char* host, const char* port )
{
    struct addrinfo hints, *res;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int gai = getaddrinfo( host, port, &hints, &res );
    if( gai != 0 )
    {
        wype_log( WYPE_LOG_WARNING, "Dashboard register: cannot resolve %s:%s: %s",
                   host, port, gai_strerror( gai ) );
        return -1;
    }

    int fd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
    if( fd < 0 )
    {
        freeaddrinfo( res );
        return -1;
    }

    struct timeval tv = { .tv_sec = REGISTER_TIMEOUT_SEC, .tv_usec = 0 };
    setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) );
    setsockopt( fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof( tv ) );

    if( connect( fd, res->ai_addr, res->ai_addrlen ) < 0 )
    {
        close( fd );
        freeaddrinfo( res );
        return -1;
    }
    freeaddrinfo( res );
    return fd;
}

/* ------------------------------------------------------------------ */
/*  Build JSON body                                                    */
/* ------------------------------------------------------------------ */

static char* build_register_json( void )
{
    extern int global_wipe_status;

    char hostname[256] = "";
    gethostname( hostname, sizeof( hostname ) - 1 );
    hostname[sizeof( hostname ) - 1] = '\0';

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "hostname", hostname );
    cJSON_AddStringToObject( root, "version", version_string );
    cJSON_AddNumberToObject( root, "port", reg_local_api_port );
    cJSON_AddBoolToObject( root, "wipe_active", global_wipe_status == 1 );

    char* json = cJSON_PrintUnformatted( root );
    cJSON_Delete( root );
    return json;  /* caller must free() */
}

/* ------------------------------------------------------------------ */
/*  Send one HTTP POST and read response status line                   */
/* ------------------------------------------------------------------ */

static int do_register( void )
{
    char* body = build_register_json();
    if( !body )
        return -1;

    int fd = tcp_connect( reg_host, reg_port_str );
    if( fd < 0 )
    {
        free( body );
        return -1;
    }

    /* Build raw HTTP/1.0 request */
    char request[HTTP_BUF_SIZE];
    int body_len = (int) strlen( body );
    int n = snprintf( request, sizeof( request ),
                      "POST /api/v1/register HTTP/1.0\r\n"
                      "Host: %s:%s\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: %d\r\n"
                      "X-API-Key: %s\r\n"
                      "Connection: close\r\n"
                      "\r\n"
                      "%s",
                      reg_host, reg_port_str, body_len, reg_api_key, body );
    free( body );

    if( n < 0 || n >= (int) sizeof( request ) )
    {
        close( fd );
        return -1;
    }

    if( send( fd, request, (size_t) n, 0 ) < 0 )
    {
        close( fd );
        return -1;
    }

    /* Read response (we only care about the status line) */
    char resp[HTTP_BUF_SIZE];
    ssize_t r = recv( fd, resp, sizeof( resp ) - 1, 0 );
    close( fd );

    if( r <= 0 )
        return -1;

    resp[r] = '\0';

    /* Check for 2xx status */
    int http_status = 0;
    if( sscanf( resp, "HTTP/%*d.%*d %d", &http_status ) == 1 )
    {
        if( http_status >= 200 && http_status < 300 )
            return 0;
    }

    wype_log( WYPE_LOG_WARNING, "Dashboard register: HTTP %d from %s:%s",
               http_status, reg_host, reg_port_str );
    return -1;
}

/* ------------------------------------------------------------------ */
/*  Background thread                                                  */
/* ------------------------------------------------------------------ */

static void* register_thread_func( void* arg )
{
    (void) arg;

    wype_log( WYPE_LOG_NOTICE, "Dashboard register: thread started, target %s:%s",
               reg_host, reg_port_str );

    while( !reg_stop )
    {
        if( do_register() == 0 )
        {
            wype_log( WYPE_LOG_INFO, "Dashboard register: heartbeat sent to %s:%s",
                       reg_host, reg_port_str );
        }

        /* Sleep in 1-second increments so we can react to stop quickly */
        for( int i = 0; i < REGISTER_INTERVAL_SEC && !reg_stop; i++ )
            sleep( 1 );
    }

    wype_log( WYPE_LOG_NOTICE, "Dashboard register: thread stopped" );
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

int wype_dashboard_register_start( const char* dashboard_url,
                                    const char* api_password,
                                    int api_port )
{
    if( reg_running )
    {
        wype_log( WYPE_LOG_WARNING, "Dashboard register: already running" );
        return 0;
    }

    /* Empty URL means disabled */
    if( dashboard_url == NULL || dashboard_url[0] == '\0' )
    {
        wype_log( WYPE_LOG_INFO, "Dashboard register: Dashboard_URL not set — disabled" );
        return -1;
    }

    /* Parse "host:port" */
    strncpy( reg_host, dashboard_url, sizeof( reg_host ) - 1 );
    reg_host[sizeof( reg_host ) - 1] = '\0';

    char* colon = strrchr( reg_host, ':' );
    if( colon && colon != reg_host )
    {
        *colon = '\0';
        strncpy( reg_port_str, colon + 1, sizeof( reg_port_str ) - 1 );
        reg_port_str[sizeof( reg_port_str ) - 1] = '\0';
    }
    else
    {
        strncpy( reg_port_str, "8080", sizeof( reg_port_str ) );
    }

    if( api_password )
    {
        strncpy( reg_api_key, api_password, sizeof( reg_api_key ) - 1 );
        reg_api_key[sizeof( reg_api_key ) - 1] = '\0';
    }

    reg_local_api_port = api_port > 0 ? api_port : 5000;
    reg_stop = 0;

    if( pthread_create( &reg_thread, NULL, register_thread_func, NULL ) != 0 )
    {
        wype_log( WYPE_LOG_ERROR, "Dashboard register: failed to create thread" );
        return -1;
    }

    reg_running = 1;
    return 0;
}

void wype_dashboard_register_stop( void )
{
    if( !reg_running )
        return;

    reg_stop = 1;
    pthread_join( reg_thread, NULL );
    reg_running = 0;
}

#else /* !HAVE_API_SERVER — stubs */

int wype_dashboard_register_start( const char* dashboard_url,
                                    const char* api_password,
                                    int api_port )
{
    (void) dashboard_url;
    (void) api_password;
    (void) api_port;
    return -1;
}

void wype_dashboard_register_stop( void )
{
}

#endif /* HAVE_API_SERVER */
