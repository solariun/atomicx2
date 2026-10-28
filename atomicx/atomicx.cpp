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
#include <ctype.h>
#include <stdint.h>
#include <setjmp.h>

#include <stdlib.h>
#include <new>

namespace ax {

    void dump_data(const void* data, size_t size, const char* label = nullptr)
    {
        static const size_t k_bytes_per_line = 16;
        static const size_t k_bytes_per_block = 8;
        static const unsigned k_nibble_bits = 4;
        static const unsigned k_nibble_mask = 0x0F;
        static const char k_hex_digits[] = "0123456789abcdef";

        const unsigned char* bytes = static_cast<const unsigned char*>(data);

        if (label) std::cout << "[" << label << "]:" << std::endl; 

        for (size_t line = 0; line < size; line += k_bytes_per_line)
        {
            for (size_t i = 0; i < k_bytes_per_line; ++i)
            {
                if (i == k_bytes_per_block) std::cout << "  ";

                if (line + i < size)
                {
                    unsigned char value = bytes[line + i];
                    std::cout << k_hex_digits[value >> k_nibble_bits] << k_hex_digits[value & k_nibble_mask] << ' ';
                }
                else
                    std::cout << "   ";
            }

            std::cout << "  ";

            for (size_t i = 0; i < k_bytes_per_line && line + i < size; ++i)
            {
                unsigned char value = bytes[line + i];
                std::cout << (isprint(value) ? (char)value : '.');
            }

            std::cout << std::endl;
        }
    }

    // Ticks wrap around: a time is due while now is less than half the tick range past it
    static const time k_tick_half_range = (time)(((time)~(time)0 >> 1) + 1);

    auto_obj_list::item::item(auto_obj_list& parent) : parent(parent)
    { 
        parent.add(this); 
        //std::cout << "Added item to parent list at address: " << this << std::endl;
    }

    auto_obj_list::item::~item()
    { 
        parent.remove(this);
        // std::cout << "Removed item from parent list at address: " << this << std::endl;
    }

    auto_obj_list::item* auto_obj_list::item::getNextItem(item* current) const { return current ? current->next : nullptr; }
    auto_obj_list::item* auto_obj_list::item::getPrevItem(item* current) const { return current ? current->prev : nullptr; }

    bool auto_obj_list::isEmpty() const { return head == nullptr; }

    size_t auto_obj_list::count() const volatile
    {
        size_t cnt = 0;
        for (item* current = head; current; current = current->next)
            ++cnt;
        return cnt;
    }

    auto_obj_list::item* auto_obj_list::getHeadItem() const { return head; }

    auto_obj_list::item* auto_obj_list::getTailItem() const { return tail; }

    bool auto_obj_list::add(item* newItem) volatile
    {
        if (!newItem) return false;
        newItem->next = nullptr;
        newItem->prev = tail;
        if (tail) tail->next = newItem;
        tail = newItem;
        if (!head) head = newItem;
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
        return true;
    }

    thread_item::thread_item(thread_context& parent, size_t stack_size) : item(parent), _v_stackPointer(new (std::nothrow) volatile char[stack_size]), _stackSize(stack_size * sizeof(size_t)), _context(parent) 
    {

        if (!this->_v_stackPointer) {
            std::cerr << "Failed to allocate stack for thread item." << std::endl;
            _stackSize = 0;
        } else {
            // std::cout << ">>> Allocated stack for thread item at address: " << this->_v_stackPointer << " with size: " << stack_size << std::endl;
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
        
        // std::cout << "[DUMP] Yielding thread at address: " << (const void*)this << std::endl;
        // std::cout << "[DUMP] Stack pointer set to: " << (const void*)_stk_bottom << std::endl;
        // std::cout << "[DUMP] Virtual stack pointer: " <<(const void*)(_v_stackPointer) << std::endl;
        // std::cout << "[DUMP] Current time: " << _data.tm_now << std::endl;
        // std::cout << "[DUMP] Nice value: " << _data.nice << std::endl;
        // std::cout << "[DUMP] Last run time: " << _data.lastRunTime << std::endl;
        // std::cout << "[DUMP] State: " << static_cast<int>(_data.state) << std::endl;
        // std::cout << "[DUMP] Parent context address: " << &_context << std::endl;
        // std::cout << "[DUMP] Head item address: " << _context.getHeadItem() << std::endl;
        // std::cout << "[DUMP] Tail item address: " << _context.getTailItem() << std::endl;

        std::cout << "[DUMP] Dumping all threads in the parent context:" << std::endl;
        while (current)
        {
            std::cout << " * [DUMP] Thread at address: " << current << " State: " << static_cast<int>(current->_data.state) << " Nice: " << current->_data.nice << " Last run time: " << current->_data.lastRunTime << std::endl;
            current = static_cast<const thread_item*>(current->next);
        }
        std::cout << "[DUMP] --------------------------------" << std::endl;
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
        start:

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
                    std::cerr << "[YIELD] *** STACK OVERFLOW ***: Used stack size exceeds allocated stack size. max: " << _stackSize << " used: " << usedStackSize << ", diff:" << (usedStackSize - _stackSize) << std::endl;
                    longjmp(_context._top_context, 1);
                }

                _context.save((volatile size_t*) _context._stk_top, (volatile size_t*) _v_stackPointer, usedStackSize);
                std::cout << "[YIELD] Stack saved for thread at address: " << current << std::endl;

                if (current->_data.state == ax::State::READY)
                {
                    current->_data.state = ax::State::RUNNING;
                    current->_data.lastRunTime = getTick();
                    current->run();
                    current->_data.state = ax::State::TERMINATED;
                    goto start;

                } else if (current->_data.state == ax::State::RUNNING) {
                    // dump_data(current->_register_context, sizeof(current->_register_context), "Register Context");
                    fflush(stdout);
                    
                    //sleep untinl thread is due
                    ssize_t now = ((ssize_t)current->_data.lastRunTime + (ssize_t)current->_data.nice) - ax::getTick();                    
                    if (now > 0)
                    {
                        ax::sleepTicks(now);
                    }
                    
                    longjmp(_context._top_context, 1);
                    exit(EXIT_FAILURE);
                } else {  
                    std::cout << "[YIELD]  **** Next thread is not ready to run. ****" << std::endl;
                    longjmp(_context._top_context, 1);
                        
                    exit(EXIT_FAILURE);
                }
            } else {
                //std::cout << "[YIELD] Returning to previously saved context." << std::endl;
                _context.restore((volatile size_t*) _context._stk_top, (volatile size_t*) _context._current->_v_stackPointer, _context._current->_stk_top - _context._current->_stk_bottom);
                std::cout << "[YIELD] Context restored." << std::endl;
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

        if (setjmp(_top_context)) goto error;

        current = (thread_item*) getHeadItem();
        current->_data.state = ax::State::RUNNING;
        current->_data.lastRunTime = getTick();

        (void)current->run();

    error:

        return false;
    }

    void thread_context::save (volatile size_t* stackTop, volatile size_t* stackPointer, size_t stackSize) volatile
    {
        // Save the current stack contents to the provided stackTop location
        memcpy((void*)stackPointer, (void*)stackTop, stackSize);
        return;
    }

    void thread_context::restore (volatile size_t* stackTop, volatile size_t* stackPointer, size_t stackSize) volatile
    {
        // Restore the stack contents from the provided stackTop location
        memcpy((void*)stackTop, (void*)stackPointer, stackSize);
        return;
    }


}