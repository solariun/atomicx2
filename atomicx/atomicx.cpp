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
#include <new>

#ifdef ARDUINO

#ifdef __AVR__
constexpr uint8_t k_d2_bit = _BV(PD2);
constexpr uint8_t k_d3_bit = _BV(PD3);

constexpr uint8_t k_dac_mask = 0x3F;           // 6-bit R-2R ladder on A0..A5 = PC0..PC5; PC6 is RESET
constexpr uint8_t k_dac_bits = 6;
constexpr uint8_t k_analog_write_bits = 8;      // analogWrite()'s 0..255

#define  PIN_ON(pin) PORTD |= k_d##pin##_bit
#define  PIN_OFF(pin) PORTD &= (uint8_t)~k_d##pin##_bit

__inline void dac_init()
{
    DDRC |= k_dac_mask;
}

// Like analogWrite(): 0..255 -> value / 256 * Vcc at the ladder's output (128 -> 2.5 V, 255 -> 4.92 V)
__inline void dac_write(uint8_t value)
{
    PORTC = (PORTC & (uint8_t)~k_dac_mask) | (value >> (k_analog_write_bits - k_dac_bits));
}
#endif
#endif

// LOG_ERROR("text " << value); prints one line to std::cerr on the PC; on Arduino, the preprocessor removes the call and its arguments
#ifdef ARDUINO
#define LOG_ERROR(...) do { } while (0)
#else
#define LOG_ERROR(...) do { std::cerr << __VA_ARGS__ << std::endl; } while (0)
#endif

// No trace on Arduino: TRACE_ON is ignored, so every TRACE line, dump_data() and dump_threads() compile to nothing
#ifdef ARDUINO
#undef TRACE_ON
#endif

// TRACE("text " << value); prints one line with TRACE_ON; without it, the preprocessor removes the call and its arguments
#ifdef TRACE_ON
#define TRACE(...) do { std::cout << "[TRACE] " << __FILE__ << ":" << __LINE__ << " - " << __VA_ARGS__ << std::endl << std::flush; } while (0)
#else
#define TRACE(...) do { } while (0)
#endif

namespace ax {

    thread_context context; // Definition of the global thread context object

    void dump_data(const void* data, size_t size, const char* label = nullptr)
    {
#ifdef TRACE_ON
        static const size_t k_bytes_per_line = 16;
        static const size_t k_bytes_per_block = 8;
        static const unsigned k_nibble_bits = 4;
        static const unsigned k_nibble_mask = 0x0F;
        static const char k_hex_digits[] = "0123456789abcdef";
        static const size_t k_hex_chars = 3; // "xx "
        static const size_t k_gap_chars = 2;
        static const size_t k_line_chars = k_bytes_per_line * k_hex_chars + 2 * k_gap_chars + k_bytes_per_line + 1;
        static const unsigned char k_first_printable = 0x20; // isprint() in the C locale, without a library call
        static const unsigned char k_last_printable = 0x7e;

        const unsigned char* bytes = static_cast<const unsigned char*>(data);

        if (label) TRACE("[" << label << "]:");

        for (size_t line = 0; line < size; line += k_bytes_per_line)
        {
            // One TRACE per line, so the prefix is not repeated on every byte
            char text[k_line_chars];
            size_t pos = 0;

            for (size_t i = 0; i < k_bytes_per_line; ++i)
            {
                if (i == k_bytes_per_block) { memset(text + pos, ' ', k_gap_chars); pos += k_gap_chars; }

                if (line + i < size)
                {
                    unsigned char value = bytes[line + i];
                    text[pos++] = k_hex_digits[value >> k_nibble_bits];
                    text[pos++] = k_hex_digits[value & k_nibble_mask];
                    text[pos++] = ' ';
                }
                else
                {
                    memset(text + pos, ' ', k_hex_chars);
                    pos += k_hex_chars;
                }
            }

            memset(text + pos, ' ', k_gap_chars);
            pos += k_gap_chars;

            for (size_t i = 0; i < k_bytes_per_line && line + i < size; ++i)
            {
                unsigned char value = bytes[line + i];
                text[pos++] = (value >= k_first_printable && value <= k_last_printable) ? (char)value : '.';
            }

            text[pos] = '\0';
            TRACE(text);
        }
#endif
        (void)data; (void)size; (void)label;
    }

    // ===============================================
    //  AUTO_OBJ_LIST ITEM IMPLEMENTATION
    // ===============================================

    // Ticks wrap around: a time is due while now is less than half the tick range past it
    // static const time k_tick_half_range = (time)(((time)~(time)0 >> 1) + 1);

    auto_obj_list::item::item(auto_obj_list& parent) : parent(parent)
    { 
        parent.add(this); 
        //TRACE("Added item to parent list at address: " << this);
    }

    auto_obj_list::item::~item()
    { 
        parent.remove(this);
        // TRACE("Removed item from parent list at address: " << this);
    }

    auto_obj_list::item* auto_obj_list::item::getNextItem(item* current) const { return current ? current->next : nullptr; }
    auto_obj_list::item* auto_obj_list::item::getPrevItem(item* current) const { return current ? current->prev : nullptr; }

    bool auto_obj_list::isEmpty() const { return head == nullptr; }

    size_t auto_obj_list::count() const volatile
    {
        return _count;
    }

    auto_obj_list::item* auto_obj_list::getHeadItem() const { return head; }

    auto_obj_list::item* auto_obj_list::getTailItem() const { return tail; }

    bool auto_obj_list::swap(item* a, item* b) volatile
    {
        if (!a || !b || &a->parent != this || &b->parent != this) return false;
        if (a == b) return true;

        // Neighbours are handled as a before b
        if (b->next == a) { item* first = b; b = a; a = first; }

        item* a_prev = a->prev;
        item* a_next = a->next;
        item* b_prev = b->prev;
        item* b_next = b->next;

        if (a_next == b)
        {
            a->prev = b;
            a->next = b_next;
            b->prev = a_prev;
            b->next = a;
        }
        else
        {
            a->prev = b_prev;
            a->next = b_next;
            b->prev = a_prev;
            b->next = a_next;
            if (a_next) a_next->prev = b;
            if (b_prev) b_prev->next = a;
        }

        if (a_prev) a_prev->next = b;
        if (b_next) b_next->prev = a;

        if (head == a) head = b; else if (head == b) head = a;
        if (tail == a) tail = b; else if (tail == b) tail = a;

        return true;
    }

    bool auto_obj_list::add(item* newItem) volatile
    {
        if (!newItem) return false;
        newItem->next = nullptr;
        newItem->prev = tail;
        if (tail) tail->next = newItem;
        tail = newItem;
        if (!head) head = newItem;
        _count++;
        return true;
    }

    bool auto_obj_list::remove(item* itemToRemove) volatile
    {
        if (!itemToRemove) return false;
        if (itemToRemove->prev) itemToRemove->prev->next = itemToRemove->next;
        if (itemToRemove->next) itemToRemove->next->prev = itemToRemove->prev;
        if (itemToRemove == head) head = itemToRemove->next;
        if (itemToRemove == tail) tail = itemToRemove->prev;
        itemToRemove->next = nullptr;
        itemToRemove->prev = nullptr;
        _count--;
        return true;
    }

    // ===============================================
    //  THREAD ITEM IMPLEMENTATION
    // ===============================================

    thread_item::thread_item(size_t stack_size) : item(ax::context), _v_stackPointer(new (std::nothrow) volatile char[stack_size]), _stackSize(stack_size) 
    {

        if (!this->_v_stackPointer) {
            LOG_ERROR("Failed to allocate stack for thread item.");
            _stackSize = 0;
        } else {
            // TRACE(">>> Allocated stack for thread item at address: " << this->_v_stackPointer << " with size: " << stack_size);
        }
    }

    thread_item::~thread_item()
    {
        delete[] _v_stackPointer;
        _v_stackPointer = nullptr;
    }

    // The schedule order's first key: a lower rank runs first
    enum class Rank { READY, RUNNING, SLEEP, TERMINATED };

    const char* toStr(State state)
    {
        // A case per State with no default, as in rank(): a new State does not compile until it is named
        switch (state)
        {
            case State::READY:      return ("READY");
            case State::NOW:        return ("NOW");
            case State::RUNNING:    return ("RUNNING");
            case State::SLEEPING:   return ("SLEEPING");
            case State::WAITING:    return ("WAITING");
            case State::NOTIFY:     return ("NOTIFY");
            case State::TERMINATED: break;
        }

        // TERMINATED returns here, so GCC sees a return on every path
        return ("TERMINATED");
    }

    static inline Rank rank(State state)
    {
        // A case per State with no default, as in toStr(): a new State does not compile until it is ranked
        switch (state)
        {
            case State::NOW:
            case State::READY:      return Rank::READY;
            case State::NOTIFY:
            case State::WAITING:
            case State::RUNNING:    return Rank::RUNNING;
            case State::SLEEPING:   return Rank::SLEEP;
            case State::TERMINATED: break;
        }

        // TERMINATED returns here, so GCC sees a return on every path
        return Rank::TERMINATED;
    }

    bool thread_item::compare(const item& a, const item& b, const thread_item* data)
    {
        const thread_item& first = static_cast<const thread_item&>(a);
        const thread_item& second = static_cast<const thread_item&>(b);

        if (first._data.timeout.isForever()) return true;

        const Rank first_rank = rank(first._data.state);
        const Rank second_rank = rank(second._data.state);

        (void) data;

        if (first_rank != second_rank) return first_rank < second_rank;

        return (time) first._data.timeout < (time) second._data.timeout;
    }

    void thread_item::dump_threads() volatile
    {
#ifdef TRACE_ON
        const thread_item* current = static_cast<const thread_item*>(ax::context.getHeadItem());
        
        TRACE("[DUMP] Dumping all threads in the parent context: ------------------------");
        while (current)
        {
            TRACE(" * [DUMP] Thread " << current->getName() << " at address: " << \
            current << " State: [" << static_cast<int>(current->_data.state) << ":" << toStr(current->_data.state) << "]" \
            << " Nice: " << current->_data.nice << \
            " remaining time: " << (long long int)current->_data.timeout.getRemainingTime());
            
            current = static_cast<const thread_item*>(current->next);
        }
        TRACE("[DUMP] --------------------------------");
#endif
    }

    void thread_item::setNice(ax::time nice)
    {
        _data.nice = nice;
    }

    ax::time thread_item::getNice() const
    {
        return _data.nice;
    }
    
    ssize_t thread_item::getUsedStackSize() const
    {
        return ax::context._stk_top - ax::context._current->_stk_bottom;
    }

    timeout thread_item::yield(timeout duration)
    {
#ifdef __AVR__
        PIN_ON(2);              // on yield timing
        PIN_ON(3);              // on sort timing
#endif
        _data.timeout = duration.forever() || duration > 0 ? duration : _data.nice;

        // Implementation of yield can be added here
        ax::context.sort(compare, this);
        dump_threads();

#ifdef __AVR__
        PIN_OFF(3);    // off D3
#endif
        if (ax::context.getHeadItem() != nullptr)
        {
            ax::context._current = static_cast<thread_item*>(ax::context.getHeadItem());
            
            if (setjmp(this->_register_context) == 0)
            {
                // dump_data(current->_register_context, sizeof(current->_register_context), "Set Jump Context");
                
                volatile char marker = 0xAA;
                this->_stk_bottom = (volatile char*) &marker;

                if (ax::context._stk_top == nullptr)
                {
                    LOG_ERROR("[YIELD] *** STACK ERROR ***: Stack top is not set.");
                    exit(EXIT_FAILURE);
                }

                this->usedStackSize = ax::context._stk_top - this->_stk_bottom;
                
                if (this->usedStackSize > this->_stackSize)
                {
                    LOG_ERROR("[YIELD] *** STACK OVERFLOW ***: Used stack size exceeds allocated stack size. max: " << this->_stackSize << " used: " << this->usedStackSize << ", diff:" << (this->usedStackSize - this->_stackSize));
                    StackOverflow();
                    longjmp(ax::context._top_context, 3);
                }

                ax::context.save((volatile size_t*) ax::context._stk_top,
                              (volatile size_t*) this->_v_stackPointer,
                              this->usedStackSize);
                
                TRACE("[YIELD] Stack saved for thread at address: " << this);

                TRACE("[YIELD] Preparing next thread for execution: " << ax::context._current << " name: " << ax::context._current->getName() << " state: " << toStr(ax::context._current->_data.state));
                
                if (ax::context._current->_data.state == ax::State::READY)
                {
                    TRACE("[YIELD] Next thread is ready to run.");
                    longjmp(ax::context._top_context, 1);
                    exit(EXIT_FAILURE);

                } else if ((ax::context._current->_data.state == ax::State::RUNNING 
                           || ax::context._current->_data.state == ax::State::NOW 
                           || ax::context._current->_data.state == ax::State::WAITING)
                           && ax::context._current->_data.timeout.isForever() == false) {
                    {
                        // Sleep until the thread is due; ticks wrap, so the distance is taken modulo the tick range (R3.16)
                        ssize_t remaining = ax::context._current->_data.timeout.getRemainingTime();
                        TRACE("[YIELD] ****---> Thread remaining time: " << (size_t) remaining);
                        if (remaining > 0)
                        {
                            ax::sleepTicks(remaining);
                        }
                    }
                    
                    TRACE("[YIELD] Woke up from sleep, resuming next thread. Status: " << toStr(ax::context._current->_data.state));
                    longjmp(ax::context._current->_register_context, 1);

                } else {  
                    TRACE("[YIELD]  **** Next thread is not ready to run. ****");
                    longjmp(ax::context._top_context, 2);    
                    exit(EXIT_FAILURE);
                }
            } else {
                ax::context.restore((volatile size_t*) ax::context._stk_top, 
                                    (volatile size_t*) ax::context._current->_v_stackPointer, 
                                    ax::context._current->usedStackSize);
            }
        } else {
            TRACE("[YIELD] Thread is not in a state to yield.");
            longjmp(ax::context._top_context, 2);    
            exit(EXIT_FAILURE);
        }
    
#ifdef __AVR__
        PIN_OFF(2);    // off D2
#endif
        // resume() put this thread's slice back before jumping here: the frame is ours again
        ax::context._current->_data.lastRunTime = ax::getTick();
        
        return ax::context._current->_data.timeout;
    }

    const char* thread_item::getName() const { return _data.name; }
    void thread_item::setName(const char* name) { _data.name = name; }

    // ===============================================
    //  THREAD CONTEXT IMPLEMENTATION
    // ===============================================

    bool thread_context::start()
    {
#ifdef __AVR__
        DDRD  |= k_d2_bit;              // output (once, in setup)
        DDRD  |= k_d3_bit;              // output (once, in setup)

        PIN_OFF(2);    // off D2
        PIN_OFF(3);    // off D3
#endif

        if (isEmpty() || running) return false;
        
        running = true;
        
        thread_item* current = (thread_item*) getHeadItem();

        sort(thread_item::compare, current);
        
        volatile char dummy = 0xBB;
        _stk_top = &dummy;

        int jmp_val = setjmp(_top_context);
        //TRACE("[START] setjmp returned with value: " << jmp_val);

        if (jmp_val > 1) 
        {
            goto error;
        } else {
            //TRACE("[START] Starting thread context.");
        }

        current = (thread_item*) getHeadItem();
        current->_data.state = ax::State::RUNNING;
        current->_data.lastRunTime = getTick();

        (void)current->run();

    error:

        return false;
    }

    // Puts next's slice back and jumps into it. Called from a frame inside that slice, it first calls itself
    // until its own frame is below the slice, so the copy never overwrites a frame still in use. Each call
    // passes its marker down, so the compiler cannot turn the recursion into a loop.
    // Every path ends in longjmp, so GCC >= 12 reports -Winfinite-recursion; the recursion is intended
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
#endif
    void thread_context::resume(thread_item& next, volatile char* caller_marker)
    {
        volatile char marker = 0;

        // This frame lies below the caller's sp, itself below the caller's marker: once that marker is below the
        // slice, this whole frame is too. From yield() (no marker) it always goes one call deeper
        if (!caller_marker || (uintptr_t)caller_marker >= (uintptr_t)next._stk_bottom) resume(next, &marker);

        restore((volatile size_t*) _stk_top, (volatile size_t*) next._v_stackPointer, _stk_top - next._stk_bottom);
        longjmp(next._register_context, 1);
    }
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif

    void thread_context::save (volatile size_t* stackTop, volatile size_t* stackPointer, size_t stackSize) volatile
    {
        // The stack grows down: the slice is the stackSize bytes below stackTop
        memcpy((void*)stackPointer, (void*)((volatile char*)stackTop - stackSize), stackSize);
        return;
    }

    void thread_context::restore (volatile size_t* stackTop, volatile size_t* stackPointer, size_t stackSize) volatile
    {
        // The stack grows down: the slice is the stackSize bytes below stackTop
        memcpy((void*)((volatile char*)stackTop - stackSize), (void*)stackPointer, stackSize);
        return;
    }

    timeout thread_item::wait(ref& reference, message& msg, timeout duration)
    {
        // Implementation goes here
        // Mark the current thread as waiting
        _data.state = ax::State::WAITING;
        _data.timeout = duration;
        _data.waitingOn = &reference;

        ax::context.iterate<thread_item&>([&reference](thread_item& item)->bool
        {
            if (item._data.state == ax::State::NOTIFY && item._data.waitingOn == &reference)
            {
                item._data.state = ax::State::NOW;
                return true;
            }
            return false;
        });

        // Save the current context and yield
        yield(duration);
        msg = _data.message;
        _data.waitingOn = nullptr; // Clear the waiting reference after resuming

        // After being resumed, return the remaining timeout
        return _data.timeout;
    }

    size_t thread_item::notify(ref& reference, message msg, size_t count, timeout duration)
    {
       
        _data.waitingOn = &reference;

        size_t notifiedCount = 0;

        // Notify all threads on NOTIFY state that are waiting on the same referencexe
        ax::context.iterate<thread_item&>([&reference, &notifiedCount](thread_item& item)->bool
        {
            if (item._data.state == ax::State::WAITING && item._data.waitingOn == &reference)
            {
                item._data.state = ax::State::NOW;
                notifiedCount++;
                return true;
            }
            return false;
        });

        if (notifiedCount > 0 &&  (_data.state = ax::State::NOTIFY, true) && yield(duration) == true) return 0;

        count = (count == 0) ? ax::context.count() : count;

        notifiedCount = 0;

        ax::context.iterate<thread_item&>([&reference, &notifiedCount, &msg](thread_item& item)->bool
        {
            if (item._data.state == ax::State::WAITING && item._data.waitingOn == &reference)
            {
                item._data.state = ax::State::NOW;
                item._data.message = msg; // Set the message for the waiting thread
                notifiedCount++;
                return true;
            }
            return false;
        }, count);
        
        return notifiedCount;
    }

    // ===============================================
    //  TIMEOUT CLASS IMPLEMENTATION
    // ===============================================

    ax::timeout::timeout(ax::time duration)  : _target(0), _forever(false)
    {
        reset(duration);
    }
 
    ax::timeout::timeout() : _target(0), _forever(true)
    {}
 
    bool ax::timeout::hasExpired() const
    {
        if (_forever) return false;
        // expired once now >= target; the signed difference keeps it right across the tick wrap (P45)
        return (int32_t)(ax::getTick() - _target) >= 0;
    }

    void ax::timeout::reset(ax::time duration)
    {
        _forever = false;
        _target = ax::getTick() + duration;
    }

    ssize_t ax::timeout::getRemainingTime() const
    {
        return getRemainingTime(ax::getTick());
    }

    ssize_t ax::timeout::getRemainingTime(time tm_now) const
    {
        if (_forever) return -1;
        ssize_t remaining = (ssize_t)_target - (ssize_t)(tm_now);
        return remaining;
    }

    bool ax::timeout::forever() const
    {
        return _forever == true;
    }

    bool ax::timeout::isForever() const
    {
        return _forever == true;
    }

    bool ax::timeout::operator >(const timeout& other) const
    {
       return _target > other._target;
    }
    bool ax::timeout::operator <(const timeout& other) const
    {
        return _target < other._target;
    }
    bool ax::timeout::operator >=(const timeout& other) const
    {
        return _target >= other._target;
    }
    bool ax::timeout::operator <=(const timeout& other) const
    {
        return _target <= other._target;
    }
    bool ax::timeout::operator ==(const timeout& other) const
    {
        return _target == other._target;
    }
    bool ax::timeout::operator !=(const timeout& other) const
    {
        return _target != other._target;
    }
    ax::timeout::operator bool() const
    {
        return !hasExpired();
    }

    ax::timeout::operator time() const
    {
        return _target;
    }

    ax::timeout::operator ssize_t() const
    {
        return static_cast<ssize_t>(getRemainingTime());
    }

    ax::timeout& ax::timeout::operator =(time target)
    {
        reset(target);
        return *this;
    }

    bool ax::timeout::operator()() const
    {
        return hasExpired();
    }
}