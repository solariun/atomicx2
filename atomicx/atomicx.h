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

namespace atomicx {

    class Thread;

    enum class state {
        READY,
        RUNNING,
        SLEEPING,
        STOPPED,
        WAIT,
        LOCKED
    };

    enum class yieldCmd {
        YIELD,
        POOL
    };

    class Context
    {
    public:
        friend class Thread;

        Context()
        {}

        void setNextActiveThread();
        
        int start();

        bool yield(size_t arg = 0, yieldCmd cmd = yieldCmd::YIELD);
            
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
    };

    //extern Context ctx;

    class Thread
    {
    private:
        friend class Context;

        jmp_buf userRegs;
        jmp_buf kernelRegs;
        state threadState{state::STOPPED};

        struct Metrix
        {
            uint16_t poolId;

            atomicx_time nice;
            atomicx_time lastExecTime;
            atomicx_time lastYieldTime;

            size_t maxSize;
            size_t size;
            
        } metrix;

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
        void defaultInit(size_t* vmemory, size_t maxSize)
        {
            stack.vmemory = vmemory;
            metrix.maxSize = maxSize * sizeof(size_t);
            metrix.size = 0;

            _ctx.AddThread(this);

            // Initialize the thread Context
            threadState = state::READY;
        }

        template<size_t stackSize>
        Thread(size_t (&vmemory)[stackSize], Context& ictx) : _ctx(ictx)
        {
            defaultInit(vmemory, stackSize);
        }

        virtual ~Thread()
        {
            _ctx.RemoveThread(this);
        }

        bool yield();

        Thread* operator++(int)
        {
            return next;
        }

        Thread* begin()
        {
            return _ctx.begin;
        }

        // Get metrix data
        const Metrix& getMetrix()
        {
            return (const Metrix&) metrix;
        }

        // Set Metrix data
        bool setNice(atomicx_time nice)
        {
            metrix.nice = nice;
            return true;
        }

    };

}; // namespace atomicx
#endif // ATOMICX_H
