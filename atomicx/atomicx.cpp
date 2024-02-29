#include "atomicx.h"

#include <iostream>
#include <memory>

namespace atomicx {

    // static list context::m_threads{};
    list<thread> context::m_threads{}; 

    // Global standard context
    context g_ctx{};

    /*
     * Static / weak functions
    */



    /*
     * ITERATOR CLASS METHODS
    */


    /*
     * CONTEXT CLASS METHODS
    */

    uint32_t context::ticks()
    {
        return 0;
    }

    void context::start()
    {
        m_current_thread = m_threads.head();

        uint8_t sp = 0xAA;
        m_sp = &sp; // save the stack pointer

        while (m_threads.count() > 0 && m_current_thread != nullptr) {
            if (m_current_thread->run()) {
                uint8_t sp = 0x10;
                m_sp = &sp; // save the stack pointer

                if (m_current_thread->m_state == state::READY) {
                    if (setjmp(m_jmp) == 0) {
                        m_current_thread->m_state = state::RUNNING;
                        m_current_thread->run();
                        m_current_thread->m_state = state::STOPPED;
                    }
                }
            }
            m_current_thread = scheduler();
        }
    }

    void context::save_context(size_t* &vstack, uint8_t* &sp, size_t size)
    {
        memccpy(vstack, sp, size, sizeof(size_t));
    }

    void context::restore_context(uint8_t* &sp, size_t* &vstack, size_t size)
    {
        memccpy(sp, vstack, size, sizeof(size_t));
    }


    /*
    * THREAD CLASS METHODS
    */

    thread* context::current_thread() {
            return m_current_thread;
    }

    thread* context::scheduler() {
        return (thread*) m_current_thread->next;
    }

    void context::sleep(uint32_t ms)
    {
        (void) ms;
    }

    /*
     * THREAD CLASS METHODS
    */

    thread::~thread()
    {
        g_ctx.m_threads.remove(*this);
    }

    bool thread::yield(uint32_t ms)
    {
        (void) ms;
        {
            uint8_t sp = 0xFF;
            std::cout << "m_current: " << g_ctx.m_current_thread << std::endl << std::flush;

            g_ctx.m_current_thread->m_sp = &sp; // save the stack pointer

            // Since we are dealing with a Stack, the size if calculated reversevly
            g_ctx.m_current_thread->m_stack_used =  (size_t) (g_ctx.m_sp - g_ctx.m_current_thread->m_sp);

            std::cout << "stack used: " << g_ctx.m_current_thread->m_stack_used << std::endl << std::flush;
            
            g_ctx.m_current_thread->m_state = state::SLEEPING;

            g_ctx.save_context(g_ctx.m_current_thread->m_stack, g_ctx.m_current_thread->m_sp, g_ctx.m_current_thread->m_stack_used);

            if (setjmp(g_ctx.m_current_thread->m_jmp) == 0) {
                longjmp(g_ctx.m_jmp, 1);
            }
            
            g_ctx.restore_context(g_ctx.m_current_thread->m_sp, g_ctx.m_current_thread->m_stack, g_ctx.m_current_thread->m_stack_used);

            g_ctx.m_current_thread->m_state = state::RUNNING;

            return true;
        }
    }

}; // namespace atomicx