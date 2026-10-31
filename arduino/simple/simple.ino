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

#ifdef __AVR__
// A thread's slice is about 30 bytes on AVR (-fstack-usage); 8 threads and their stacks must fit the Nano's 2 KB
constexpr size_t k_stack_size = 80;
#else
constexpr size_t k_stack_size = 500;
#endif

ax::time ax::getTick(void)
{
    return millis();
}

void ax::sleepTicks(ax::time nSleep)
{
    delay(nSleep);
}

ax::thread_context myThreadContext;

size_t dumb()
{
    return 10;
}

class MyThreadItem : public ax::thread_item
{
    public:
        MyThreadItem(ax::thread_context& parent, size_t stackSize, ax::time nice, size_t counter) : thread_item(parent, stackSize), _counter(counter) { setNice(nice); }

        void run() override
        {
            while (true)
            {
                Serial.print(">> Thread item at address: 0x"); Serial.print((uintptr_t)this, HEX); 
                Serial.print(F(" with counter: ")); Serial.println(_counter++); 
                // //Serial.print(" Nice: "); Serial.println(getNice()); 
                // Serial.print(F(" Free memory: ")); Serial.println(freeMemory());
                yield();
            }
        }

        void StackOverflow() override
        {
            Serial.print(F("Thread item at address: 0x"));
            Serial.print((uintptr_t)this, HEX);
            Serial.print(F(" Thread item encountered a stack overflow! of: "));
            Serial.println(String(getUsedStackSize()));
        }
    private:
        size_t _counter;
};

class MySecThreadItem : public ax::thread_item
{
    public:
        MySecThreadItem(ax::thread_context& parent, size_t stackSize, ax::time nice, size_t counter) : thread_item(parent, stackSize), _counter(counter) { setNice(nice); }


        void run() override
        {
            while (true)
            {
                dump();
            }
        }

        void StackOverflow() override
        {
            Serial.print(F("Thread item at address: 0x"));
            Serial.print((uintptr_t)this, HEX);
            Serial.print(F("Thread item encountered a stack overflow! of: "));
            Serial.println(String(getUsedStackSize()));
        }
    private:

        void dump()
        {
            Serial.print(F("** Thread item at address: 0x"));
            Serial.print((uintptr_t)this, HEX);
            Serial.print(F(" with counter: "));
            Serial.print(_counter++);
            ///Serial.print(F(" Nice: "));
            //Serial.println(getNice());
            //Serial.print(F(" Free memory: "));
            //Serial.println(freeMemory());
            
            size_t freeMem = freeMemory();
            Serial.print(F(" Free memory: "));
            Serial.println((size_t)freeMem);
            yield();
        }

        size_t _counter;
};


void setup()
{
    Serial.begin(115200);
    while (!Serial) {} // wait for the serial port on boards with native USB

    pinMode(pin, OUTPUT);

    MyThreadItem myThreadItem_1(myThreadContext, k_stack_size, 500, 1000);
    MyThreadItem myThreadItem_2(myThreadContext, k_stack_size, 1000, 2000);
    MyThreadItem myThreadItem_3(myThreadContext, k_stack_size, 5000, 3000);

    MySecThreadItem mySecThreadItem_1(myThreadContext, k_stack_size, 500, 5000);
    MySecThreadItem mySecThreadItem_2(myThreadContext, k_stack_size, 1000, 6000);
    MySecThreadItem mySecThreadItem_3(myThreadContext, k_stack_size, 5000, 7000);


    for(ax::auto_obj_list::item* current = myThreadContext.getHeadItem(); current; current = current->getNextItem(current))
    {
        Serial.print(F("Thread item at address: 0x"));
        Serial.println((uintptr_t)current, HEX);
    }

#ifdef __AVR__
    Serial.print(F("Free memory: "));
    Serial.println(freeMemory());
#endif

    Serial.println(F("------------------------------"));
    Serial.println(F("Starting thread context..."));
    Serial.println(F("------------------------------"));

    myThreadContext.start();

    Serial.println(F("Main thread finished."));
}

void loop()
{
    // start() only returns when the threads stop; blink to show it
    digitalWrite(pin, !digitalRead(pin));
    delay(500);
}
