#include "stm32_uart.h"

#include "config.h"
#include "debug_logger.h"
#include "message_handler.h"

HardwareSerial STM32Serial(1);


// ============================================================
// RX buffer
// ============================================================

static String rxBuffer;


// ============================================================
// Initialize STM32 UART
// ============================================================

void stm32UartInit()
{
    STM32Serial.begin(
        STM32_UART_BAUDRATE,
        SERIAL_8N1,
        STM32_UART_RX_PIN,
        STM32_UART_TX_PIN
    );

    rxBuffer.reserve(512);

    logInfo("STM32 UART initialized");

    logInfo(
        "UART1 RX = GPIO" +
        String(STM32_UART_RX_PIN)
    );

    logInfo(
        "UART1 TX = GPIO" +
        String(STM32_UART_TX_PIN)
    );

    logInfo(
        "UART baudrate = " +
        String(STM32_UART_BAUDRATE)
    );
}


// ============================================================
// Check whether UART data is available
// ============================================================

bool stm32UartAvailable()
{
    return STM32Serial.available() > 0;
}


// ============================================================
// Send message to STM32
// ============================================================

bool stm32UartSend(const String& message)
{
    if (message.length() == 0)
    {
        return false;
    }


    // Send JSON followed by newline
    STM32Serial.println(message);


    // Debug log
    logTxStm32(message);


    return true;
}


// ============================================================
// Read one complete line from STM32
// ============================================================

String stm32UartReadLine()
{
    while (STM32Serial.available())
    {
        char c = STM32Serial.read();


        // ----------------------------------------------------
        // End of message
        // ----------------------------------------------------

        if (c == '\n')
        {
            String message = rxBuffer;

            rxBuffer = "";

            message.trim();


            if (message.length() > 0)
            {
                logRxStm32(message);
            }


            return message;
        }


        // ----------------------------------------------------
        // Ignore carriage return
        // ----------------------------------------------------

        if (c != '\r')
        {
            rxBuffer += c;
        }


        // ----------------------------------------------------
        // Protect against buffer overflow
        // ----------------------------------------------------

        if (rxBuffer.length() >= 512)
        {
            rxBuffer = "";

            logWarn(
                "STM32 UART RX buffer overflow"
            );

            return "";
        }
    }


    return "";
}


// ============================================================
// Process incoming STM32 UART data
// ============================================================

void stm32UartLoop()
{
    String message = stm32UartReadLine();


    if (message.length() == 0)
    {
        return;
    }


    // Pass complete message to message handler
    handleStm32Message(message);
}