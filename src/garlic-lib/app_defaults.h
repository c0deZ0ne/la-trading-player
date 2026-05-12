#ifndef APP_DEFAULTS_H
#define APP_DEFAULTS_H

/**
 * @brief Global Production Defaults for La Player
 * 
 * This file is the single source of truth for hardcoded fallbacks.
 * Use these constants instead of hardcoding IPs or ports in individual files.
 */

namespace AppDefaults {
    // Production Server Identity
    #define PROD_SERVER_IP "192.168.0.5"
    
    // Default Ports
    #define PROD_MANAGEMENT_PORT "3000"
    #define PROD_API_PORT "3001"
    #define PROD_VPN_PORT "51820"

    // Default URLs and Endpoints
    #define PROD_MANAGEMENT_URL "http://" PROD_SERVER_IP ":" PROD_MANAGEMENT_PORT
    #define PROD_VPN_ENDPOINT    PROD_SERVER_IP ":" PROD_VPN_PORT
}

#endif // APP_DEFAULTS_H
