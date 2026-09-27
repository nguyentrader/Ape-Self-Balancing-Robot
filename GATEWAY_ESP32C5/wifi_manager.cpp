#include "wifi_manager.h"

#include "config.h"
#include "debug_logger.h"

#include <WiFi.h>


// ============================================================
// Initialize WiFi Access Point
// ============================================================

void wifiInit()
{
    logInfo("Starting WiFi Access Point...");


    // --------------------------------------------------------
    // Set WiFi mode to Access Point
    // --------------------------------------------------------

    WiFi.mode(WIFI_AP);


    // --------------------------------------------------------
    // Configure AP IP address
    // --------------------------------------------------------

    IPAddress localIP(
        WIFI_IP
    );

    IPAddress gateway(
        WIFI_GATEWAY
    );

    IPAddress subnet(
        WIFI_SUBNET
    );


    if (!WiFi.softAPConfig(
            localIP,
            gateway,
            subnet))
    {
        logError(
            "Failed to configure WiFi AP IP"
        );

        return;
    }


    // --------------------------------------------------------
    // Start Access Point
    // --------------------------------------------------------

    if (!WiFi.softAP(
            WIFI_SSID,
            WIFI_PASSWORD))
    {
        logError(
            "Failed to start WiFi AP"
        );

        return;
    }


    // --------------------------------------------------------
    // Report WiFi information
    // --------------------------------------------------------

    logInfo("WiFi AP started");

    logInfo(
        "SSID: " +
        String(WIFI_SSID)
    );

    logInfo(
        "IP: " +
        WiFi.softAPIP().toString()
    );
}