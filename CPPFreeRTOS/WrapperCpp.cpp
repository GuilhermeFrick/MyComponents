/**
 * @file WrapperCpp.cpp
 * @author Guilherme Frick de Oliveira
 * @brief
 * @version 0.1
 * @date 2022-01-14
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <new>
#include "FreeRTOS.h"
#include "task.h"

void *operator new(size_t size)
{
    void *p;

    if (uxTaskGetNumberOfTasks())
        p = pvPortMalloc(size);
    else
        p = malloc(size);

#ifdef __EXCEPTIONS
    if (p == nullptr)
        throw std::bad_alloc();
#endif

    return p;
}

void *operator new[](size_t size)
{
    void *p;

    if (uxTaskGetNumberOfTasks())
        p = pvPortMalloc(size);
    else
        p = malloc(size);

#ifdef __EXCEPTIONS
    if (p == nullptr)
        throw std::bad_alloc();
#endif

    return p;
}

void *operator new(std::size_t count, std::align_val_t alignment)
{
    void *p;
    (void)alignment;

    if (uxTaskGetNumberOfTasks())
        p = pvPortMalloc(count);
    else
        p = malloc(count);

#ifdef __EXCEPTIONS
    if (p == nullptr)
        throw std::bad_alloc();
#endif

    return p;
}

void operator delete(void *p)
{
    if (uxTaskGetNumberOfTasks())
        vPortFree(p);
    else
        free(p);

    p = NULL;
}

void operator delete[](void *p)
{

    if (uxTaskGetNumberOfTasks())
        vPortFree(p);
    else
        free(p);

    p = NULL;
}

void operator delete(void *ptr, std::align_val_t alignment)
{
    (void)alignment;

    if (uxTaskGetNumberOfTasks())
        vPortFree(ptr);
    else
        free(ptr);

    ptr = NULL;
}
