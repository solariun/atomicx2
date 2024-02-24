#ifndef ATOMICX_H
#define ATOMICX_H

#include "Utils.h"

#include <iostream>
#include <cstdint>

namespace atomicx {

    class thread : public Node {
    public:
        thread() = delete;

        template <std::size_t N>
        thread(std::size_t (&stack)[N]) : m_stack(stack), m_stack_max_size(N) {
            m_threads.push_back(*this);
            std::cout << "thread::thread():" << this << std::endl;

            (void) m_stack;
            (void) m_stack_max_size;
        }

        ~thread();

        virtual bool run() = 0;

        Iterator<Node> begin();

        Iterator<Node> end();

    private:
        static StaticList m_threads;
        // Initialized on construction
        std::size_t* m_stack;
        std::size_t m_stack_max_size;

        // Self-initialized 
    };

}; // namespace atomicx
#endif // ATOMICX_H
