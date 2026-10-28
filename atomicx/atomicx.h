

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

    using time = uint32_t;
    using ref = size_t;

    // Those functions MUST be 
    // implemented by the user
    time getTick();
    void sleepTicks(time nsleep);

    enum class State
    {
        READY,
        NOW,
        RUNNING,
        SLEEPING,
        WAITING,
        TERMINATED
    };

    class auto_obj_list
    {
        public:
            struct item
            {
                item() = delete;
                
                item(auto_obj_list& parent);
                
                ~item();

                item* getNextItem(item* current) const;
                item* getPrevItem(item* current) const;

            protected:
                friend class auto_obj_list;

                item* next = nullptr;
                item* prev = nullptr;
                volatile auto_obj_list& parent;
            };
            
            bool isEmpty() const;
            
            size_t count() const volatile;

            item* getHeadItem() const;
            
            item* getTailItem() const;

            /**
             * @brief Sorts the list in place using a bottom-up merge sort.
             *
             * Merge sort is stable, O(n log n) in all cases, needs O(1) extra
             * memory on linked lists and no recursion (safe for small MCUs).
             * No STL is used.
             *
             * @tparam Compare Callable type (functor, lambda or function pointer)
             *         supplied by the caller, invoked as `bool cmp(const item& a, const item& b)`.
             *         Must return true when @p a must come before @p b.
             * @param cmp Comparison callable. Inlined by the compiler (no virtual
             *        calls, no std::function).
             */
            template <typename Compare, typename Data>
            void sort(Compare cmp, Data data) volatile
            {
                if (!head || !head->next) return;

                size_t total = count();

                for (size_t width = 1; width < total; width <<= 1)
                {
                    item* cur = head;
                    item* newHead = nullptr;
                    item* newTail = nullptr;

                    while (cur)
                    {
                        // Split two runs of up to 'width' items
                        item* left = cur;
                        item* right = left;
                        for (size_t i = 0; i < width && right; ++i) right = right->next;
                        item* rest = right;
                        for (size_t i = 0; i < width && rest; ++i) rest = rest->next;

                        // Merge runs: left [left..right), right [right..rest)
                        item* lend = right;
                        item* rend = rest;
                        while (left != lend || right != rend)
                        {
                            item* pick;
                            if (left == lend)       { pick = right; right = right->next; }
                            else if (right == rend) { pick = left;  left  = left->next;  }
                            else if (cmp(*right, *left, data)) { pick = right; right = right->next; }
                            else                    { pick = left;  left  = left->next;  }

                            if (newTail) newTail->next = pick; else newHead = pick;
                            newTail = pick;
                        }
                        cur = rest;
                    }

                    newTail->next = nullptr;
                    head = newHead;
                }

                // Rebuild prev links and tail
                item* p = nullptr;
                for (item* c = head; c; c = c->next) { c->prev = p; p = c; }
                tail = p;
            }

            
        protected:
            bool add(item* newItem) volatile;

            bool remove(item* itemToRemove) volatile;

        private:
            item* head = nullptr;
            item* tail = nullptr;
    };

    class thread_context;

    // Thread item represents an individual thread with its own stack and execution context.
    class thread_item : public auto_obj_list::item
    {
        public:
            struct st_data
            {
                time tm_now = 0;
                time lastRunTime = 0;
                time nice = 1;
                State state = State::READY;
            };

            thread_item(thread_context& parent, size_t stack_size);

            ~thread_item();

            static bool compare(const item& a, const item& b, const thread_item* data);

            virtual void run() = 0; 

            void dump_threads() volatile;

        protected:
            void setNice(ax::time nice);
            ax::time getNice() const;
            void yield();

        private:
            friend class thread_context;

            volatile char* _v_stackPointer =  nullptr; // virtual stack memory pointer
            volatile char* _stk_bottom =  nullptr; // local stack memory pointer
            jmp_buf _register_context = {};

            volatile size_t _stackSize = 0;
            volatile char* _stk_top = nullptr; // top of the stack memory pointer

            volatile st_data _data;

            thread_context& _context;

    };

    class thread_context : public auto_obj_list
    {
        public:

            /* BY DESIGN would never return. If so the system failed to start threads*/
            bool start();

        protected:
            friend class thread_item;

            void save (volatile size_t* stackTop, volatile size_t* vStackPointer, size_t stackSize) volatile;

            void restore (volatile size_t* stackTop, volatile size_t* vStackPointer, size_t stackSize) volatile;

            thread_item* _current = nullptr;
            jmp_buf _top_context = {};
            volatile char* _stk_top = nullptr;

        private:
            bool running = false;
    };


} // namespace ax   

#endif // ATOMICX_H
