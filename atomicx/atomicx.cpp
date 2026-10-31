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
// Serial has no operator<<: each piece goes to Serial.print
struct serial_stream
{
    serial_stream& operator<<(const char* text) { Serial.print(text); return *this; }
    serial_stream& operator<<(char value) { Serial.print(value); return *this; }
    serial_stream& operator<<(int value) { Serial.print(value); return *this; }
    serial_stream& operator<<(unsigned int value) { Serial.print(value); return *this; }
    serial_stream& operator<<(long value) { Serial.print(value); return *this; }
    serial_stream& operator<<(unsigned long value) { Serial.print(value); return *this; }
    serial_stream& operator<<(const volatile void* address) { Serial.print("0x"); Serial.print((unsigned long)(uintptr_t)address, HEX); return *this; }
};
#endif

// LOG_ERROR(a << b); always prints one line: std::cerr on the PC, Serial on Arduino
#ifdef ARDUINO
#define LOG_ERROR(...) do { serial_stream() << __VA_ARGS__; Serial.println(); } while (0)
#else
#define LOG_ERROR(...) do { std::cerr << __VA_ARGS__ << std::endl; } while (0)
#endif

// TRACE(a << b); prints one line with TRACE_ON; without it, the preprocessor removes the call and its arguments
#ifdef TRACE_ON
#ifdef ARDUINO
#define TRACE(...) do { serial_stream() << "[TRACE] " << __FILE__ << ":" << __LINE__ << " - " << __VA_ARGS__; Serial.println(); } while (0)
#else
#define TRACE(...) do { std::cout << "[TRACE] " << __FILE__ << ":" << __LINE__ << " - " << __VA_ARGS__ << std::endl; } while (0)
#endif
#else
#define TRACE(...) do { } while (0)
#endif

namespace ax {

    void dump_data(const void* data, size_t size, const char* label = nullptr)
    {
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
    }

    // Ticks wrap around: a time is due while now is less than half the tick range past it
    static const time k_tick_half_range = (time)(((time)~(time)0 >> 1) + 1);

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

    thread_item::thread_item(thread_context& parent, size_t stack_size) : item(parent), _v_stackPointer(new (std::nothrow) volatile char[stack_size]), _stackSize(stack_size * sizeof(size_t)), _context(parent) 
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
    enum class Rank { READY, RUNNING, WAITING, TERMINATED };

    static Rank rank(State state)
    {
        Rank order = Rank::TERMINATED;

        switch (state)
        {
            case State::NOW:
            case State::READY:
                order = Rank::READY;
                break;

            case State::RUNNING:
                order = Rank::RUNNING;
                break;

            case State::SLEEPING:
            case State::WAITING:
                order = Rank::WAITING;
                break;

            case State::TERMINATED:
                order = Rank::TERMINATED;
                break;
        }

        return order;
    }

    bool thread_item::compare(const item& a, const item& b, const thread_item* data)
    {
        const thread_item& first = static_cast<const thread_item&>(a);
        const thread_item& second = static_cast<const thread_item&>(b);

        Rank first_rank = rank(first._data.state);
        Rank second_rank = rank(second._data.state);

        if (first_rank != second_rank) return first_rank < second_rank;

        time now = data->_data.tm_now;

        // Earliest deadline first. Offsetting each deadline by now - half range maps
        // "half range overdue .. half range ahead" onto 0 .. max, so the keys order
        // correctly across the tick wrap (and the most overdue thread comes first)
        time first_key = (time)(first._data.lastRunTime + first._data.nice - now + k_tick_half_range);
        time second_key = (time)(second._data.lastRunTime + second._data.nice - now + k_tick_half_range);

        return first_key < second_key;
    }

    void thread_item::dump_threads() volatile
    {   
        const thread_item* current = static_cast<const thread_item*>(_context.getHeadItem());
        
        // TRACE("[DUMP] Yielding thread at address: " << (const void*)this);
        // TRACE("[DUMP] Stack pointer set to: " << (const void*)_stk_bottom);
        // TRACE("[DUMP] Virtual stack pointer: " <<(const void*)(_v_stackPointer));
        // TRACE("[DUMP] Current time: " << _data.tm_now);
        // TRACE("[DUMP] Nice value: " << _data.nice);
        // TRACE("[DUMP] Last run time: " << _data.lastRunTime);
        // TRACE("[DUMP] State: " << static_cast<int>(_data.state));
        // TRACE("[DUMP] Parent context address: " << &_context);
        // TRACE("[DUMP] Head item address: " << _context.getHeadItem());
        // TRACE("[DUMP] Tail item address: " << _context.getTailItem());

        TRACE("[DUMP] Dumping all threads in the parent context:");
        while (current)
        {
            TRACE(" * [DUMP] Thread at address: " << current << " State: " << static_cast<int>(current->_data.state) << " Nice: " << current->_data.nice << " Last run time: " << current->_data.lastRunTime);
            current = static_cast<const thread_item*>(current->next);
        }
        TRACE("[DUMP] --------------------------------");
    }

    void thread_item::setNice(ax::time nice)
    {
        _data.nice = (volatile ax::time)nice;
    }

    ax::time thread_item::getNice() const
    {
        return _data.nice;
    }
    
    void thread_item::yield()
    {
        _data.tm_now = getTick();

        //dump_threads();

        // Implementation of yield can be added here
        _context.sort(compare, this);
        
        auto next = static_cast<thread_item*>(_context.getHeadItem());
        
        if (next != nullptr)
        {
            auto* current = _context._current = next;

            if (setjmp(current->_register_context) == 0)
            {
                // dump_data(current->_register_context, sizeof(current->_register_context), "Set Jump Context");
        
                volatile char dummy = 0xAA;
                _stk_bottom =  &dummy;

                size_t usedStackSize = _context._stk_top - _stk_bottom;

                if (usedStackSize > _stackSize)
                {
                    LOG_ERROR("[YIELD] *** STACK OVERFLOW ***: Used stack size exceeds allocated stack size. max: " << _stackSize << " used: " << usedStackSize << ", diff:" << (usedStackSize - _stackSize));
                    longjmp(_context._top_context, 3);
                }

                _context.save((volatile size_t*) _context._stk_top, (volatile size_t*) _v_stackPointer, usedStackSize);
                TRACE("[YIELD] Stack saved for thread at address: " << current);

                if (current->_data.state == ax::State::READY)
                {
                    TRACE("[YIELD] Next thread is ready to run.");
                    longjmp(_context._top_context, 1);
                    exit(EXIT_FAILURE);
                } else if (current->_data.state == ax::State::RUNNING) {
                    // dump_data(current->_register_context, sizeof(current->_register_context), "Register Context");
                    fflush(stdout);
                    
                    // Sleep until the thread is due; ticks wrap, so the distance is taken modulo the tick range (R3.16)
                    ax::time remaining = (ax::time)(current->_data.lastRunTime + current->_data.nice - ax::getTick());
                    if (remaining != 0 && remaining < k_tick_half_range)
                    {
                        ax::sleepTicks(remaining);
                    }
                    
                    TRACE("[YIELD] Woke up from sleep, resuming thread.");
                    longjmp(current->_register_context, 1);
                    exit(EXIT_FAILURE);
                } else {  
                    TRACE("[YIELD]  **** Next thread is not ready to run. ****");
                    longjmp(_context._top_context, 2);    
                    exit(EXIT_FAILURE);
                }
            } else {
                //TRACE("[YIELD] Returning to previously saved context.");
                _context.restore((volatile size_t*) _context._stk_top, (volatile size_t*) _context._current->_v_stackPointer, _context._stk_top - _context._current->_stk_bottom);
                TRACE("[YIELD] Context restored.");
                current->_data.lastRunTime = ax::getTick();
            }
        }

        return;
    }

    bool thread_context::start()
    {
        if (isEmpty() || running) return false;
        
        running = true;
        
        thread_item* current = (thread_item*) getHeadItem();

        sort(thread_item::compare, current);
        
        {
            volatile char dummy = 0xBB;
            _stk_top = &dummy;
        }

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
}