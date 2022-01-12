/**
 * @file SlidingWindow.c
 * @author Henrique Awoyama Klein
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @brief
 * @version 0.1
 * @date 2021-11-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "SlidingWindow.h"
#include <string.h>

/** @weakgroup   SlidingWindowWeak   Sliding Window Weak
 *  @ingroup  SlidingWindow
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak)) /**<Definition of weak attribute*/
#endif
__weak uint32_t SlidingWindowGetTick(void);
__weak void *   SlidingWindowMalloc(size_t size);
__weak void     SlidingWindowFree(void *ptr);

/** @}*/ // End of SlidingWindowWeak

/** \addtogroup   SlidingWindowPrivate  SlidingWindow Private
 *  \ingroup  SlidingWindow
 * @{
 */

/**
 * @brief     Sliding Window definition
 */
typedef struct SlidingWindowCtrlDef
{
    void * first_item; ///< Pointer to the first window item
    void * last_item;  ///< Pointer to the last window item*/
    void * next_item;  ///< Pointer to the next item to be inserted in the window
    size_t item_size;  ///< Size of each window item
} SlidingWindowCtrl_t;

/**
 * @brief       Creates a sliding window for a custom item and configures the default value
 * @param[in]   window: Window instance to be created
 * @param[in]   item_size: size of each element on the window
 * @param[in]   num_elements: Number of elements on the window
 * @param[in]   default_value: Default value to set the window. If NULL, sets to zero
 * @return      Result of the operation \ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowCreate(SlidingWindow_t *window, size_t item_size, size_t num_elements, void *default_value)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;

    do
    {
        if (*window != NULL)
        {
            break;
        }
        if ((item_size == 0) || (num_elements == 0))
        {
            break;
        }
        SlidingWindowCtrl_t *new_window = (SlidingWindowCtrl_t *)SlidingWindowMalloc(sizeof(SlidingWindowCtrl_t) + item_size * num_elements);

        if (new_window == NULL)
        {
            ret = SLIDING_WINDOW_ERR_MEM;
            break;
        }

        new_window->item_size  = item_size;
        new_window->first_item = new_window + 1;
        new_window->last_item  = new_window->first_item;
        while (num_elements--)
        {
            if (default_value == NULL)
            {
                memset(new_window->last_item, 0, new_window->item_size);
            }
            else
            {
                memcpy(new_window->last_item, default_value, new_window->item_size);
            }
            if (num_elements)
                new_window->last_item = (uint8_t *)new_window->last_item + new_window->item_size;
        }
        new_window->next_item = new_window->first_item;

        *window = new_window;
        ret     = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Append a new item into the window, replacing the first item
 * @param[in]   window: Target window
 * @param[in]   item: Pointer to the item
 * @return      Result of the operation \ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowAppend(SlidingWindow_t window, void *item)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;

        memcpy(this_window->next_item, item, this_window->item_size);

        this_window->next_item = (uint8_t *)this_window->next_item + this_window->item_size;

        if (this_window->next_item > this_window->last_item)
        {
            this_window->next_item = this_window->first_item;
        }
        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Retrieves the last 'n' items in the desired window
 * @param[in]   window: The target window
 * @param[in]   n: The number of items
 * @param[out]  items: Pointer to the items to be retrieved
 * @return      Result of the operation \ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowGetLastItems(SlidingWindow_t window, size_t n, void *items)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window   = window;
        void *               previous_item = (uint8_t *)this_window->next_item - this_window->item_size;

        while (n--)
        {
            if (previous_item < this_window->first_item)
            {
                previous_item = this_window->last_item;
            }
            memcpy(items, previous_item, this_window->item_size);
            items         = (uint8_t *)items + this_window->item_size;
            previous_item = (uint8_t *)previous_item - this_window->item_size;
        }

        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Retrieves the float average of the last "n" items
 * @param[in]   window: The target window
 * @param[in]   n: The number of items (or filter order)
 * @param[out]  avg: resulting average
 * @return      Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowGetFloatAvg(SlidingWindow_t window, size_t n, float *const avg)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        if (avg == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;
        if (this_window->item_size != sizeof(float))
        {
            break;
        }
        float filter_order  = n;
        void *previous_item = (uint8_t *)this_window->next_item - this_window->item_size;
        if (previous_item < this_window->first_item)
        {
            previous_item = this_window->last_item;
        }
        *avg = 0.0f;
        while (n--)
        {
            int32_t temp_value;
            memcpy(&temp_value, previous_item, sizeof(float));
            *avg += (float)temp_value / filter_order;
            previous_item = (uint8_t *)previous_item - this_window->item_size;
            if (previous_item < this_window->first_item)
            {
                previous_item = this_window->last_item;
            }
        }
        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}
/**
 * @brief   Checks whether the desired window is cleared(zero-filled)
 *
 * @param window: The target window
 * @param win_size: The total number of items in the window
 * @param is_empty: Result "true" if window is cleared
 * @return Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowIsCleared(SlidingWindow_t window, bool *is_empty)
{
    SlidingWindowRet_et  ret            = SLIDING_WINDOW_ERR_INV_PARAM;
    SlidingWindowCtrl_t *this_window    = window;
    uint8_t *            buffer_cleared = NULL;
    size_t               win_size       = (size_t)this_window->last_item - (size_t)this_window->first_item + this_window->item_size;
    do
    {
        if (window == NULL)
        {
            break;
        }
        buffer_cleared = SlidingWindowMalloc(this_window->item_size);
        if (buffer_cleared == NULL)
        {
            ret = SLIDING_WINDOW_ERR_MEM;
            break;
        }

        memset(buffer_cleared, 0, this_window->item_size);

        void *curr_item = (uint8_t *)this_window->first_item;

        *is_empty = true;

        for (size_t i = 0; i < win_size; i += this_window->item_size)
        {
            if (memcmp(buffer_cleared, curr_item, this_window->item_size) != 0)
            {
                *is_empty = false;
                break;
            }
            curr_item = (uint8_t *)curr_item + this_window->item_size;
        }

        ret = SLIDING_WINDOW_OK;
    } while (0);

    SlidingWindowFree(buffer_cleared);

    return ret;
}

/**
 * @brief Retrieves the item in the tail position of the desired window
 * @param window: The target window
 * @param item :Pointer to the Tail item to be retrieved
 * @return Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowGetTail(SlidingWindow_t window, void *item)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }

        if (item == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;

        memcpy(item, this_window->next_item, this_window->item_size);

        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}
/**
 * @brief   Retrieves the item in the head position of the desired window
 * @param window: The target window
 * @param item :Pointer to the Head item to be retrieved
 * @return Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowGetHead(SlidingWindow_t window, void *item)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        if (item == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window   = window;
        void *               previous_item = (uint8_t *)this_window->next_item - this_window->item_size;
        if (previous_item < this_window->first_item)
        {
            previous_item = this_window->last_item;
        }
        memcpy(item, previous_item, this_window->item_size);

        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}
/**
 * @brief
 *
 * @param window: The target window
 * @param n :
 * @param items
 * @return Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowGetItem(SlidingWindow_t window, size_t n, void *items)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        if (items == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;
        void *               curr_item   = (uint8_t *)this_window->next_item;
        while (n--)
        {
            curr_item = curr_item + this_window->item_size;

            if (curr_item > this_window->last_item)
            {
                curr_item = this_window->first_item;
            }
        }
        memcpy(items, curr_item, this_window->item_size);

        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}
/**
 * @brief
 *
 * @param window
 * @param win_size
 * @return Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowReset(SlidingWindow_t window)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;
        void *               curr_item   = (uint8_t *)this_window->first_item;

        do
        {
            memset(curr_item, 0, this_window->item_size);
            curr_item = (uint8_t *)curr_item + this_window->item_size;
        } while (curr_item <= this_window->last_item);

        ret = SLIDING_WINDOW_OK;
    } while (0);

    return ret;
}

/**
 * @brief         Deletes a previously created window
 * @param[in,out] window: The pointer to the window
 * @return        Result of the operation @ref SlidingWindowRet_et
 */
SlidingWindowRet_et SlidingWindowDelete(SlidingWindow_t *window)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;

    if (*window != NULL)
    {
        SlidingWindowFree(*window);
        *window = NULL;
        ret     = SLIDING_WINDOW_OK;
    }
    return ret;
}

SlidingWindowRet_et SlidingWindowGetWinSize(SlidingWindow_t window, size_t *window_size)
{
    SlidingWindowRet_et ret = SLIDING_WINDOW_ERR_INV_PARAM;

    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;

        *window_size = ((size_t)(this_window->last_item - this_window->first_item) / this_window->item_size) + 1;

        ret = SLIDING_WINDOW_OK;

    } while (0);

    return ret;
}
/**
 * @brief       Weakly defined memory allocation function
 * @param[in]   size: Size in bytes of the desired area
 * @return      A pointer to the allocated area or NULL if it fails
 */
__weak void *SlidingWindowMalloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief       Weakly defined memory deallocation function
 * @param[out]  ptr: Buffer to be freed
 */
__weak void SlidingWindowFree(void *ptr)
{
    free(ptr);
}
