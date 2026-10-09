

/**
 * @file atomicx.h
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

#ifndef ATOMICX_H
#define ATOMICX_H

#include <iostream>

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

// Helper macro to define the virtual memory
// parameters automatically
#define VMEM(vmemory) vmemory[0], sizeof(vmemory) / sizeof(size_t)

namespace ax {

#define ATIMICX_SYS_CHANEL 255

    class thread;

    using Time = uint32_t;
    using RefId = size_t;

    // Those functions MUST be 
    // implemented by the user
    Time getTick();
    void sleepTicks(Time nsleep);

    enum class State
    {
        READY,
        RUNNING,
        SLEEPING,
        TERMINATED
    };

    class auto_obj_list
    {
        public:
            struct item
            {
                item() = delete;
                
                item(auto_obj_list& parent) : parent(parent)
                { 
                    parent.add(this); 
                    std::cout << "Added item to parent list at address: " << this << std::endl;
                }
                
                ~item()
                { 
                    parent.remove(this);
                    std::cout << "Removed item from parent list at address: " << this << std::endl;
                }

                item* next = nullptr;
                item* prev = nullptr;
                auto_obj_list& parent;
            };
            
            bool isEmpty() const { return head == nullptr; }
            
            size_t count() const
            {
                size_t cnt = 0;
                for (item* current = head; current; current = current->next)
                    ++cnt;
                return cnt;
            }
            
            item* getHeadItem() const { return head; }
            
            item* getTailItem() const { return tail; }
        protected:
            bool add(item* newItem)
            {
                if (!newItem) return false;
                newItem->next = nullptr;
                newItem->prev = tail;
                if (tail) tail->next = newItem;
                tail = newItem;
                if (!head) head = newItem;
                return true;
            }

            bool remove(item* itemToRemove)
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

        private:
            item* head = nullptr;
            item* tail = nullptr;
    };

    class thread_context : public auto_obj_list
    {
        public:

            bool start()
            {
                if (isEmpty()) return false;
                running = true;
                
                item* current = getHeadItem();

                return true;
            }

        protected:
            void save (volatile size_t* stackPointer, size_t stackSize) volatile
            {
                (void)stackPointer;
                (void)stackSize;
                return;
            }

            void restore (volatile size_t* stackPointer, size_t stackSize) volatile
            {
                (void)stackPointer;
                (void)stackSize;
                return;
            }

        private:
            bool running = false;
    };

    class thread_item : public auto_obj_list::item
    {
        public:
            thread_item(thread_context& parent, size_t stackSize) : item(parent), stackSize(stackSize * sizeof(size_t)) 
            {
                this->v_stackPointer = new volatile size_t[stackSize];

                if (!this->v_stackPointer) {
                    std::cerr << "Failed to allocate stack for thread item." << std::endl;
                    stackSize = 0;
                } else {
                std::cout << ">>> Allocated stack for thread item at address: " << this->v_stackPointer << " with size: " << stackSize << std::endl;
                }
            }

            ~thread_item()
            {
                delete[] v_stackPointer;
                v_stackPointer = nullptr;
            }

            virtual void run() = 0; 
             
        protected:
            volatile size_t* v_stackPointer =  nullptr; // virtual stack memory pointer
            volatile size_t* l_stackPointer =  nullptr; // local stack memory pointer

            size_t stackSize = 0;
            
            State state = State::READY;

        private:

    };

} // namespace ax   

#endif // ATOMICX_H
