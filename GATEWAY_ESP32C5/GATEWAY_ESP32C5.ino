#include "config.h"

#include "debug_logger.h"
#include "wifi_manager.h"
#include "stm32_uart.h"
#include "websocket_server.h"
#include "message_handler.h"
#include "web_server.h"

void setup()
{
    debugInit();

    logInfo("System initialization started");

    messageHandlerInit();
    wifiInit();
    stm32UartInit();
    websocketInit();
    webServerInit();

    logInfo("System initialization complete");
}

void loop()
{
    websocketLoop();
    webServerLoop();
    stm32UartLoop();
}