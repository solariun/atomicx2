
#include "Utils.h"

#include <iostream>


namespace atomicx {

StaticList::StaticList() : m_head(nullptr), m_tail(nullptr), m_size(0) {}

void StaticList::push_back(Node& data)
{
    std::cout << "StaticList::push_back:" << &data << std::endl;

    if (m_head == nullptr) {
        m_head = &data;
        m_tail = m_head;
        data.next = nullptr;
        data.prev = nullptr;
    } else {
        m_tail->next = &data;
        data.prev = m_tail;
        data.next = nullptr;
        m_tail = m_tail->next;
    }

    m_size++;

    std::cout << "StaticList::push_back: Head(" << m_head << ") Data:[" << &data << ":" << data.prev << ":" << data.next << "] Tail: [" << m_tail << "]" << std::endl;
}

void StaticList::remove(Node& data)
{
    std::cout << "StaticList::remove():" << &data << std::endl;
    if (m_head == nullptr) {
        return;
    }

    std::cout << "StaticList::remove: Head(" << m_head << ") Data:[" << &data << ":" << data.prev << ":" << data.next << "] Tail: [" << m_tail << "]" << std::endl;

    if (m_head == &data) {
        m_head = m_head->next;
        if (m_head) m_head->prev = nullptr;
    } else if (m_tail == &data) {
        m_tail = m_tail->prev;
        if (m_tail) m_tail->next = nullptr;
    } else {
        data.prev->next = data.next;
        data.next->prev = data.prev;
    }

    m_size--;
}


} // namespace atomicx
