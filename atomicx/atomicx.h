#ifndef ATOMICX_H
#define ATOMICX_H

#include <setjmp.h>

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

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
    public:
        void defaultInit(size_t* vmemory, size_t maxSize)
        {
            stack.vmemory = vmemory;
            stack.maxSize = maxSize * sizeof(size_t);
            stack.size = 0;

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

    protected:
        bool virtual run() = 0;

        bool virtual StackOverflow() = 0;

    private:
        friend class Context;

        jmp_buf userRegs;
        jmp_buf kernelRegs;
        state threadState{state::STOPPED};
        struct
        {
            uint8_t *kernelPointer{nullptr};
            uint8_t *userPointer{nullptr};
            size_t maxSize;
            size_t size;
            size_t *vmemory;
        } stack;

        // Node control
        Thread* next{nullptr};
        Thread* prev{nullptr};

        Context& _ctx;
    };

}; // namespace atomicx
#endif // ATOMICX_H
