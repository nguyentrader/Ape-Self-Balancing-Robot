#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <Arduino.h>

void messageHandlerInit();

void handleWebMessage(String& message);
void handleStm32Message(String& message);

#endif