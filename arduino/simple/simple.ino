#include <Arduino.h>
#include "atomicx.h"

constexpr int pin = 13; // Default LED pin on Arduino Nano

static atomicx::Context localCtx;

void fastBlink(size_t n) {
    for (size_t i = 0; i < n; ++i) {
        digitalWrite(pin, HIGH);
        delay(1); // 100 milliseconds on
        digitalWrite(pin, LOW);
    }
}
class testThread : public atomicx::Thread
{
public:
    testThread(size_t start) : Thread(vmemory, localCtx), start(start)
    {
        id = counterId++;
        Serial.println("Thread " + String(id) + " created");
    }

    ~testThread()
    {
    }
protected:
    bool run() override
    {
        Serial.println("Thread is running");
        size_t nCount=start;

        while(localCtx.yield())
        {
            fastBlink(id+1);
            Serial.println(String(id) + ": Thread is running " + String(nCount++));
            delay(2); // Sleep for 1/2 second (by instance 1,000,000 microseconds)
        }

        Serial.println("Thread " + String(id) + " stopped");
        return true;
    }

    bool StackOverflow() override
    {
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

    for(atomicx::Thread* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        Serial.println("Thread " + String((size_t)thread) + " is in the thread pool");
    }
    
    localCtx.start();

}