/*!
 * \file       KE401M120.c
 * \brief      Source file of the KE401-M120-R1 displacement sensor
 * \date       2020-11-10
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2020
 */
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "KE401M120.h"
/** @addtogroup KE401M120Private KE401M120 Private
 *  @ingroup KE401M120
 *  @{
 */

/**
 * @brief       KE401M120 Raw information
 */
typedef struct KE401Raw_t
{
    int32_t count_A_um; ///< Micrometers counter on the A sensor
    int32_t count_B_um; ///< Micrometers counter on the B sensor
    int32_t sum;        ///< Sum of A and B counters
} KE401Raw_st;

/**
 * @brief       KE401M120 Control structure
 */
typedef struct KE401Ctrl_t
{
    struct KE401Ctrl_t    *self;        ///< Pointer to self
    const KE401M120Cfg_st *cfg;         ///< Pointer to configuration
    KE401Raw_st            raw_info;    ///< Raw information @ref KE401Raw_st
    int32_t                last_dx;     ///< Last x axis value
    int32_t                last_sum;    ///< Last raw sum
    uint32_t               residual_dx; ///< Leftover x axis difference
} KE401Ctrl_st;

static void KE401SendPulse(KE401Ctrl_st *ctrl, int32_t dx);

/** @} */

/**
 * @brief       Initializes a KE401Instance with the informed configuration
 * @param[in]   ins: Pointer to a instance to be initialized
 * @param[in]   cfg: Desired configuration
 * @return      Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401Init(KE401Instance_t *ins, const KE401M120Cfg_st *const cfg)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        if (*ins != NULL)
        {
            if (((KE401Ctrl_st *)(*ins))->self == *ins)
            {
                ret = KE401M120_RET_OK;
            }
            break;
        }
        if (cfg == NULL)
        {
            break;
        }
        if ((cfg->MallocF == NULL) || (cfg->FreeF == NULL))
        {
            break;
        }
        ret  = KE401M120_RET_ERR_MEM;
        *ins = cfg->MallocF(sizeof(KE401Ctrl_st));
        if (*ins == NULL)
        {
            break;
        }
        KE401Ctrl_st *this_ctrl = *ins;

        this_ctrl->self                = *ins;
        this_ctrl->cfg                 = cfg;
        this_ctrl->last_dx             = 0;
        this_ctrl->residual_dx         = 0;
        this_ctrl->raw_info.sum        = 0;
        this_ctrl->raw_info.count_A_um = 0;
        this_ctrl->raw_info.count_B_um = 0;

        ret = KE401M120_RET_OK;

    } while (0);

    return ret;
}

/**
 * @brief           De-initializes a KE401Instance
 * @param[in,out]   ins: Instance to be de-initialized
 * @return          Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401DeInit(KE401Instance_t *ins)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        if (*ins == NULL)
        {
            ret = KE401M120_RET_OK;
            break;
        }
        KE401Ctrl_st *ctrl = (KE401Ctrl_st *)(*ins);
        if (ctrl->self != *ins)
        {
            break;
        }
        if (ctrl->cfg->FreeF)
        {
            ctrl->cfg->FreeF(*ins);
        }
        *ins = NULL;

        ret = KE401M120_RET_OK;

    } while (0);

    return ret;
}

/**
 * @brief       Process raw data according to the x axis information
 * @param[in]   ins: Instance to process
 * @return      Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401Update(KE401Instance_t ins)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        KE401Ctrl_st *this_ctrl = ins;
        if ((this_ctrl == NULL) || (this_ctrl->self != this_ctrl))
        {
            break;
        }
        const KE401M120Cfg_st *c = this_ctrl->cfg;

        if ((c->GetDx == NULL) || (c->sample_interval_dx == 0))
        {
            break;
        }
        int32_t curr_dx = 0;

        c->GetDx(&curr_dx);
        int32_t diff_dx = curr_dx - this_ctrl->last_dx;
        
        if (diff_dx < 0)
        {
            this_ctrl->last_dx = 0;
            diff_dx = curr_dx;
        }

        if (diff_dx > 0)
        {
            KE401SendPulse(this_ctrl, diff_dx);
            this_ctrl->last_dx = curr_dx;
        }
        

    } while (0);

    return ret;
}

/**
 * @brief       Returns the current raw measure
 * @param[in]   ins: Instance of the sensor
 * @param[out]  measure_um: Measure in micrometers
 * @return      Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401GetRawMeasure(KE401Instance_t ins, int32_t *measure_um)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        KE401Ctrl_st *this_ctrl = ins;
        if ((this_ctrl == NULL) || (this_ctrl->self != this_ctrl))
        {
            break;
        }
        *measure_um = this_ctrl->raw_info.sum;

        ret = KE401M120_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Updates the raw data according to the edge and level of A or B sensors
 * @param[in]   ins: Instance of the sensor
 * @param[in]   edge: Which edge triggered the interruption @ref KE401M120Edge_e
 * @param[in]   other_sens_lvl: The level of the other sensor (A or B, depending on the edge)
 * @return      Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401AcquirePulse(KE401Instance_t ins, KE401M120Edge_e edge, bool other_sens_lvl)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        KE401Ctrl_st *this_ctrl = ins;
        if ((this_ctrl == NULL) || (this_ctrl->self != this_ctrl))
        {
            break;
        }
        switch (edge)
        {
            case KE401RisingEdgeA:
                if (other_sens_lvl)
                {
                    this_ctrl->raw_info.count_A_um -= 5;
                }
                else
                {
                    this_ctrl->raw_info.count_A_um += 5;
                }
                break;
            case KE401FallingEdgeA:
                if (other_sens_lvl)
                {
                    this_ctrl->raw_info.count_A_um += 5;
                }
                else
                {
                    this_ctrl->raw_info.count_A_um -= 5;
                }
                break;
            case KE401RisingEdgeB:
                if (other_sens_lvl)
                {
                    this_ctrl->raw_info.count_B_um += 5;
                }
                else
                {
                    this_ctrl->raw_info.count_B_um -= 5;
                }
                break;
            case KE401FallingEdgeB:
                if (other_sens_lvl)
                {
                    this_ctrl->raw_info.count_B_um -= 5;
                }
                else
                {
                    this_ctrl->raw_info.count_B_um += 5;
                }
                break;
        }
        this_ctrl->raw_info.sum = this_ctrl->raw_info.count_A_um + this_ctrl->raw_info.count_B_um;
        ret                     = KE401M120_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Sends measures according to the sampling rate and dx value
 * @param[in]   ctrl: The control by the current instance.
 * @param[in]   dx: difference between the last x position and the new one
 * @note        This function also implements a linear approximation curve if the
 *              dx + residual dx value exceeds 1 sampling interval.
 */
static void KE401SendPulse(KE401Ctrl_st *ctrl, int32_t dx)
{
    uint32_t curr_sum      = ctrl->raw_info.sum;
    uint32_t num_to_insert = (uint32_t)(abs(dx + (int32_t)ctrl->residual_dx)) / ctrl->cfg->sample_interval_dx;

    ctrl->residual_dx = (uint32_t)(abs(dx + (int32_t)ctrl->residual_dx)) % ctrl->cfg->sample_interval_dx;

    if ((num_to_insert > 1) && (ctrl->cfg->NewMeasCB))
    {
        float last_sum_f    = (float)(ctrl->last_sum);
        float current_ratio = ((float)curr_sum - last_sum_f) / (float)num_to_insert;

        for (uint32_t i = 0; i < num_to_insert - 1; i++)
        {
            last_sum_f += current_ratio;
            ctrl->cfg->NewMeasCB((int32_t)roundf(last_sum_f));
        }
    }
    if (num_to_insert)
    {
        if (ctrl->cfg->NewMeasCB)
        {
            ctrl->cfg->NewMeasCB(curr_sum);
        }
        if (ctrl->cfg->NewMeasNotify)
        {
            ctrl->cfg->NewMeasNotify();
        }
        ctrl->last_sum = curr_sum;
    }
}

/**
 * @brief       Clears the raw measure
 * @param[in]   ins: Instance of the sensor
 * @return      Result of the operation @ref KE401M120Ret_e
 */
KE401M120Ret_e KE401Clear(KE401Instance_t ins)
{
    KE401M120Ret_e ret = KE401M120_RET_ERR_PARAM;

    do
    {
        KE401Ctrl_st *this_ctrl = ins;
        if ((this_ctrl == NULL) || (this_ctrl->self != this_ctrl))
        {
            break;
        }
        this_ctrl->raw_info.sum        = 0;
        this_ctrl->raw_info.count_A_um = 0;
        this_ctrl->raw_info.count_B_um = 0;

        ret = KE401M120_RET_OK;
    } while (0);

    return ret;
}
