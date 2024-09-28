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

    void Context::setNextActiveThread()
    {
        Thread* thread = nullptr;

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
                    m_nextThread = thread;
                break;
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

        m_activeThread = begin;

        while(m_running && threadCount > 0)
        {
            // for debugging
            //m_activeThread = m_activeThread == nullptr ? begin : m_activeThread->next;

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
            }
            
            setNextActiveThread();
            m_activeThread = m_nextThread;
        }

        return 0;
    }

    void Context::AddThread(Thread* thread)
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

    void Context::RemoveThread(Thread* thread)
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

    bool Context::yield(size_t arg, state cmd)
    {
        (void)cmd; (void)arg;
        uint8_t stackPointer = 0xBB;
        
        // Calculate stack size and store are stack.size
        m_activeThread->stack.userPointer = &stackPointer;
        m_activeThread->metrics.stackSize = (size_t)(m_activeThread->stack.kernelPointer - m_activeThread->stack.userPointer);

        if (m_activeThread->metrics.stackSize > m_activeThread->metrics.maxStackSize)
        {
            // Call the user defined StackOverflow function
            m_activeThread->StackOverflow();
            return false;
        }

        if (setjmp(m_activeThread->userRegs) == 0)
        {   
            m_activeThread->metrics.nextExecTime = getTick() + m_activeThread->metrics.nice;
            //setNextActiveThread();

            memcpy(m_activeThread->stack.vmemory, m_activeThread->stack.userPointer, m_activeThread->metrics.stackSize);
            m_activeThread->metrics.state = cmd;
            longjmp(m_activeThread->kernelRegs, 1);
        }

        memcpy(m_activeThread->stack.userPointer, m_activeThread->stack.vmemory, m_activeThread->metrics.stackSize);
        
        m_activeThread->metrics.state = state::READY;
        sleepUntilTick(m_activeThread->metrics.nextExecTime);        
        m_activeThread->metrics.nextExecTime = getTick();

        return true;
    }

    // ----------------------------------------------
    // Thread Class Implementation
    // ----------------------------------------------

    bool Thread::yield()
    {
        return _ctx.yield();
    }

    void Thread::defaultInit(size_t* vmemory, size_t maxSize)
    {
        stack.vmemory = vmemory;
        metrics.maxStackSize = maxSize * sizeof(size_t);
        metrics.stackSize = 0;

        metrics.nextExecTime = getTick();
        metrics.nice = 0;
        
        _ctx.AddThread(this);

        // Initialize the thread Context
        metrics.state = state::READY;
    }

    Thread::Thread(size_t& vmemory, size_t stackSize, Context& ictx) : _ctx(ictx)
    {
        defaultInit(&vmemory, stackSize);
    }

    Thread::~Thread()
    {
        _ctx.RemoveThread(this);
    }

    Thread* Thread::operator++(int)
    {
        return next;
    }

    Thread* Thread::begin()
    {
        return _ctx.begin;
    }

    // Get metrix data
    const Thread::Metrix& Thread::getMetrix()
    {
        return (const Metrix&) metrics;
    }

    // Set Metrix data
    bool Thread::setNice(atomicx_time nice)
    {
        metrics.nice = nice;
        return true;
    }

    // ----------------------------------------------
    // Timeout class Implementation
    // ----------------------------------------------

    Timeout::Timeout () : m_timeoutValue (0)
    {
        Set (0);
    }

    Timeout::Timeout (atomicx_time nTimeoutValue) : m_timeoutValue (0)
    {
        Set (nTimeoutValue);
    }

    void Timeout::Set(atomicx_time nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue ? nTimeoutValue + getTick () : 0;
    }

    bool Timeout::IsTimedout()
    {
        return (m_timeoutValue == 0 || getTick () < m_timeoutValue) ? false : true;
    }

    atomicx_time Timeout::GetRemaining()
    {
        auto nNow = getTick ();

        return (nNow < m_timeoutValue) ? m_timeoutValue - nNow : 0;
    }

    atomicx_time Timeout::GetDurationSince(atomicx_time startTime)
    {
        return startTime - GetRemaining ();
    }

    atomicx_time Timeout::GetTimeoutValue()
    {
        return m_timeoutValue;
    }

    // ----------------------------------------------
    // Wait / Notify methods implementation
    // ----------------------------------------------

    template<typename T> bool Thread::wait(T& refId, Tag& tag, atomicx_time timeout, uint8_t channel)
    {
        metrics.tag = tag;
        metrics.refId = &refId;
        metrics.waitChannel = channel;
        metrics.waitTimeout = timeout;

        if(_ctx.yield(timeout, state::WAIT) && metrics.state == state::TIMEDOUT)
        {
            return false;
        }

        return true;
    }

}; // namespace atomicx 
