/*!
 * \file       KE401M120.h
 * \brief      Header file of the KE401-M120-R1 displacement sensor
 * \date       2020-11-10
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2020
 */
#ifndef _KE401M120_H_
#define _KE401M120_H_
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
/** @addtogroup KE401M120 KE401M120
 *
 */
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief       Return values of the KE401M120 component
     */
    typedef enum KE401M120RetDefinition
    {
        KE401M120_RET_OK = 0,    ///< Function executed successfully
        KE401M120_RET_ERR_MEM,   ///< Insufficient memory
        KE401M120_RET_ERR_INIT,  ///< Initialization error
        KE401M120_RET_ERR_PARAM, ///< Invalid parameters
    } KE401M120Ret_e;

    /**
     * @brief       KE401M120 Edges definition
     */
    typedef enum KE401M120EdgeDefinition
    {
        KE401RisingEdgeA = 0, ///< The A sensor had an rising edge event
        KE401FallingEdgeA,    ///< The A sensor had an falling edge event
        KE401RisingEdgeB,     ///< The A sensor had an rising edge event
        KE401FallingEdgeB,    ///< The A sensor had an falling edge event
    } KE401M120Edge_e;

    /**
     * @brief       KE401M120 Configuration structure
     */
    typedef struct KE401M120Cfg_s
    {
        uint32_t sample_interval_dx;     ///< The increment of the x axis used as a sample rate
        void (*GetDx)(int32_t *dx);      ///< Function to get the x axis
        void (*NewMeasCB)(int32_t meas); ///< Callback that the component will call when a new measure is acquired
        void (*NewMeasNotify)(void);     ///< Callback to notify a new measure
        void *(*MallocF)(size_t val);    ///< Function to allocate memory
        void (*FreeF)(void *val);        ///< Function to free memory
    } KE401M120Cfg_st;

    /**
     * @brief       Instace of a KE401M120 sensor
     */
    typedef void *KE401Instance_t;

    KE401M120Ret_e KE401Init(KE401Instance_t *ins, const KE401M120Cfg_st *const cfg);
    KE401M120Ret_e KE401DeInit(KE401Instance_t *ins);
    KE401M120Ret_e KE401Update(KE401Instance_t ins);
    KE401M120Ret_e KE401GetRawMeasure(KE401Instance_t ins, int32_t *measure_um);
    KE401M120Ret_e KE401AcquirePulse(KE401Instance_t ins, KE401M120Edge_e edge, bool other_sens_lvl);
    KE401M120Ret_e KE401Clear(KE401Instance_t ins);

#ifdef __cplusplus
}
#endif

/** @}*/ // End of KE401M120
#endif

