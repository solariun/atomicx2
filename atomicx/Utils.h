

/**
 * @file Utils.h
 * @brief This file contains utility functions and global variables/constants for the AtomicX project.
 */

#ifndef ATOMICX_UTILS_H
#define ATOMICX_UTILS_H

#include <iostream>
#include <cstdint>

namespace atomicx {

    // Define an iterator for the list
    template <typename T>
    class Iterator 
    {
        // Define the list class
        T* node;
    public:
        friend class node;

        Iterator() = delete;
        Iterator(T* node) : node(node) {}
        T* operator*() { return node; }
        bool operator!=(const Iterator<T>& other) { return node != other.node; }
        void operator++() { node = (T*) node->next; }
    };

    // Define the node class
    class node
    {
        // Define the list class as a friend
        template<typename T> friend class list;
        // Define the iterator class as a friend
        template <typename U> friend class Iterator;
    public:
        // Define the node class
        node() : next(nullptr), prev(nullptr) {}


    //private:
        node* next;
        node* prev;
    };

    // Define the list class
    template <typename T>
    class list
    {
    public:
        list() : m_head(nullptr), m_tail(nullptr), m_size(0) {}

        /**
         * @brief push_back
         * @param data  Data to be added to the list
         * @return      void
         * @note        Add data to the end of the list
         */
        void push_back(node& data)
        {
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
        }

        /**
         * @brief remove
         * @param data  Data to be removed from the list
         * @return      void
         * @note        Remove data from the list
         */
        void remove(node& data)
        {
            if (m_head == nullptr) {
                return;
            }

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
        
        /**
         * @brief count
         * @return      size_t
         * @note        Return the size of the list
         */
        size_t count() { return m_size; }

        /**
         * @brief begin
         * @return      Iterator<node>
         * @note        Return the beginning of the list
         */
        Iterator<T> begin()
        {
            return Iterator<T>((T*) m_head);
        }

        /**
         * @brief end
         * @return      Iterator<node>
         * @note        Return the end of the list
         */
        Iterator<T> end()
        {
            return Iterator<T>((T) m_tail);
        }

        T* head() { return (T*) m_head; }

        T* tail() { return (T*) m_tail; }

    protected:

    private:
        node* m_head; // Head of the list
        node* m_tail; // Tail of the list
        size_t m_size; // Size of the list
    };


}; // namespace atomicx
#endif // ATOMICX_UTILS_H
