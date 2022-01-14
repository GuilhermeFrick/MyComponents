/**
 * @file        SlidingWindow.hpp
 * @brief       Class for a c++ sliding window implementation
 * @date        2021-11-12
 * @version     1.0
 * @author      Henrique Awoyama Klein (henrique.klein@perto.com.br)
 * @author      Luiz Guilherme Rizzatto Zucchi (luiz.zucchi@perto.com.br)
 * @copyright   Copyright (c) 2021
 */
#ifndef _SLIDING_WINDOW_HPP_
#define _SLIDING_WINDOW_HPP_
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief       Generic Sliding Window Class
 * @tparam      T: type of the item
 */
template <typename T>
class SlidingWindow
{
  public:
    /**
     * @brief       Construct a new Sliding Window object
     * @param[in]   num_items: Maximum number of items
     * @param[in]   default_val: default value to fill the window
     */
    SlidingWindow(const size_t num_items, const T &default_val);
    /**
     * @brief       Destroy the Sliding Window object
     */
    ~SlidingWindow();
    /**
     * @brief       Returns the head of the window
     * @return      Head of the window
     */
    T &head();
    /**
     * @brief       Returns the tail of the window
     * @return      Tail of the window
     */
    T &tail();
    /**
     * @brief       Returns the size of the window
     * @return      Result of the operation @ref size_t
     */
    size_t size();
    /**
     * @brief       Appends a new item, removing the oldest (tail)
     * @param[in]   item: new item to be appended
     */
    bool append(const T &item);
    /**
     * @brief       Get the item indexed by idx
     * @param[in]   idx: Desired index of the item
     * @return      Result of the operation @ref T&
     */
    T &at(const size_t idx) const;
    /**
     * @brief       Retrieves a number of items starting from the tail
     * @param[in]   num_items: Number of items
     * @param[out]  arr: Output array
     * @return      Booleand indicating if the items were retrieved
     */
    bool GetItems(const size_t num_items, T arr[]);

  private:
    T     *begin   = nullptr;
    T     *end     = nullptr;
    T     *current = nullptr;
    size_t n_items = 0;
};

template <typename T>
SlidingWindow<T>::SlidingWindow(const size_t num_items, const T &default_val)
{
    this->begin = new T[num_items];
    if (begin != nullptr)
    {
        this->n_items = num_items;
        this->end     = this->begin + this->n_items;
        for (this->current = this->begin; current < this->end; this->current++)
        {
            memcpy(this->current, &default_val, sizeof(T));
        }
        this->current = this->begin;
    }
}

template <typename T>
SlidingWindow<T>::~SlidingWindow()
{
    delete[] this->begin;
}
template <typename T>
T &SlidingWindow<T>::head()
{
    return *this->current;
}
template <typename T>
T &SlidingWindow<T>::tail()
{
    if (this->current == this->end - 1)
    {
        return *(this->begin);
    }
    else
    {
        return *(this->current + 1);
    }
}
template <typename T>
size_t SlidingWindow<T>::size()
{
    return n_items;
}
template <typename T>
bool SlidingWindow<T>::append(const T &item)
{
    bool ret = false;
    if (n_items)
    {
        this->current++;
        if (this->current == this->end)
        {
            this->current = this->begin;
        }
        memcpy(this->current, &item, sizeof(T));
        ret = true;
    }

    return ret;
}
template <typename T>
T &SlidingWindow<T>::at(const size_t idx) const
{
    T *t = this->current - idx;
    if (t < this->begin)
    {
        t += this->n_items;
    }
    return *t;
}
template <typename T>
bool SlidingWindow<T>::GetItems(const size_t num_items, T arr[])
{
    bool ret = false;
    if ((num_items <= this->n_items) && (arr != nullptr))
    {
        T *t = this->current;
        for (size_t i = 0; i < num_items; i++)
        {
            memcpy(arr++, t, sizeof(T));
            t--;
            if (t < begin)
            {
                t = end - 1;
            }
        }
        ret = true;
    }

    return ret;
}

#endif
