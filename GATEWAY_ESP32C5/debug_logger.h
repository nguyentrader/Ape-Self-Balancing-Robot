#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <Arduino.h>

void debugInit();

void logInfo(const String& message);
void logWarn(const String& message);
void logError(const String& message);

void logRxWeb(const String& message);
void logTxWeb(const String& message);

void logTxStm32(const String& message);
void logRxStm32(const String& message);

#endif