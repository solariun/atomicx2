#include "atomicx.h"

#ifndef ARDUINO
#include <iostream>
#include <memory>
#else
#include <Arduino.h>
#endif

#include <stddef.h>

#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include <stdlib.h>

namespace atomicx {

    Context ctx;

    void Context::setNextActiveThread()
    {
        thread* thread = nullptr;

        thread = m_nextThread = m_activeThread;
        
        for(size_t nCount = 0; nCount < threadCount; nCount++)
        {
            thread = (thread == nullptr) ? begin : thread->next;

            if (thread != nullptr) switch (thread->metrics.state)
            {
            case state::READY:
            case state::TIMEDOUT:
            case state::NOW:
                m_nextThread = thread;
                return;
                
            case state::SLEEPING:
                if (m_nextThread->metrics.nextExecTime >= thread->metrics.nextExecTime)
                    m_nextThread = thread;
                break;
            
            case state::WAIT:
                if (thread->metrics.waitTimeout > 0
                    && m_nextThread->metrics.nextExecTime >= thread->metrics.nextExecTime)
                {
                    m_nextThread = thread;
                    break;
                }
            default:
                break;
            }
        }
    }

    void Context::sleepUntilTick(atomicx_time until)
    {
        atomicx_time now = getTick();

        if (now < until)
        {
            sleepTicks(until - now);
        }
    }

    int Context::start()
    {
        m_running = true;

        m_nextThread = begin;

        while(m_running && threadCount > 0)
        {
            // for debugging
            //m_activeThread = m_activeThread == nullptr ? begin : m_activeThread->next;
            
            // for production
            m_activeThread = m_nextThread;

            if (m_activeThread != nullptr)
            {
                uint8_t kernelPointer = 0xAA;
                m_activeThread->stack.kernelPointer = &kernelPointer;
                if (setjmp(m_activeThread->kernelRegs) == 0)
                {
                    if (m_activeThread->metrics.state == state::READY)
                    {
                        m_activeThread->metrics.state = state::RUNNING;
                        m_activeThread->run();
                        m_activeThread->metrics.state = state::STOPPED;
                    } else {
                        longjmp(m_activeThread->userRegs, 1);
                    }
                }

                setNextActiveThread();
            }
        }
        return 0;
    }

    void Context::AddThread(thread* thread)
    {
        if (begin == nullptr)
        {
            begin = thread;
            last = thread;
        } else {
            last->next = thread;
            thread->prev = last;
            last = thread;
        }
        threadCount++;
    }

    void Context::RemoveThread(thread* thread)
    {
        if (thread == begin)
        {
            begin = thread->next;
        }
        else if (thread == last)
        {
            last = thread->prev;
        }
        else {
            if (thread->prev != nullptr)
                thread->prev->next = thread->next;
            if (thread->next != nullptr)
                thread->next->prev = thread->prev;
        }
        threadCount--;
    }

    // ----------------------------------------------
    // Thread Class Implementation
    // ----------------------------------------------

   bool thread::yield(size_t arg, state cmd)
    {
        (void)cmd; (void)arg;
        uint8_t stackPointer = 0xBB;
                
        // Calculate stack size and store are stack.size
        ctx.m_activeThread->stack.userPointer = &stackPointer;
        ctx.m_activeThread->metrics.stackSize = (size_t)(ctx.m_activeThread->stack.kernelPointer - ctx.m_activeThread->stack.userPointer);

        if (ctx.m_activeThread->metrics.stackSize > ctx.m_activeThread->metrics.maxStackSize)
        {
            // Call the user defined StackOverflow function
            ctx.m_activeThread->StackOverflow();
            return false;
        }

        if (setjmp(ctx.m_activeThread->userRegs) == 0)
        {   
            memcpy(ctx.m_activeThread->stack.vmemory, ctx.m_activeThread->stack.userPointer, ctx.m_activeThread->metrics.stackSize);
            
            ctx.m_activeThread->metrics.state = cmd;

            ctx.m_activeThread->metrics.nextExecTime = getTick() + ctx.m_activeThread->metrics.nice;            

            //ctx.setNextActiveThread();

            longjmp(ctx.m_activeThread->kernelRegs, 1);
        } else {
            memcpy(ctx.m_activeThread->stack.userPointer, ctx.m_activeThread->stack.vmemory, ctx.m_activeThread->metrics.stackSize);
        }

        ctx.m_activeThread->metrics.state = state::READY;
        ctx.sleepUntilTick(ctx.m_activeThread->metrics.nextExecTime);        
        ctx.m_activeThread->metrics.nextExecTime = getTick();

        return true;
    }

    bool thread::yieldUntil(atomicx_time timeout, size_t arg, state cmd)
    {   
        if(ctx.m_activeThread->metrics.nextExecTime + timeout <= getTick()) 
            return yield(arg, cmd);

        return true;
    }

    void thread::defaultInit(size_t* vmemory, size_t maxSize)
    {
        stack.vmemory = vmemory;
        metrics.maxStackSize = maxSize * sizeof(size_t);
        metrics.stackSize = 0;

        metrics.nextExecTime = getTick();

        // Initialize the thread Context
        metrics.state = state::READY;
    }

    thread::thread(size_t& vmemory, size_t stackSize)
    {
        defaultInit(&vmemory, stackSize);
        ctx.AddThread(this);
    }

    thread::~thread()
    {
        ctx.RemoveThread(this);
    }

    thread* thread::operator++(int)
    {
        return next;
    }

    thread* thread::begin()
    {
        return ctx.begin;
    }

    // Get metrix data
    const thread::Metrics& thread::getMetrics()
    {
        return (const Metrics&) metrics;
    }

    // Set Metrics data
    bool thread::setNice(atomicx_time nice)
    {
        metrics.nice = nice;
        return true;
    }

    // ----------------------------------------------
    // Wait / Notify methods implementation
    // ----------------------------------------------

    bool thread::wait(RefId& refId, Tag& tag, atomicx_time timeout, uint8_t channel)
    {
        metrics.tag = tag;
        metrics.refId = &refId;
        metrics.waitChannel = channel;
        metrics.waitTimeout = timeout;

        // TODO: Add a notification for a wait call transaction.
        //ctx.yield();

        if(yield(timeout, state::WAIT) && metrics.state == state::TIMEDOUT)
        {
        return false;
        }

        return true;
    }

    size_t thread::notify(RefId& refId, Notify type, Tag tag, atomicx_time timeout, uint8_t channel)
    {
        size_t count = 0;

        (void)refId; (void)type; (void)tag; (void)timeout; (void)channel;

        // TODO: Add a waiting for a wait call transaction.
        for(auto* i = begin(); i != nullptr; i = i->next)
        {
            if (i->metrics.state == state::WAIT)
            {
                if(i->metrics.waitChannel == channel && i->metrics.refId == &refId)
                {
                    i->metrics.state = state::SLEEPING;
                    i->metrics.nextExecTime = getTick() + timeout;
                    i->metrics.tag = tag;
                    i->metrics.refId = nullptr;
                    count++;
                    if(type == Notify::ONE) break;
                }
            }
        }

        yield();

        return count;
    }
}; // namespace atomicx 
