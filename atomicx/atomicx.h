

/**
 * @file atomicx.h
 * @brief AtomicX Library Header
 *
 * This file contains the definitions and declarations for the AtomicX library.
 * AtomicX is designed to provide a lightweight threading and synchronization
 * mechanism suitable for small microprocessors, such as AVR, which have limited
 * C++ standard library support. Also support for Arduino and other embedded
 * systems and operating systems like FreeRTOS, windows, and Linux and macOS.
 *
 * @note The use of old-style includes and certain object handling methods is 
 * due to the need for compatibility with simple and small microprocessors. 
 * These microprocessors, like AVR, often do not support the full C++ Standard 
 * Library (STL). To address this, some STL functionalities have been ported 
 * to ensure synchronization support and compatibility with build systems that 
 * do not support the full STL.
 *
 * @version 2.0.0.proto
 * @date __TIMESTAMP__
 *
 * @section License
 * Licensed under the MIT License.
 *
 * @section Author
 * Gustavo Campos lgustavocampos@gmail.com
 */

#ifndef ATOMICX_H
#define ATOMICX_H

#include <setjmp.h>

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

/* Official version */
#define ATOMICX_VERSION "2.0.0.proto"
#define ATOMIC_VERSION_LABEL "AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__

// Helper macro to define the virtual memory
// parameters automatically
#define VMEM(vmemory) vmemory[0], sizeof(vmemory) / sizeof(size_t)

namespace ax {

#define ATIMICX_SYS_CHANEL 255

    class thread;

    using Time = uint32_t;
    using RefId = size_t;

    // Those functions MUST be 
    // implemented by the user
    Time getTick();
    void sleepTicks(Time nsleep);

    enum class STATE 
    {
        READY,
        RUNNING,
        SLEEPING,
        STOPPED,
        WAIT,
        TIMEDOUT,
        LOCKED,
        NOW
    };

    enum class TIME
    {
        UNDERFINED,
    };
    
    enum class Notify
    {
        ONE,
        ALL
    };

    struct Tag
    {
        size_t param;
        size_t value;
    };

    /**
     * @brief Timeout Check object
     */

    class Timeout
    {
        public:

            Timeout ();

            Timeout (TIME type);
            
            Timeout (Time timeoutValue);

            void set(Time timeoutValue);

            bool isTimedOut();

            Time getRemaining();

            Time getDurationSince(Time startTime);

            Time operator ()();
            
        private:
            Time m_timeoutValue = 0;
    };
    
    class Context
    {
    public:
        friend class thread;

        void setNextActiveThread();

        void sleepUntilTick(Time nSleep);
        
        int start();

        void AddThread(thread* thread);

        void RemoveThread(thread* thread);

        thread& operator()();

    private:
        friend class thread;

        bool CheckAllThreadsStopped();

        thread* begin{nullptr};
        thread* last{nullptr};
        size_t threadCount{0};

        bool m_running{false};
        thread *m_activeThread;
        thread *m_nextThread;

        Time m_switchTime{0};
    };

    extern Context ctx;

    // ----------------------------------------------
    // Thread class
    // ----------------------------------------------
    class thread
    {
    private:
        friend class Context;

        jmp_buf userRegs;
        jmp_buf kernelRegs;

        struct Metrics
        {
            STATE state{STATE::STOPPED};

            uint16_t poolId{0};

            Time nice{0};
            Time nextExecTime{0};

            size_t maxStackSize{0};
            size_t stackSize{0};

            Tag tag{0, 0};
            RefId* refId{nullptr};
            uint8_t waitChannel{0};
            Timeout waitTimeout{0};
        } metrics;

        struct
        {
            uint8_t *kernelPointer{nullptr};
            uint8_t *userPointer{nullptr};
            size_t *vmemory;
        } stack;

        // Node control
        thread* next{nullptr};
        thread* prev{nullptr};

    protected:
        bool virtual run() = 0;

        bool virtual StackOverflow() = 0;
 
        size_t doNotification(RefId& refId, Notify type, Tag& tag, uint8_t channel);

    public:
        bool yield(Timeout arg = 0, STATE cmd = STATE::SLEEPING);
        bool yieldUntil(Time timeout, size_t arg = 0, STATE cmd = STATE::SLEEPING);

        void defaultInit(size_t* vmemory, size_t maxSize);

        thread(size_t& vmemory, size_t stackSize);

        virtual ~thread();

        thread* operator++(int);
 
        thread* begin();
 
        // Get metrics data
        const Metrics& getMetrics();
 
        // Set Metrics data
        bool setNice(Time nice);

        // Wait and notify
        bool wait(RefId& refId, Tag& tag, Timeout timeout, uint8_t channel);

        size_t notify(RefId& refId, Notify type, Tag tag, Timeout timeout, uint8_t channel);
    };
}; // namespace ax


#endif // ATOMICX_H
