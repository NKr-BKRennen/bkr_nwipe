/*
 *  dashboard_register.h: Self-registration with a central wype-dashboard.
 *
 *  A background thread periodically POSTs the node's identity and live
 *  status summary to the dashboard's /api/v1/register endpoint so that
 *  the dashboard no longer needs to scan subnets.
 *
 *  Requires cJSON (for JSON building) and POSIX sockets.  When
 *  HAVE_API_SERVER is not defined the public functions compile as stubs.
 */

#ifndef DASHBOARD_REGISTER_H_
#define DASHBOARD_REGISTER_H_

/**
 * Start the dashboard registration background thread.
 *
 * @param dashboard_url  "host:port" of the dashboard (e.g. "192.168.1.5:8080").
 *                       Empty or NULL disables registration.
 * @param api_password   X-API-Key sent with every request.
 * @param api_port       The local API server port advertised to the dashboard.
 * @return 0 on success, -1 when disabled or on error.
 */
int wype_dashboard_register_start( const char* dashboard_url,
                                    const char* api_password,
                                    int api_port );

/**
 * Signal the registration thread to stop and wait for it to exit.
 */
void wype_dashboard_register_stop( void );

#endif /* DASHBOARD_REGISTER_H_ */
