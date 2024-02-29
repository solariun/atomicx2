#ifndef ATOMICX_H
#define ATOMICX_H

#include "Utils.h"

#include <setjmp.h>

#include <iostream>
#include <cstdint>

namespace atomicx {

    class thread;

    enum class state {
        READY,
        RUNNING,
        SLEEPING,
        STOPPED,
        WAIT,
        LOCKED
    };

    /**
     * @brief The context class
    */
    class context {
    public:
        /**
         * @brief context
         */
        virtual uint32_t ticks();
        /** 
         * @brief sleep
         * @param ms    Sleep time in milliseconds
         * @return      void
         * @note        Sleep for the specified time in milliseconds
         *              This is a blocking call
        */
        virtual void sleep(uint32_t ms);

        /**
         * @brief start thread execution
         * @return  void
         * @note    This function block while the threads are running
        */
        void start();

        /**
         * @brief current_thread
         * @return  thread*
         * @note    Return the current thread
        */
        thread* current_thread();

    protected:

        /**
         * @brief scheduler
         * @return  thread*
         * @note    Return the next thread to be executed
        */
        thread* scheduler();

        /**
         * @brief save_context
         * @param sp        Stack pointer
         * @param vstack    Virtual stack pointer
         * @param size      Size of the stack
         * @return          void
         * @note            Save the current context
        */
        void save_context(size_t* &vstack, uint8_t* &sp, size_t size);

        /**
         * @brief restore_context
         * @param vstack    Virtual stack pointer
         * @param sp        Stack pointer
         * @param size      Size of the stack
         * @return          void
         * @note            Restore the context
        */
        void restore_context(uint8_t* &sp, size_t* &vstack, size_t size); 

    private:
        friend class thread;

        // static thread list
        static list<thread> m_threads;

        // Self initialized
        thread* m_current_thread{nullptr};
        jmp_buf m_jmp{};
        uint8_t* m_sp{nullptr};
    };

    // Global standard context
    extern context g_ctx;

    class thread : public node {
    public:
        thread() = delete;

        /**
         * @brief thread
         * @param stack     Stack pointer
         * @param N         Stack size
         * @note            Constructor for the thread class
         *                  This constructor is used to initialize 
         *                  the stack pointer and stack size
        */
        template <std::size_t N>
        thread(std::size_t (&stack)[N]) : m_stack(stack), m_stack_max_size(N) {
            g_ctx.m_threads.push_back(*this);
            (void) m_stack;
            (void) m_stack_max_size;
        }

        ~thread();

        /**
         * @brief run
         * @return  bool
         * @note    Pure virtual function to be implemented by the derived class
         *          This function is called when the thread is started
        */
        virtual bool run() = 0;

        /**
         * @brief begin
         * @return  Iterator<node>
         * @note    Return the begin iterator for the thread list
        */
        Iterator<thread> begin()
        {
            return Iterator<thread>(g_ctx.m_threads.begin());
        }

        /**
         * @brief end
         * @return  Iterator<node>
         * @note    Return the end iterator for the thread list
        */
        Iterator<thread> end()
        {
            return Iterator<thread>(nullptr);
        }

        bool yield(uint32_t ms = 0);

        virtual const char* name() = 0;
    private:
        friend class context;

        // Initialized on construction
        std::size_t* m_stack;
        std::size_t m_stack_max_size;

        // Self-initialized
        uint8_t* m_sp{nullptr};
        jmp_buf m_jmp{};
        state m_state{state::READY};
        size_t m_stack_used{0};
    };

}; // namespace atomicx
#endif // ATOMICX_H
