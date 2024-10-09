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

ax::Time ax::getTick(void)
{
    return millis();
}

void ax::sleepTicks(ax::Time nSleep)
{
    delay(nSleep);
}

char text[100];

ax::RefId refId;

class testThread : public ax::thread
{
public:

    void fastBlink(size_t n, size_t &nCount, ax::Tag& tag) {
        for (size_t i = 0; i < n; ++i) {
            digitalWrite(pin, HIGH);
            delay(1); // 100 milliseconds on
            digitalWrite(pin, LOW);
        }

        auto& metrics = getMetrics();

        snprintf(text, sizeof(text), "%d: Count:%d, Tag:{%d/%d} size:%d/%d bytes, CTX:%d, Thread:%d Free:%u", id, nCount++, tag.param, tag.value, metrics.stackSize, metrics.maxStackSize, sizeof(ax::ctx), sizeof(ax::thread), freeMemory());
        Serial.println(text); Serial.flush();
    }

    testThread(size_t start) : thread(VMEM(vmemory)), start(start)
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
        ax::Tag tag;

        while(yield(50))
        {
            fastBlink(id+1, nCount, tag);
            wait(refId, tag, ax::TIME::UNDERFINED, 0);
        }

        Serial.println("Thread " + String(id) + " stopped");
        return true;
    }

    bool StackOverflow() override
    {
        auto& metrics = getMetrics();   
        Serial.println("Thread " + String(id) + " stack overflow: " + String(metrics.stackSize) + "/" + String(metrics.maxStackSize) + " bytes"); 
        return false;
    }

private:
    size_t vmemory[50];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

class testProducerThread : public ax::thread
{
public:

    void fastBlink(size_t n, size_t &nCount) {
        for (size_t i = 0; i < n; ++i) {
            digitalWrite(pin, HIGH);
            delay(1); // 100 milliseconds on
            digitalWrite(pin, LOW);
        }

        auto& metrics = getMetrics();

        snprintf(text, sizeof(text), "PRODUCER: %d: Count:%d, size:%d/%d bytes, CTX:%d, Thread:%d Free:%u", id, nCount++, metrics.stackSize, metrics.maxStackSize, sizeof(ax::ctx), sizeof(ax::thread), freeMemory());
        Serial.println(text); Serial.flush();
    }

    testProducerThread(size_t start) : thread(VMEM(vmemory)), start(start)
    {
        id = counterId++;
        Serial.println("Thread " + String(id) + " created");
        setNice(1000);
    }

    ~testProducerThread()
    {
    }

protected:
    bool run() final
    {
        //Serial.println("Thread is running");
        size_t nCount=start;
        setNice(1000 * (id+1));

        while(true)
        {
            fastBlink(id+1, nCount);
            notify(refId, ax::Notify::ONE, {id, nCount}, 1000, 0);
            fastBlink(id+1, nCount);
        }

        Serial.println("PRODUCER " + String(id) + " stopped");
        return true;
    }

    bool StackOverflow() override
    {
        auto& metrics = getMetrics();   
        Serial.println("Thread " + String(id) + " stack overflow: " + String(metrics.stackSize) + "/" + String(metrics.maxStackSize) + " bytes"); 
        return false;
    }

private:
    size_t vmemory[40];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

// Initialize the static counter
size_t testThread::counterId = 0;
size_t testProducerThread::counterId = 0;

void setup() {
    // Initialize serial communication at 9600 bits per second:
    Serial.begin(115200);
}

void loop() {
    // Print "Hello, World!" to the serial monitor:
    Serial.println("Starting up Hello, World!");
    Serial.flush();

    testThread test1(0);
    testThread test2(2000);
    testThread test3(3000);
    testThread test4(4000);
    testThread test5(5000);
    
    testProducerThread test6(10000);

    for(ax::thread* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        Serial.println("Thread " + String((size_t)thread) + " is in the thread pool");
    }

    ax::ctx.start();
}