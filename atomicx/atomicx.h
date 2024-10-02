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

using atomicx_time = uint32_t;

// Helper macro to define the virtual memory
// parameters automatically
#define VMEM(vmemory) vmemory[0], sizeof(vmemory) / sizeof(size_t)

namespace atomicx {

    class thread;

    // Those functions MUST be 
    // implemented by the user
    atomicx_time getTick();
    void sleepTicks(atomicx_time nsleep);

    using RefId = size_t;

    enum class state 
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

    class Context
    {
    public:
        friend class thread;

        void setNextActiveThread();

        void sleepUntilTick(atomicx_time nSleep);
        
        int start();

        void AddThread(thread* thread);

        void RemoveThread(thread* thread);

    private:
        friend class thread;

        bool CheckAllThreadsStopped();

        thread* begin{nullptr};
        thread* last{nullptr};
        size_t threadCount{0};

        bool m_running{false};
        thread *m_activeThread;
        thread *m_nextThread;
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
            state state{state::STOPPED};

            uint16_t poolId{0};

            atomicx_time nice{0};
            atomicx_time nextExecTime{0};

            size_t maxStackSize{0};
            size_t stackSize{0};

            Tag tag{0, 0};
            RefId* refId{nullptr};
            uint8_t waitChannel{0};
            atomicx_time waitTimeout{0};
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
 
    public:
        bool yield(size_t arg = 0, state cmd = state::SLEEPING);
        bool yieldUntil(atomicx_time timeout, size_t arg = 0, state cmd = state::SLEEPING);

        void defaultInit(size_t* vmemory, size_t maxSize);

        thread(size_t& vmemory, size_t stackSize);

        virtual ~thread();

        bool yieldUntil(atomicx_time timeout);

        thread* operator++(int);
 
        thread* begin();
 
        // Get metrics data
        const Metrics& getMetrics();
 
        // Set Metrics data
        bool setNice(atomicx_time nice);

        // Wait and notify
        bool wait(RefId& refId, Tag& tag, atomicx_time timeout, uint8_t channel);

        size_t notify(RefId& refId, Notify type, Tag tag, atomicx_time timeout, uint8_t channel);
    };



}; // namespace atomicx


#endif // ATOMICX_H
