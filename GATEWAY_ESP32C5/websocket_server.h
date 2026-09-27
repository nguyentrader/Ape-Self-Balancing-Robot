#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>


// ============================================================
// Initialize WebSocket server
// ============================================================

void websocketInit();


// ============================================================
// Process WebSocket events
// ============================================================

void websocketLoop();


// ============================================================
// Send message to all connected clients
// ============================================================

void websocketBroadcast(const String& message);


// ============================================================
// Send message to specific client
// ============================================================

void websocketSend(
    uint8_t clientNum,
    const String& message
);


// ============================================================
// Check whether a client is connected
// ============================================================

bool websocketHasClient();

#endif