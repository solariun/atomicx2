

/**
 * @file Utils.h
 * @brief This file contains utility functions and global variables/constants for the AtomicX project.
 */

#ifndef ATOMICX_UTILS_H
#define ATOMICX_UTILS_H

#include <iostream>
#include <cstdint>

namespace atomicx {

    // Define an iterator for the StaticList
    template <typename T>
    class Iterator 
    {
        T* node;
    public:
        Iterator() = delete;
        Iterator(T* node) : node(node) {}
        T* operator*() { return node; }
        bool operator!=(const Iterator<T>& other) { return node != other.node; }
        void operator++() { node = node->next; }
    };

    class Node
    {
        friend class StaticList;
        template <typename U> friend class Iterator;
    public:
        Node() : next(nullptr), prev(nullptr) {}
    //private:
        Node* next;
        Node* prev;
    };

    class StaticList
    {
    public:
        StaticList();

        void push_back(Node& data);
        void remove(Node& data);
        
        Iterator<Node> begin() { return Iterator<Node>(m_head); }
        Iterator<Node> end() { return Iterator<Node>(nullptr); }

    protected:

    private:
        Node* m_head;
        Node* m_tail;
        uint32_t m_size;
    };


}; // namespace atomicx
#endif // ATOMICX_UTILS_H
