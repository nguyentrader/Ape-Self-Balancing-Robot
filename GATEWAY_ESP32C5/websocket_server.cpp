#include "websocket_server.h"

#include "config.h"
#include "debug_logger.h"
#include "message_handler.h"

#include <WebSocketsServer.h>


// ============================================================
// WebSocket server
// ============================================================

static WebSocketsServer webSocket(WEBSOCKET_PORT);


// ============================================================
// Connection state
// ============================================================

static bool clientConnected = false;


// ============================================================
// Internal event handler
// ============================================================

static void webSocketEvent(
    uint8_t clientNum,
    WStype_t type,
    uint8_t* payload,
    size_t length
);


// ============================================================
// Initialize WebSocket server
// ============================================================

void websocketInit()
{
    webSocket.begin();

    webSocket.onEvent(webSocketEvent);

    logInfo(
        "WebSocket server started on port " +
        String(WEBSOCKET_PORT)
    );
}


// ============================================================
// WebSocket loop
// ============================================================

void websocketLoop()
{
    webSocket.loop();
}


// ============================================================
// Broadcast message to all connected clients
// ============================================================

void websocketBroadcast(const String& message)
{
    if (!clientConnected)
    {
        return;
    }

    // WebSockets library requires String&
    String payload = message;

    webSocket.broadcastTXT(payload);

    logTxWeb(message);
}


// ============================================================
// Send message to specific client
// ============================================================

void websocketSend(
    uint8_t clientNum,
    const String& message
)
{
    // WebSockets library requires String&
    String payload = message;

    webSocket.sendTXT(
        clientNum,
        payload
    );

    logTxWeb(message);
}


// ============================================================
// Check whether a WebSocket client is connected
// ============================================================

bool websocketHasClient()
{
    return clientConnected;
}


// ============================================================
// WebSocket event handler
// ============================================================

static void webSocketEvent(
    uint8_t clientNum,
    WStype_t type,
    uint8_t* payload,
    size_t length
)
{
    switch (type)
    {
        // ----------------------------------------------------
        // Client connected
        // ----------------------------------------------------

        case WStype_CONNECTED:
        {
            clientConnected = true;

            logInfo(
                "WebSocket client connected: " +
                String(clientNum)
            );

            break;
        }


        // ----------------------------------------------------
        // Client disconnected
        // ----------------------------------------------------

        case WStype_DISCONNECTED:
        {
            clientConnected = false;

            logWarn(
                "WebSocket client disconnected: " +
                String(clientNum)
            );

            break;
        }


        // ----------------------------------------------------
        // Text message received
        // ----------------------------------------------------

        case WStype_TEXT:
        {
            String message;

            message.reserve(length);

            for (size_t i = 0; i < length; i++)
            {
                message += (char)payload[i];
            }

            logRxWeb(message);

            // Pass message to message handler
            handleWebMessage(message);

            break;
        }


        // ----------------------------------------------------
        // WebSocket error
        // ----------------------------------------------------

        case WStype_ERROR:
        {
            logError("WebSocket error");

            break;
        }


        // ----------------------------------------------------
        // Other events
        // ----------------------------------------------------

        default:
        {
            break;
        }
    }
}