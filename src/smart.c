/*
 *  smart.c: Read SMART attributes via smartctl -A / smartctl -H.
 *
 *  Calls smartctl as an external process and parses the output.
 *  Populates the smart_* fields in the device context.
 *  Designed to be called once during device enumeration.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "smart.h"
#include "logging.h"

/* Try multiple paths in case PATH is not set up (Debian su vs su -) */
static const char* smartctl_paths[] = {
    "smartctl",
    "/usr/sbin/smartctl",
    "/usr/bin/smartctl",
    "/sbin/smartctl",
    NULL
};

static const char* find_smartctl( void )
{
    char cmd[128];
    for( int i = 0; smartctl_paths[i]; i++ )
    {
        snprintf( cmd, sizeof( cmd ), "which %s > /dev/null 2>&1", smartctl_paths[i] );
        if( system( cmd ) == 0 )
            return smartctl_paths[i];
    }
    return NULL;
}

/* Parse a single SMART attribute line (ATA format):
 *   ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
 *     9 Power_On_Hours          0x0032   099   099   000    Old_age   Always       -       1234
 */
static int parse_ata_attr_line( const char* line, int* id_out, long long* raw_out )
{
    /* Skip leading whitespace */
    while( *line == ' ' || *line == '\t' )
        line++;

    if( !isdigit( (unsigned char) *line ) )
        return -1;

    int id = atoi( line );

    /* Find the raw value: it's the last whitespace-delimited token */
    const char* last_token = NULL;
    const char* p = line;
    while( *p )
    {
        while( *p == ' ' || *p == '\t' )
            p++;
        if( *p )
        {
            last_token = p;
            while( *p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
                p++;
        }
    }

    if( !last_token )
        return -1;

    *id_out = id;
    *raw_out = atoll( last_token );
    return 0;
}

/* Parse NVMe "Percentage Used:" or similar lines */
static int parse_nvme_line( const char* line, const char* key, long long* val_out )
{
    const char* p = strstr( line, key );
    if( !p )
        return -1;
    p += strlen( key );
    while( *p == ' ' || *p == '\t' || *p == ':' )
        p++;

    /* Find first digit (skip any units like '%') */
    char buf[64] = "";
    int i = 0;
    while( *p && i < 63 )
    {
        if( isdigit( (unsigned char) *p ) || *p == '-' )
            buf[i++] = *p;
        else if( i > 0 )
            break; /* stop at first non-digit after digits */
        p++;
    }
    buf[i] = '\0';
    if( i == 0 )
        return -1;

    *val_out = atoll( buf );
    return 0;
}

void wype_read_smart( wype_context_t* c )
{
    /* Initialise all fields */
    c->smart_available = 0;
    c->smart_power_on_hours = -1;
    c->smart_power_cycle_count = -1;
    c->smart_start_stop_count = -1;
    c->smart_reallocated_sectors = -1;
    c->smart_pending_sectors = -1;
    c->smart_offline_uncorrectable = -1;
    c->smart_udma_crc_errors = -1;
    c->smart_spin_up_time_ms = -1;
    c->smart_overall_health = -1;
    c->smart_firmware[0] = '\0';
    c->smart_percentage_used = -1;

    if( !c->device_name || c->device_name[0] == '\0' )
        return;

    const char* smartctl = find_smartctl();
    if( !smartctl )
    {
        wype_log( WYPE_LOG_INFO, "smartctl not found — SMART data unavailable for %s", c->device_name );
        return;
    }

    /* Run smartctl -iHA <device> to get health + attributes + info in one call */
    char cmd[512];
    snprintf( cmd, sizeof( cmd ), "%s -iHA %s 2>/dev/null", smartctl, c->device_name );

    FILE* fp = popen( cmd, "r" );
    if( !fp )
    {
        wype_log( WYPE_LOG_WARNING, "SMART: popen failed for %s", c->device_name );
        return;
    }

    char line[512];
    int in_attrs = 0;
    int got_anything = 0;

    while( fgets( line, sizeof( line ), fp ) )
    {
        /* Health check result */
        if( strstr( line, "SMART overall-health" ) || strstr( line, "SMART Health Status" ) )
        {
            if( strstr( line, "PASSED" ) || strstr( line, "OK" ) )
                c->smart_overall_health = 1;
            else
                c->smart_overall_health = 0;
            got_anything = 1;
        }

        /* Firmware version (from -i output) */
        if( strstr( line, "Firmware Version:" ) )
        {
            const char* p = strchr( line, ':' );
            if( p )
            {
                p++;
                while( *p == ' ' || *p == '\t' )
                    p++;
                /* Copy trimmed value */
                int i = 0;
                while( *p && *p != '\n' && *p != '\r' && i < (int) sizeof( c->smart_firmware ) - 1 )
                    c->smart_firmware[i++] = *p++;
                /* Trim trailing spaces */
                while( i > 0 && c->smart_firmware[i - 1] == ' ' )
                    i--;
                c->smart_firmware[i] = '\0';
                got_anything = 1;
            }
        }

        /* Detect start of ATA attribute table */
        if( strstr( line, "ID#" ) && strstr( line, "ATTRIBUTE_NAME" ) )
        {
            in_attrs = 1;
            continue;
        }

        /* ATA attributes */
        if( in_attrs )
        {
            /* Empty line or non-numeric start ends the table */
            const char* p = line;
            while( *p == ' ' || *p == '\t' )
                p++;
            if( !isdigit( (unsigned char) *p ) )
            {
                in_attrs = 0;
            }
            else
            {
                int id = 0;
                long long raw = 0;
                if( parse_ata_attr_line( line, &id, &raw ) == 0 )
                {
                    got_anything = 1;
                    switch( id )
                    {
                        case 3:
                            c->smart_spin_up_time_ms = (int) raw;
                            break;
                        case 4:
                            c->smart_start_stop_count = (int) raw;
                            break;
                        case 5:
                            c->smart_reallocated_sectors = (int) raw;
                            break;
                        case 9:
                            c->smart_power_on_hours = (int) raw;
                            break;
                        case 12:
                            c->smart_power_cycle_count = (int) raw;
                            break;
                        case 197:
                            c->smart_pending_sectors = (int) raw;
                            break;
                        case 198:
                            c->smart_offline_uncorrectable = (int) raw;
                            break;
                        case 199:
                            c->smart_udma_crc_errors = (int) raw;
                            break;
                    }
                }
            }
        }

        /* NVMe attributes (different output format) */
        {
            long long val;
            if( parse_nvme_line( line, "Power On Hours", &val ) == 0 )
            {
                c->smart_power_on_hours = (int) val;
                got_anything = 1;
            }
            if( parse_nvme_line( line, "Power Cycles", &val ) == 0 )
            {
                c->smart_power_cycle_count = (int) val;
                got_anything = 1;
            }
            if( parse_nvme_line( line, "Percentage Used", &val ) == 0 )
            {
                c->smart_percentage_used = (int) val;
                got_anything = 1;
            }
            if( parse_nvme_line( line, "Unsafe Shutdowns", &val ) == 0 )
            {
                c->smart_start_stop_count = (int) val;
                got_anything = 1;
            }
        }
    }

    pclose( fp );

    if( got_anything )
    {
        c->smart_available = 1;
        wype_log( WYPE_LOG_INFO, "SMART: %s — health=%d poh=%d cycles=%d realloc=%d",
                   c->device_name,
                   c->smart_overall_health,
                   c->smart_power_on_hours,
                   c->smart_power_cycle_count,
                   c->smart_reallocated_sectors );
    }
}
