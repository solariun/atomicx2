#include <Arduino.h>
#include "atomicx.h"

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

static atomicx::Context localCtx;

atomicx_time atomicx::getTick(void)
{
    return millis();
}

void atomicx::sleepTicks(atomicx_time nSleep)
{
    delay(nSleep);
}

char text[100];

class testThread : public atomicx::Thread
{
public:

    void fastBlink(size_t n, size_t &nCount) {
        for (size_t i = 0; i < n; ++i) {
            digitalWrite(pin, HIGH);
            delay(1); // 100 milliseconds on
            digitalWrite(pin, LOW);
        }

        auto& metrix = getMetrix();

        snprintf(text, sizeof(text), "%d: Count:%d, size:%d/%d bytes, CTX:%d, Thread:%d Free:%u", id, nCount++, metrix.stackSize, metrix.maxStackSize, sizeof(localCtx), sizeof(atomicx::Thread), freeMemory());
        Serial.println(text); Serial.flush();
    }

    testThread(size_t start) : Thread(VMEM(vmemory), localCtx), start(start)
    {
        id = counterId++;
        Serial.println("Thread " + String(id) + " created");
        setNice(1000);
    }

    ~testThread()
    {
    }

protected:
    bool run() final
    {
        //Serial.println("Thread is running");
        size_t nCount=start;
        setNice(1000 * (id+1));
        
        while(localCtx.yield())
        {
            fastBlink(id+1, nCount);
            delay(1); // Sleep for 1/2 second (by instance 1,000,000 microseconds)
        }

        Serial.println("Thread " + String(id) + " stopped");
        return true;
    }

    bool StackOverflow() override
    {
        auto& metrix = getMetrix();   
        Serial.println("Thread " + String(id) + " stack overflow: " + String(metrix.stackSize) + "/" + String(metrix.maxStackSize) + " bytes"); 
        return false;
    }

private:
    size_t vmemory[30];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

// Initialize the static counter
size_t testThread::counterId = 0;

void setup() {
    // Initialize serial communication at 9600 bits per second:
    Serial.begin(115200);
}

void loop() {
    // Print "Hello, World!" to the serial monitor:
    Serial.println("Hello, World!");
    Serial.flush();

    testThread test1(0);
    testThread test2(2000);
    testThread test3(3000);
    testThread test4(4000);
    testThread test5(5000);
    testThread test6(6000);

    for(atomicx::Thread* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        Serial.println("Thread " + String((size_t)thread) + " is in the thread pool");
    }

    localCtx.start();
}