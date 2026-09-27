#include "debug_logger.h"
#include "config.h"

void debugInit()
{
    Serial.begin(DEBUG_BAUDRATE);
    delay(100);

    Serial.println();
    Serial.println("=========================================");
    Serial.println("      SINGFAST OF UKRAINIAN APE LOG");
    Serial.println("=========================================");
}

void logInfo(const String& message)
{
    Serial.print("[INFO] ");
    Serial.println(message);
}

void logWarn(const String& message)
{
    Serial.print("[WARN] ");
    Serial.println(message);
}

void logError(const String& message)
{
    Serial.print("[ERROR] ");
    Serial.println(message);
}

void logRxWeb(const String& message)
{
    Serial.print("[RX WEB] ");
    Serial.println(message);
}

void logTxWeb(const String& message)
{
    Serial.print("[TX WEB] ");
    Serial.println(message);
}

void logTxStm32(const String& message)
{
    Serial.print("[TX STM32] ");
    Serial.println(message);
}

void logRxStm32(const String& message)
{
    Serial.print("[RX STM32] ");
    Serial.println(message);
}