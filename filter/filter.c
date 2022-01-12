/**
 * @file        filter.c
 * @brief       Source file with basic filter and PID regulator functions
 * @date        2020-10-14
 * @version     1.0
 * @author      Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @copyright   Copyright (c) 2020
 */
#include "filter.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/** @weakgroup   FilterWeak  Filter Weak
 *  @ingroup Filter
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak)) /**<Definition of weak attribute*/
#endif
__weak uint32_t FilterGetTick(void);
__weak void    *FilterMalloc(size_t size);
__weak void     FilterFree(void *ptr);
/** @}*/ // End of FilterWeak

/** \addtogroup  FilterPrivate Filter Private
 *  \ingroup Filter
 * @{
 */

/*!
 * @brief     Circular buffer definition
 */
typedef struct CircBuffCtrlDef
{
    struct CircBuffCtrlDef *self;       ///< Pointer to self instance
    void                   *first_item; ///< Pointer to first item of circular buffer
    void                   *last_item;  ///< Pointer to last item of circular buffer
    void                   *next_item;  ///< Pointer to the next item to be inserted in the circular buffer
    size_t                  item_size;  ///< Size of each buffer item
} CircBuffCtrl_t;
/**
 * @brief     Sliding Window definition
 */
typedef struct SlidingWindowCtrlDef
{
    void  *first_item; ///< Pointer to the first window item
    void  *last_item;  ///< Pointer to the last window item*/
    void  *next_item;  ///< Pointer to the next item to be inserted in the window
    size_t item_size;  ///< Size of each window item
} SlidingWindowCtrl_t;

static uint32_t FilterGetElapsedTime(uint32_t InitialTime);
/** @}*/ // End of FilterPrivate

/**
 *  @brief      Function with proportional integral derivative regulator
 *  @param[in]  setpoint: PID regulator setpoint
 *  @param[in]  config: pointer to a @ref PID32Config_t structure with filter configuration
 *  @param[out] data: pointer to a @ref PID32Data_t structure with output process variable and filter data
 *  @warning    all data must be 0 initialized
 */
void PID32Regulator(int32_t setpoint, const PID32Config_t *config, PID32Data_t *data)
{
    float aux_process_var = (float)(data->process_var);

    data->last_error = data->error;
    data->error      = (float)(setpoint - data->process_var);

    if (config->sat > 0.0F)
        if (data->error > config->sat)
            data->error = config->sat;

    // proportional
    aux_process_var += config->Kp * data->error;

    // integral
    data->integral += data->error;
    aux_process_var += config->Ki * data->integral;

    // derivative
    aux_process_var += config->Kd * (data->error - data->last_error);

    if (aux_process_var > (int32_t)config->max)
        data->process_var = config->max;
    else if (aux_process_var < (int32_t)config->min)
        data->process_var = config->min;
    else
        data->process_var = (int32_t)aux_process_var;
}
/**
 *  @brief      Function with simplified proportional regulator
 *  @param[in]  setpoint: P regulator setpoint
 *  @param[in]  Kp: proportional constant
 *  @param[out] process_var: output process variable
 */
void P32Regulator(int32_t setpoint, float Kp, int32_t *process_var)
{
    float aux_process_var = (float)*process_var;

    aux_process_var += Kp * ((float)setpoint - aux_process_var);

    *process_var = (int32_t)aux_process_var;
}

/**
 *  @brief      Function with simplified proportional regulator to float values
 *  @param[in]  setpoint: P regulator setpoint
 *  @param[in]  Kp: proportional constant
 *  @param[out] process_var: output process variable
 */
void PfRegulator(float setpoint, float Kp, float *process_var)
{
    *process_var += Kp * ((float)setpoint - *process_var);
}
/**
 *  @brief          Function that debounce the sensors
 *  @param[in,out]  control: [in]  pointer to structure with status pin and debounce configuration
 *                           [out] pointer to structure to receive debounced sensor status
 */
void SensorDebounce(DebounceControl_t *control)
{
    switch (control->state)
    {
        case UNDEFINED_STATE:
            if (control->status_pin == SENSOR_CLEARED)
                control->state = FALLING_STATE;
            else if (control->status_pin == SENSOR_SET)
                control->state = RISING_STATE;
            control->timestamp = FilterGetTick();
            break;
        case RISING_STATE:
            if (FilterGetElapsedTime(control->timestamp) > control->trigger_high)
            {
                control->status = SENSOR_SET;
                control->state  = HIGH_STATE;
            }
            if (control->status_pin == SENSOR_CLEARED)
                control->state = UNDEFINED_STATE;
            break;
        case FALLING_STATE:
            if (FilterGetElapsedTime(control->timestamp) > control->trigger_low)
            {
                control->status = SENSOR_CLEARED;
                control->state  = LOW_STATE;
            }
            if (control->status_pin == SENSOR_SET)
                control->state = UNDEFINED_STATE;
            break;
        case HIGH_STATE:
            if (control->status_pin == SENSOR_CLEARED)
                control->state = UNDEFINED_STATE;
            else
                control->status = SENSOR_SET;
            break;
        case LOW_STATE:
            if (control->status_pin == SENSOR_SET)
                control->state = UNDEFINED_STATE;
            else
                control->status = SENSOR_CLEARED;
            break;
        default:
            control->state = UNDEFINED_STATE;
            break;
    }
}

/**
 * @brief       Creates a circular buffer instance with a custom item
 * @param[out]  circ_buff: Circular buff instance
 * @param[in]   item_size: Size of the item in bytes
 * @param[in]   num_elements: Number of elements
 * @return      Result of the operation \ref FilterRet_e
 */
FilterRet_e FilterCreateCircBuff(CircBuff_t *circ_buff, size_t item_size, size_t num_elements)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;

    do
    {
        if (*circ_buff != NULL)
        {
            break;
        }
        CircBuffCtrl_t *new_buff = (CircBuffCtrl_t *)FilterMalloc(sizeof(CircBuffCtrl_t) + item_size * num_elements);

        if (new_buff == NULL)
        {
            ret = FILTER_RET_ERR_MEM;
            break;
        }

        new_buff->self       = new_buff;
        new_buff->item_size  = item_size;
        new_buff->first_item = new_buff + 1;
        new_buff->last_item  = new_buff->first_item;
        new_buff->next_item  = new_buff->first_item;

        *circ_buff = new_buff;
        ret        = FILTER_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Creates a sliding window for a custom item and configures the default value
 * @param[out]  window: Window instance to be created
 * @param[in]   item_size: size of each element on the window
 * @param[in]   num_elements: Number of elements on the window
 * @param[in]   default_value: Default value to set the window. If NULL, sets to zero
 * @return      Result of the operation \ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowCreate(SlidingWindow_t *window, size_t item_size, size_t num_elements, void *default_value)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;

    do
    {
        if (*window != NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *new_window = (SlidingWindowCtrl_t *)FilterMalloc(sizeof(SlidingWindowCtrl_t) + item_size * num_elements);

        if (new_window == NULL)
        {
            ret = FILTER_RET_ERR_MEM;
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
        ret     = FILTER_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Append a new item into the window, replacing the first item
 * @param[in]   window: Target window
 * @param[in]   item: Pointer to the item
 * @return      Result of the operation \ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowAppend(SlidingWindow_t window, void *item)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
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
        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Retrieves the last 'n' items in the desired window
 * @param[in]   window: The target window
 * @param[in]   n: The number of items
 * @param[out]  items: Pointer to the items to be retrieved
 * @return      Result of the operation \ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowGetLastItems(SlidingWindow_t window, size_t n, void *items)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window   = window;
        void                *previous_item = (uint8_t *)this_window->next_item - this_window->item_size;

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

        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Retrieves the float average of the last "n" items
 * @param[in]   window: The target window
 * @param[in]   n: The number of items (or filter order)
 * @param[out]  avg: resulting average
 * @return      Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowGetFloatAvg(SlidingWindow_t window, size_t n, float *const avg)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
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
        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}
/**
 * @brief       Checks whether the desired window is cleared(zero-filled)
 * @param[in]   window: The target window
 * @param[in]   win_size: The total number of items in the window
 * @param[out]  is_empty: Result "true" if window is cleared
 * @return      Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowIsCleared(SlidingWindow_t window, bool *is_empty)
{
    FilterRet_e          ret            = FILTER_RET_ERR_INV_PARAM;
    SlidingWindowCtrl_t *this_window    = window;
    uint8_t             *buffer_cleared = NULL;
    size_t               win_size       = (size_t)this_window->last_item - (size_t)this_window->first_item + this_window->item_size;
    do
    {
        if (window == NULL)
        {
            break;
        }
        buffer_cleared = FilterMalloc(this_window->item_size);
        if (buffer_cleared == NULL)
        {
            ret = FILTER_RET_ERR_MEM;
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

        ret = FILTER_RET_OK;
    } while (0);

    FilterFree(buffer_cleared);

    return ret;
}
/**
 * @brief Retrieves the item in the tail position of the desired window
 * @param window: The target window
 * @param item :Pointer to the Tail item to be retrieved
 * @return Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowGetTail(SlidingWindow_t window, void *item)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
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

        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}
/**
 * @brief   Retrieves the item in the head position of the desired window
 * @param window: The target window
 * @param item :Pointer to the Head item to be retrieved
 * @return Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowGetHead(SlidingWindow_t window, void *item)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
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
        void                *previous_item = (uint8_t *)this_window->next_item - this_window->item_size;
        if (previous_item < this_window->first_item)
        {
            previous_item = this_window->last_item;
        }
        memcpy(item, previous_item, this_window->item_size);

        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}
/**
 * @brief
 *
 * @param window: The target window
 * @param n :
 * @param items
 * @return Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowGetItem(SlidingWindow_t window, size_t n, void *items)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
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
        void                *curr_item   = (uint8_t *)this_window->next_item;
        while (n--)
        {
            curr_item = (uint8_t *)curr_item + this_window->item_size;

            if (curr_item > this_window->last_item)
            {
                curr_item = this_window->first_item;
            }
        }
        memcpy(items, curr_item, this_window->item_size);

        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}
/**
 * @brief
 *
 * @param window
 * @param win_size
 * @return Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowReset(SlidingWindow_t window)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;
    do
    {
        if (window == NULL)
        {
            break;
        }
        SlidingWindowCtrl_t *this_window = window;
        void                *curr_item   = (uint8_t *)this_window->first_item;

        do
        {
            memset(curr_item, 0, this_window->item_size);
            curr_item = (uint8_t *)curr_item + this_window->item_size;
        } while (curr_item <= this_window->last_item);

        ret = FILTER_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief         Deletes a previously created window
 * @param[in,out] window: The pointer to the window
 * @return        Result of the operation @ref FilterRet_e
 */
FilterRet_e FilterSlidingWindowDelete(SlidingWindow_t *window)
{
    FilterRet_e ret = FILTER_RET_ERR_INV_PARAM;

    if (*window != NULL)
    {
        FilterFree(*window);
        *window = NULL;
        ret     = FILTER_RET_OK;
    }

    return ret;
}

/**
 *  @brief 		Function that calculates the elapsed time from an initial time (use \ref FilterGetTick)
 *	@param[in]  InitialTime: Initial time for calculation
 *  @return 	Elapsed time from initial time in milliseconds
 *  @note       This function corrects the error caused by overflow
 */
static uint32_t FilterGetElapsedTime(uint32_t InitialTime)
{
    uint32_t actualTime;
    actualTime = FilterGetTick();

    if (InitialTime <= actualTime)
        return (actualTime - InitialTime);
    else
        return (((0xFFFFFFFFUL) - InitialTime) + actualTime);
}
/**
 *  @brief      Returns the current value of the tick timer in 1 ms
 *  @return     Tick value
 *  @warning    Stronger function must be declared externally
 */
__weak uint32_t FilterGetTick(void)
{
    return 0;
}

/**
 * @brief       Weakly defined memory allocation function
 * @param[in]   size: Size in bytes of the desired area
 * @return      A pointer to the allocated area or NULL if it fails
 */
__weak void *FilterMalloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief       Weakly defined memory deallocation function
 * @param[out]  ptr: Buffer to be freed
 */
__weak void FilterFree(void *ptr)
{
    free(ptr);
}
