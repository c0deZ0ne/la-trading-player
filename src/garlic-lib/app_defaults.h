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
    #define PROD_SERVER_HOST "api.la-trading-cms.co.uk"
    
    // Default Ports
    #define PROD_MANAGEMENT_PORT "3000"
    #define PROD_API_PORT "3001"
    #define PROD_VPN_PORT "51820"

    // Default URLs and Endpoints
    #define PROD_MANAGEMENT_URL "https://" PROD_SERVER_HOST
    #define PROD_VPN_ENDPOINT    PROD_SERVER_HOST ":" PROD_VPN_PORT
}

#endif // APP_DEFAULTS_H
