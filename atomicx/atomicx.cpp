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

namespace ax {

    // Ticks wrap around: a time is due while now is less than half the tick range past it
    static const time k_tick_half_range = (time)(((time)~(time)0 >> 1) + 1);

    auto_obj_list::item::item(auto_obj_list& parent) : parent(parent)
    { 
        parent.add(this); 
        std::cout << "Added item to parent list at address: " << this << std::endl;
    }

    auto_obj_list::item::~item()
    { 
        parent.remove(this);
        std::cout << "Removed item from parent list at address: " << this << std::endl;
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

    thread_item::thread_item(thread_context& parent, size_t stack_size) : item(parent), _v_stackPointer(new (std::nothrow) volatile char[stack_size]), stackSize(stack_size * sizeof(size_t)), _context(parent) 
    {

        if (!this->_v_stackPointer) {
            std::cerr << "Failed to allocate stack for thread item." << std::endl;
            stackSize = 0;
        } else {
            std::cout << ">>> Allocated stack for thread item at address: " << this->_v_stackPointer << " with size: " << stack_size << std::endl;
        }
    }

    thread_item::~thread_item()
    {
        delete[] _v_stackPointer;
        _v_stackPointer = nullptr;
    }

    bool thread_item::compare(const item& a, const item& b, const thread_item* data)
    {
        const thread_item& first = static_cast<const thread_item&>(a);
        const thread_item& second = static_cast<const thread_item&>(b);

        bool first_ready = first._data.state == State::READY || first._data.state == State::NOW;
        bool second_ready = second._data.state == State::READY || second._data.state == State::NOW;

        if (first_ready || second_ready) return true;

        time now = data->_data.tm_now;

        bool first_due = (time)(now - (first._data.lastRunTime + first._data.nice)) < k_tick_half_range;
        bool second_due = (time)(now - (second._data.lastRunTime + second._data.nice)) < k_tick_half_range;

        return first_due && !second_due;
    }

    void thread_item::dump_threads() volatile
    {   
        const thread_item* current = static_cast<const thread_item*>(_context.getHeadItem());
        
        std::cout << "[DUMP] Yielding thread at address: " << (const void*)this << std::endl;
        std::cout << "[DUMP] Stack pointer set to: " << (const void*)_stk_bottom << std::endl;
        std::cout << "[DUMP] Virtual stack pointer: " <<(const void*)(_v_stackPointer) << std::endl;
        std::cout << "[DUMP] Current time: " << _data.tm_now << std::endl;
        std::cout << "[DUMP] Nice value: " << _data.nice << std::endl;
        std::cout << "[DUMP] Last run time: " << _data.lastRunTime << std::endl;
        std::cout << "[DUMP] State: " << static_cast<int>(_data.state) << std::endl;
        std::cout << "[DUMP] Parent context address: " << &_context << std::endl;
        std::cout << "[DUMP] Head item address: " << _context.getHeadItem() << std::endl;
        std::cout << "[DUMP] Tail item address: " << _context.getTailItem() << std::endl;
        std::cout << "[DUMP] Dumping all threads in the parent context..." << std::endl;
        while (current)
        {
            std::cout << "[DUMP] Thread at address: " << current << " State: " << static_cast<int>(current->_data.state) << std::endl;
            current = static_cast<const thread_item*>(current->next);
        }
    }

    void thread_item::yield()
    {
        _data.tm_now = getTick();
        std::cout << "[YIELD] Dumping threads before sort..." << std::endl;
        dump_threads();

        // Implementation of yield can be added here
        std::cout << "[YIELD] Sorting threads..." << std::endl;
        _context.sort(compare, this);
        std::cout << "[YIELD] Threads sorted." << std::endl;
        
        std::cout << "[YIELD] Dumping threads after sort..." << std::endl;
        dump_threads();

        auto next = static_cast<thread_item*>(_context.getHeadItem());
        if (next != nullptr)
        {
            auto* current = _context._current = next;

            std::cout << "[YIELD] Next thread to run at address: " << current << std::endl;
            std::cout << "[YIELD] Next thread state: " << static_cast<int>(current->_data.state) << std::endl;
            std::cout << "[YIELD] Next thread last run time: " << current->_data.lastRunTime << std::endl;
            std::cout << "[YIELD] Next thread stack pointer: " << current->_stk_bottom << std::endl;
            std::cout << "[YIELD] Next thread context address: " << &(current->_context) << std::endl;
            std::cout << "[YIELD] Next thread next item address: " << current->next << std::endl;
            std::cout << "[YIELD] Next thread previous item address: " << current->prev << std::endl;
            std::cout << "[YIELD] Next thread is about to be checked for readiness." << std::endl;

            if (setjmp(current->_register_context) == 0)
            {
                volatile char dummy = 0xAA;
                current->_stk_bottom =  &dummy;
                size_t usedStackSize = _context._stk_top - current->_stk_bottom;

                std::cout << "[YIELD] >>> Used stack size: " << usedStackSize << std::endl;

                _context.save((volatile size_t*) _context._stk_top, (volatile size_t*) _v_stackPointer, usedStackSize);

                if (current->_data.state == ax::State::READY)
                {
                    current->_data.state = ax::State::RUNNING;
                    current->_data.lastRunTime = getTick();
                    current->run();
                }else{  
                    std::cout << "[YIELD]  **** Next thread is not ready to run. ****" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

        return;
    }

    bool thread_context::start()
    {
        if (isEmpty() || running) return false;
        
        running = true;
        
        thread_item* current = (thread_item*) getHeadItem();

        current->_data.state = ax::State::RUNNING;

        volatile char dummy = 0xBB;
        _stk_top = &dummy;
        
        (void)current->run();

        return true;
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