#include "atomicx.h"

#include <iostream>
#include <memory>

namespace atomicx {

    StaticList thread::m_threads{};

    thread::~thread() {
        m_threads.remove(*this);
        std::cout << "thread::~thread():" << this << std::endl;
    }

    Iterator<Node> thread::begin() {
        return Iterator<Node>(m_threads.begin());
    }

    Iterator<Node> thread::end() {
        return Iterator<Node>(m_threads.end());
    }

}; // namespace atomicx