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

    size_t auto_obj_list::count() const
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

    thread_item::thread_item(thread_context& parent, size_t stack_size) : item(parent), _v_stackPointer(new (std::nothrow) volatile size_t[stack_size]), stackSize(stack_size * sizeof(size_t)), _parent(parent) 
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

    void thread_item::yield()
    {
        // Implementation of yield can be added here
        return;
    }

    bool thread_context::start()
    {
        if (isEmpty() || running) return false;
        
        running = true;
        
        thread_item* current = (thread_item*) getHeadItem();

        (void)current->run();

        return true;
    }

    void thread_context::save (volatile size_t* stackPointer, size_t stackSize) volatile
    {
        (void)stackPointer;
        (void)stackSize;
        return;
    }

    void thread_context::restore (volatile size_t* stackPointer, size_t stackSize) volatile
    {
        (void)stackPointer;
        (void)stackSize;
        return;
    }


}