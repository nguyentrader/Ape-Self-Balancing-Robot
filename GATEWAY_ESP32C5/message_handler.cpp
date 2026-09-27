#include "message_handler.h"

#include "debug_logger.h"
#include "stm32_uart.h"
#include "websocket_server.h"

#include <ArduinoJson.h>


// ============================================================
// Internal handler declarations
// ============================================================

static void handleWebCommand(JsonDocument& doc);
static void handleWebPid(JsonDocument& doc);
static void handleInternalPing(JsonDocument& doc);

static void handleStm32Telemetry(String& message);
static void handleStm32Ack(String& message);
static void handleStm32Status(String& message);
static void handleStm32RobotState(String& message);
static void handleStm32Error(String& message);
static void handleInternalPong(String& message);


// ============================================================
// Initialize
// ============================================================

void messageHandlerInit()
{
    logInfo("Message handler initialized");
}


// ============================================================
// Handle message from WebSocket
// ============================================================

void handleWebMessage(String& message)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        logError("Invalid JSON from Web");

        return;
    }


    // --------------------------------------------------------
    // Get message type
    // --------------------------------------------------------

    const char* type = doc["type"];

    if (type == nullptr)
    {
        logWarn("Web message missing type");

        return;
    }

    String messageType = type;


    // --------------------------------------------------------
    // Route message
    // --------------------------------------------------------

    if (messageType == "command")
    {
        handleWebCommand(doc);
    }
    else if (messageType == "pid")
    {
        handleWebPid(doc);
    }
    else if (messageType == "internal_ping")
    {
        handleInternalPing(doc);
    }
    else
    {
        logWarn(
            "Unknown Web message type: " +
            messageType
        );
    }
}


// ============================================================
// Handle message from STM32
// ============================================================

void handleStm32Message(String& message)
{
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        logError("Invalid JSON from STM32");

        return;
    }


    // --------------------------------------------------------
    // Get message type
    // --------------------------------------------------------

    const char* type = doc["type"];

    if (type == nullptr)
    {
        logWarn("STM32 message missing type");

        return;
    }

    String messageType = type;


    // --------------------------------------------------------
    // Route message
    // --------------------------------------------------------

    if (messageType == "telemetry")
    {
        handleStm32Telemetry(message);
    }
    else if (messageType == "ack")
    {
        handleStm32Ack(message);
    }
    else if (messageType == "status")
    {
        handleStm32Status(message);
    }
    else if (messageType == "robot_state")
    {
        handleStm32RobotState(message);
    }
    else if (messageType == "error")
    {
        handleStm32Error(message);
    }
    else if (messageType == "internal_pong")
    {
        handleInternalPong(message);
    }
    else
    {
        logWarn(
            "Unknown STM32 message type: " +
            messageType
        );
    }
}


// ============================================================
// WEB → STM32
// Command
// ============================================================

static void handleWebCommand(JsonDocument& doc)
{
    const char* command = doc["command"];

    if (command == nullptr)
    {
        logWarn("Command message missing command");

        return;
    }


    // Log only the command name here.
    // The complete Web packet has already been logged
    // by websocket_server.

    logInfo(
        "Processing command: " +
        String(command)
    );


    // --------------------------------------------------------
    // Forward original JSON to STM32
    // --------------------------------------------------------

    String output;

    serializeJson(doc, output);

    if (!stm32UartSend(output))
    {
        logError("Failed to send command to STM32");
    }
}


// ============================================================
// WEB → STM32
// PID
// ============================================================

static void handleWebPid(JsonDocument& doc)
{
    // Basic validation
    if (!doc["balance"].is<JsonObject>() &&
        !doc["speed"].is<JsonObject>() &&
        !doc["yaw"].is<JsonObject>())
    {
        logWarn("PID message contains no valid PID section");

        return;
    }


    logInfo("Processing PID update");


    // --------------------------------------------------------
    // Forward original JSON to STM32
    // --------------------------------------------------------

    String output;

    serializeJson(doc, output);

    if (!stm32UartSend(output))
    {
        logError("Failed to send PID update to STM32");
    }
}


// ============================================================
// WEB → STM32
// Internal ping
// ============================================================

static void handleInternalPing(JsonDocument& doc)
{
    /*
     * The timestamp / ID is generated by the Web UI.
     *
     * ESP32 does not modify the packet.
     * It simply forwards it to STM32.
     */

    String output;

    serializeJson(doc, output);

    if (!stm32UartSend(output))
    {
        logError("Failed to send internal ping to STM32");
    }
}


// ============================================================
// STM32 → WEB
// Telemetry
// ============================================================

static void handleStm32Telemetry(String& message)
{
    websocketBroadcast(message);
}


// ============================================================
// STM32 → WEB
// ACK
// ============================================================

static void handleStm32Ack(String& message)
{
    websocketBroadcast(message);
}


// ============================================================
// STM32 → WEB
// Status
// ============================================================

static void handleStm32Status(String& message)
{
    websocketBroadcast(message);
}


// ============================================================
// STM32 → WEB
// Robot state
// ============================================================

static void handleStm32RobotState(String& message)
{
    websocketBroadcast(message);
}


// ============================================================
// STM32 → WEB
// Error
// ============================================================

static void handleStm32Error(String& message)
{
    logError("STM32 reported an error");

    websocketBroadcast(message);
}


// ============================================================
// STM32 → WEB
// Internal pong
// ============================================================

static void handleInternalPong(String& message)
{
    websocketBroadcast(message);
}