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

    class Thread;

    // Those functions MUST be 
    // implemented by the user
    atomicx_time getTick();
    void sleepTicks(atomicx_time nsleep);

    enum class state {
        READY,
        RUNNING,
        SLEEPING,
        STOPPED,
        WAIT,
        TIMEDOUT,
        LOCKED,
        NOW
    };

    struct Tag
    {
        size_t param;
        size_t value;
    };
    
    class Context
    {
    public:
        friend class Thread;

        void setNextActiveThread();

        void sleepUntilTick(atomicx_time nSleep);
        
        int start();

        bool yield(size_t arg = 0, state cmd = state::SLEEPING);
            
        void AddThread(Thread* thread);

        void RemoveThread(Thread* thread);

    private:
        friend class Thread;

        bool CheckAllThreadsStopped();

        Thread* begin{nullptr};
        Thread* last{nullptr};
        size_t threadCount{0};

        bool m_running{false};
        Thread *m_activeThread;
        Thread *m_nextThread;
    };

    //extern Context ctx;

    // ----------------------------------------------
    // Timeout class
    // ----------------------------------------------
    class Timeout
    {
        public:

            Timeout ();

            Timeout (atomicx_time nTimoutValue);

            void Set(atomicx_time nTimoutValue);

            bool IsTimedout();

            atomicx_time GetRemaining();

            atomicx_time GetDurationSince(atomicx_time startTime);

            atomicx_time GetTimeoutValue();
            
        private:
            atomicx_time m_timeoutValue = 0;
    };

    // ----------------------------------------------
    // Thread class
    // ----------------------------------------------
    class Thread
    {
    private:
        friend class Context;

        jmp_buf userRegs;
        jmp_buf kernelRegs;

        struct Metrix
        {
            state state{state::STOPPED};

            uint16_t poolId;

            atomicx_time nice;
            atomicx_time nextExecTime;

            size_t maxStackSize;
            size_t stackSize;

            Tag tag;
            void* refId;
            uint8_t waitChannel;
            atomicx_time waitTimeout;   
        } metrics;

        struct
        {
            uint8_t *kernelPointer{nullptr};
            uint8_t *userPointer{nullptr};
            size_t *vmemory;
        } stack;

        // Node control
        Thread* next{nullptr};
        Thread* prev{nullptr};

        Context& _ctx;

    protected:
        bool virtual run() = 0;

        bool virtual StackOverflow() = 0;

    public:
        void defaultInit(size_t* vmemory, size_t maxSize);

        Thread(size_t& vmemory, size_t stackSize, Context& ictx);

        virtual ~Thread();

        bool yield();

        Thread* operator++(int);
 
        Thread* begin();
 
        // Get metrics data
        const Metrix& getMetrix();
 
        // Set Metrix data
        bool setNice(atomicx_time nice);

        // Wait and notify
        template<typename T> bool wait(T& refId, Tag& tag, atomicx_time timeout, uint8_t channel = 0);
    };

}; // namespace atomicx
#endif // ATOMICX_H
