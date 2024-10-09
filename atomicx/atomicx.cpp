/**
 * @file atomicx.cpp
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

#include "atomicx.h"

#ifndef ARDUINO
#include <iostream>
#include <memory>
#define LOGGING 0
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

namespace ax {

    Context ctx;

    // ----------------------------------------------
    // AtomicX Timeout methods implementation
    // ----------------------------------------------
    Timeout::Timeout () : m_timeoutValue (0)
    {
        set (0);
    }

    Timeout::Timeout (Time nTimeoutValue) : m_timeoutValue (0)
    {
        set (nTimeoutValue);
    }

    Timeout::Timeout(TIME /*type*/) : m_timeoutValue (0)
    {}

    void Timeout::set(Time nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue + getTick();
    }

    bool Timeout::isTimedOut()
    {
        return (m_timeoutValue == 0 || getTick () < m_timeoutValue) ? false : true;
    }

    Time Timeout::getRemaining()
    {
        auto nNow = getTick ();

        return (nNow < m_timeoutValue) ? m_timeoutValue - nNow : 0;
    }

    Time Timeout::getDurationSince(Time startTime)
    {
        return startTime - getRemaining ();
    }

    Time Timeout::operator()() 
    { 
        return m_timeoutValue;
    }

    // ----------------------------------------------
    // AtomicX Context methods implementation
    // ----------------------------------------------
    void Context::setNextActiveThread()
    {
        thread* thread = nullptr;

        thread = m_nextThread = m_activeThread;
        
        for(size_t nCount = 0; nCount < threadCount; nCount++)
        {
            thread = (thread == nullptr) ? begin : thread->next;

            if (thread != nullptr) switch (thread->metrics.state)
            {
            case STATE::READY:
                m_nextThread = thread;
                return;
                
            case STATE::NOW:
                m_nextThread->metrics.nextExecTime = m_switchTime;
            case STATE::SLEEPING:
                if (m_nextThread->metrics.nextExecTime >= thread->metrics.nextExecTime)
                    m_nextThread = thread;
                break;
            
            case STATE::WAIT:
                if (thread->metrics.waitTimeout() > 0 && thread->metrics.waitTimeout() < m_switchTime)
                {
                    thread->metrics.state = STATE::TIMEDOUT;
                    thread->metrics.nextExecTime = 0;
                    m_nextThread = thread;
                    return;
                }
            default:
                break;
            }
        }

        if (m_nextThread->metrics.state == STATE::WAIT)
        {
            // detects if only timeoutless threads are 
            // waiting, mark start() to finish
            if (m_nextThread->metrics.waitTimeout() == 0)
                m_nextThread = nullptr;
            else
                m_nextThread->metrics.state = STATE::TIMEDOUT;
        }
        else
            m_nextThread->metrics.state = STATE::RUNNING;
    }

    void Context::sleepUntilTick(Time until)
    {
        Time now = getTick();

        if (now < until)
        {
            sleepTicks(until - now);
        }
    }

    int Context::start()
    {
        m_running = true;

        m_nextThread = begin;

        while(m_nextThread != nullptr &&  m_running && threadCount > 0)
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
                    if (m_activeThread->metrics.state == STATE::READY)
                    {
                        m_activeThread->metrics.state = STATE::RUNNING;
                        m_activeThread->run();
                        m_activeThread->metrics.state = STATE::STOPPED;
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

    thread& Context::operator()()
    {
        return *m_activeThread;
    }

    // ----------------------------------------------
    // Thread Class Implementation
    // ----------------------------------------------

   bool thread::yield(Timeout till, STATE cmd)
    {
        // adjusting the next execution time
        if(cmd == STATE::NOW) till.set(0);
        else if (!till() && cmd != STATE::WAIT) till.set(ctx.m_activeThread->metrics.nice);
        
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

            ctx.m_activeThread->metrics.nextExecTime = till();

            //ctx.setNextActiveThread();
            ctx.m_switchTime = getTick();

            longjmp(ctx.m_activeThread->kernelRegs, 1);
        } else {
            memcpy(ctx.m_activeThread->stack.userPointer, ctx.m_activeThread->stack.vmemory, ctx.m_activeThread->metrics.stackSize);
        }

        if (ctx.m_activeThread->metrics.state == STATE::RUNNING)
            ctx.sleepUntilTick(ctx.m_activeThread->metrics.nextExecTime);

        ctx.m_activeThread->metrics.nextExecTime = getTick();

        return true;
    }

    bool thread::yieldUntil(Time timeout, size_t till, STATE cmd)
    {   
        if(ctx.m_activeThread->metrics.nextExecTime + timeout <= getTick()) 
            return yield(till, cmd);

        return true;
    }

    void thread::defaultInit(size_t* vmemory, size_t maxSize)
    {
        stack.vmemory = vmemory;
        metrics.maxStackSize = maxSize * sizeof(size_t);
        metrics.stackSize = 0;

        metrics.nextExecTime = getTick();

        // Initialize the thread Context
        metrics.state = STATE::READY;
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
    bool thread::setNice(Time nice)
    {
        metrics.nice = nice;
        return true;
    }

    // ----------------------------------------------
    // Wait / Notify methods implementation
    // ----------------------------------------------

    bool thread::wait(RefId& refId, Tag& tag, Timeout timeout, uint8_t channel)
    {
        (void) notify(refId, Notify::ONE, {0,0}, 0, ATIMICX_SYS_CHANEL);

        metrics.tag = tag;
        metrics.refId = &refId;
        metrics.waitChannel = channel;
        metrics.waitTimeout = timeout;

        if(!yield(timeout, STATE::WAIT) || metrics.state != STATE::TIMEDOUT)
        {
            return false;
        }

        tag = metrics.tag;

        return true;
    }

    size_t thread::doNotification(RefId& refId, Notify type, Tag& tag, uint8_t channel)
    {
        size_t count = 0;

        for(auto* i = begin(); i != nullptr; i = i->next)
        {
            if (i->metrics.state == STATE::WAIT)
            {
                if(i->metrics.waitChannel == channel && i->metrics.refId == &refId)
                {
                    i->metrics.state = STATE::NOW;
                    i->metrics.nextExecTime = getTick();
                    i->metrics.tag = tag;
                    i->metrics.refId = nullptr;
                    count++;
                    if(type == Notify::ONE) break;
                }
            }
        }

        return count;
    }

    size_t thread::notify(RefId& refId, Notify type, Tag tag, Timeout timeout, uint8_t channel)
    {
        size_t count = 0;
        Tag sysTag = {0,0};

        do 
            count = doNotification(refId, type, tag, channel);
        while(!count  
              && timeout() > 0 && !timeout.isTimedOut()
              && wait(refId, sysTag, timeout, ATIMICX_SYS_CHANEL));

        if (!count || !yield(0, STATE::NOW)) return 0; 

        return count;
    }

}; // namespace ax 
