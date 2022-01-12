/***********************************************************************
 * $Id$		filter.h         2020-10-14
 *//**
* \file		filter.h
* \version	1.0
* \date		14. Oct. 2020
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
* \brief	Source file with basic filter and PID regulator functions
*************************************************************************/
/** \addtogroup   Filter Filter
 * @{
 */
#ifndef FILTER_H
#define FILTER_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif
    /*!
     *  \brief States of debounce state machine
     */
    typedef enum
    {
        LOW_STATE       = -2, /**<Sensor is clear*/
        FALLING_STATE   = -1, /**<Sensor was setted but now is clear*/
        UNDEFINED_STATE = 0,  /**<Transition state*/
        RISING_STATE    = 1,  /**<Sensor was cleared but now is set*/
        HIGH_STATE      = 2   /**<Sensor is set*/
    } DebounceState_t;
    /*!
     *  \brief Status of sensor
     */
    typedef enum
    {
        SENSOR_CLEARED = 0, /**<Sensor is clear*/
        SENSOR_SET     = 1, /**<Sensor is set*/
    } SensorStatus_t;
    /*!
     *  \brief Control structure to debounce filter \ref SensorDebounce
     */
    typedef struct
    {
        DebounceState_t state;        /**<State of FSM in \ref SensorDebounce*/
        SensorStatus_t  status_pin;   /**<Instant sensor pin status, must be provided by caller*/
        SensorStatus_t  status;       /**<Debounced pin status*/
        uint32_t        trigger_low;  /**<Debounce to set sensor in clear state*/
        uint32_t        trigger_high; /**<Debounce to set sensor in set state*/
        uint32_t        timestamp;    /**<Timestamp used to debounce*/
    } DebounceControl_t;
    /*!
     *  \brief PID 32 bits regulator config
     */
    typedef struct
    {
        int32_t min; /**<Minimum value to output process_var value*/
        int32_t max; /**<Maximum value to output process_var value*/
        float   Kp;  /**<Proportional constant*/
        float   Ki;  /**<Integrative constant*/
        float   Kd;  /**<Derivative constant*/
        float   sat; /**<Error saturation, maximum error value to be processed*/
    } PID32Config_t;
    /*!
     *  \brief PID 32 bits data
     */
    typedef struct
    {
        int32_t process_var; /**<Output value of PID regulator*/
        float   error;       /**<Difference between setpoint and current process_var value*/
        float   last_error;  /**<Last calculated error*/
        float   integral;    /**<Sum of all calculated errors*/
    } PID32Data_t;

    /*!
     * \brief       Filter return values
     */
    typedef enum FilterRetDefinition
    {
        FILTER_RET_OK = 0,        /**<The function returned OK*/
        FILTER_RET_ERR_INV_PARAM, /**<A parameter is wrong*/
        FILTER_RET_ERR_MEM        /**<Insufficient memory*/
    } FilterRet_e;

    typedef void *CircBuff_t;      /**<Circular buffer handle*/
    typedef void *SlidingWindow_t; /**<Sliding window handle*/

    void        PID32Regulator(int32_t setpoint, const PID32Config_t *config, PID32Data_t *data);
    void        P32Regulator(int32_t setpoint, float Kp, int32_t *process_var);
    void        PfRegulator(float setpoint, float Kp, float *process_var);
    void        SensorDebounce(DebounceControl_t *control);
    FilterRet_e FilterCreateCircBuff(CircBuff_t *circ_buff, size_t item_size, size_t num_elements);
    FilterRet_e FilterSlidingWindowCreate(SlidingWindow_t *window, size_t item_size, size_t num_elements, void *default_value);
    FilterRet_e FilterSlidingWindowAppend(SlidingWindow_t window, void *item);
    FilterRet_e FilterSlidingWindowGetLastItems(SlidingWindow_t window, size_t n, void *items);
    FilterRet_e FilterSlidingWindowGetFloatAvg(SlidingWindow_t window, size_t n, float *const avg);
    FilterRet_e FilterSlidingWindowDelete(SlidingWindow_t *window);
    FilterRet_e FilterSlidingWindowIsCleared(SlidingWindow_t window, bool *is_empty);
    FilterRet_e FilterSlidingWindowGetTail(SlidingWindow_t window, void *item);
    FilterRet_e FilterSlidingWindowGetHead(SlidingWindow_t window, void *item);
    FilterRet_e FilterSlidingWindowGetItem(SlidingWindow_t window, size_t n, void *items);
    FilterRet_e FilterSlidingWindowReset(SlidingWindow_t window);

#ifdef __cplusplus
}
#endif
/** @}*/ // End of Filter
#endif
