#include <Arduino.h>
#include "atomicx/atomicx.h"
#include "atomicx/atomicx.cpp"

#ifdef __AVR__  // Only compile this section if we're on an AVR device

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

size_t freeMemory() {
  int free_memory;
  if ((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);
  return free_memory;
}

#endif

constexpr int pin = 13; // Default LED pin on Arduino Nano

ax::time ax::getTick(void)
{
    return millis();
}

void ax::sleepTicks(ax::time nSleep)
{
    delay(nSleep);
}

