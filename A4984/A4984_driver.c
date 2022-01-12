/*!*********************************************************************
 * $Id$     A4984_driver.c         11 de nov de 2020
 *//*!
 * \file       A4984_driver.c
 * \brief      Allegro A4984 Stepper Motor Driver component source file
 * \date       2020-11-11
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2020
 ***********************************************************************/
/*! @addtogroup A4984DriverPrivate A4984 Driver Private
 * @{*/
#include "A4984_driver.h"
#include "linked_list.h"

/*!
 * \brief       Instance of a A4984 Driver
 */
typedef struct A4984DriverDefinition
{
    struct A4984DriverDefinition *self;      /*!<Pointer to self to identify created object*/
    A4984Mode_e                   step_mode; /*!<Mode of stepping \ref A4984Mode_e*/
    bool                          enabled;   /*!<Boolean indicating if the driver is currenly enabled*/
    A4984PinFunc_t                funcs;     /*!<Set of function pointers required to drive the A4984*/
} A4984Driver_t;

/*!
 * \brief       A4984 List of drivers
 */
static LinkedListInstance_t A4984List;

/*! @}*/

/*!
 * \brief       Initializes a instance of a driver and inserts into a list
 * \param[out]  p_driver_ins: Created driver instance
 * \param[in]   pin_funcs: Set of function to drive the necessary pins \ref A4984PinFunc_t
 * \return      Resulf of the operation
 */
A4984Ret_e A4984Init(A4984Instance_t *p_driver_ins, A4984PinFunc_t pin_funcs)
{
    A4984Ret_e ret = A4984_RET_NOT_INIT_ERR;
    do
    {
        if (*p_driver_ins != NULL)
        {
            break;
        }
        if (A4984List == NULL)
        {
            if (LinkedListInit(&A4984List, sizeof(A4984Driver_t), 10 * sizeof(A4984Driver_t)) != LINKED_LIST_RET_OK)
            {
                ret = A4984_RET_MEM_ERR;
                break;
            }
        }
        LinkedListNode_t *this_drv_node = LinkedListApend(A4984List, NULL);

        if (this_drv_node == NULL)
        {
            ret = A4984_RET_MEM_ERR;
            break;
        }
        A4984Driver_t *driver_ptr;

        driver_ptr = (A4984Driver_t *)this_drv_node->item;

        driver_ptr->self      = driver_ptr;
        driver_ptr->step_mode = A4984_FULL_STEP;
        driver_ptr->enabled   = false;
        driver_ptr->funcs     = pin_funcs;
        ret                   = A4984_RET_OK;

        if (A4984DisableMotor(driver_ptr) != A4984_RET_OK)
        {
            ret = A4984_RET_CFG_ERR;
        }
        if (A4984SetMode(driver_ptr, driver_ptr->step_mode) != A4984_RET_OK)
        {
            ret = A4984_RET_CFG_ERR;
        }
        if (A4984DisableMotor(driver_ptr) != A4984_RET_OK)
        {
            ret = A4984_RET_CFG_ERR;
        }
        if (A4984SleepMotor(driver_ptr, false) != A4984_RET_OK)
        {
            ret = A4984_RET_CFG_ERR;
        }
        if (A4984ResetMotor(driver_ptr, false) != A4984_RET_OK)
        {
            ret = A4984_RET_CFG_ERR;
        }

        *p_driver_ins = driver_ptr;

    } while (0);

    return ret;
}

/*!
 * \brief       Enables the informed driver
 * \param[in]   driver_ins: The driver to be enabled
 * \return      Result of the operation \ref A4984Ret_e
 */
A4984Ret_e A4984EnableMotor(A4984Instance_t driver_ins)
{
    A4984Ret_e     ret      = A4984_RET_NOT_INIT_ERR;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;

    do
    {
        if (this_drv != this_drv->self)
        {
            break;
        }
        if (this_drv->funcs.en_func == NULL)
        {
            ret = A4984_RET_CFG_ERR;
            break;
        }
        A4984PinLevel_e enable_level = A4984_CLEAR_PIN;
        this_drv->enabled            = true;
        this_drv->funcs.en_func(enable_level);
        ret = A4984_RET_OK;
    } while (0);

    return ret;
}

/*!
 * \brief       Disabled the informed driver
 * \param[in]   driver_ins: The driver to be disabled
 * \return      Result of the operation \ref A4984Ret_e
 */
A4984Ret_e A4984DisableMotor(A4984Instance_t driver_ins)
{
    A4984Ret_e     ret      = A4984_RET_NOT_INIT_ERR;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;

    do
    {
        if (this_drv != this_drv->self)
        {
            break;
        }
        if (this_drv->funcs.en_func == NULL)
        {
            ret = A4984_RET_CFG_ERR;
            break;
        }
        A4984PinLevel_e enable_level = A4984_SET_PIN;
        this_drv->enabled            = false;
        this_drv->funcs.en_func(enable_level);
        ret = A4984_RET_OK;
    } while (0);
    return ret;
}

/*!
 * \brief       Signals the desired driver to sleep or wakeup
 * \param[in]   driver_ins: The driver to be waken or put to sleep
 * \param[in]   sleep: Boolean indicating if supposed to sleep or not
 * \return      Result of the operation \ref A4984Ret_e
 */
A4984Ret_e A4984SleepMotor(A4984Instance_t driver_ins, bool sleep)
{
    A4984Ret_e     ret      = A4984_RET_NOT_INIT_ERR;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;

    do
    {
        if (this_drv != this_drv->self)
        {
            break;
        }
        if (this_drv->funcs.en_func == NULL)
        {
            ret = A4984_RET_CFG_ERR;
            break;
        }
        A4984PinLevel_e nsleep_level = (sleep ? A4984_CLEAR_PIN : A4984_SET_PIN);
        this_drv->funcs.sleep_func(nsleep_level);
        ret = A4984_RET_OK;
    } while (0);
    return ret;
}

/*!
 * \brief       Signals the desired driver to reset or turn on
 * \param[in]   driver_ins: The desired driver to be reset or turned on
 * \param[in]   reset: Boolean indicating if supposed be in reset mode or not
 * \return      Result of the operation \ref A4984Ret_e
 */
A4984Ret_e A4984ResetMotor(A4984Instance_t driver_ins, bool reset)
{
    A4984Ret_e     ret      = A4984_RET_NOT_INIT_ERR;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;

    do
    {
        if (this_drv != this_drv->self)
        {
            break;
        }
        if (this_drv->funcs.en_func == NULL)
        {
            ret = A4984_RET_CFG_ERR;
            break;
        }
        A4984PinLevel_e nreset_level = (reset ? A4984_CLEAR_PIN : A4984_SET_PIN);
        this_drv->funcs.rst_func(nreset_level);
        ret = A4984_RET_OK;
    } while (0);
    return ret;
}

/*!
 * \brief       Change the driving mode of the desired driver instance
 * \param[in]   driver_ins: The desired driver
 * \param[in]   motor_mode: The desired mode
 * \return      Result of the operation \ref A4984Ret_e
 */
A4984Ret_e A4984SetMode(A4984Instance_t driver_ins, A4984Mode_e motor_mode)
{
    A4984Ret_e     ret      = A4984_RET_NOT_INIT_ERR;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;

    do
    {
        if (this_drv != this_drv->self)
        {
            break;
        }
        A4984PinLevel_e ms1_lvl = A4984_CLEAR_PIN;
        A4984PinLevel_e ms2_lvl = A4984_CLEAR_PIN;
        this_drv->step_mode     = motor_mode;
        ret                     = A4984_RET_OK;

        switch (this_drv->step_mode)
        {
            case A4984_FULL_STEP:
                break;
            case A4984_HALF_STEP:
                ms1_lvl = A4984_SET_PIN;
                break;
            case A4984_1_4_STEP:
                ms2_lvl = A4984_SET_PIN;
                break;
            case A4984_1_8_STEP:
                ms1_lvl = A4984_SET_PIN;
                ms2_lvl = A4984_SET_PIN;
                break;
            default:
                break;
        }
        if (this_drv->funcs.ms1_func != NULL)
        {
            this_drv->funcs.ms1_func(ms1_lvl);
        }
        else
        {
            ret = A4984_RET_CFG_ERR;
        }

        if (this_drv->funcs.ms2_func != NULL)
        {
            this_drv->funcs.ms2_func(ms2_lvl);
        }
        else
        {
            ret = A4984_RET_CFG_ERR;
        }
    } while (0);

    return ret;
}

/*!
 * \brief       Returns if the desired driver is currently enabled
 * \param[in]   driver_ins: The desired driver
 * \retval      true: Driver enabled
 * \retval      false: Driver disabled
 */
bool A4984CheckEnabled(A4984Instance_t driver_ins)
{
    bool           ret      = false;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;
    if (this_drv == this_drv->self)
    {
        ret = this_drv->enabled;
    }

    return ret;
}

/*!
 * \brief       Returns the currently configured mode of the desired driver
 * \param[in]   driver_ins: The desired driver
 * \return      The stepping mode \ref A4984Mode_e
 */
A4984Mode_e A4984GetMode(A4984Instance_t driver_ins)
{
    A4984Mode_e    ret      = A4984_MODE_INDEF;
    A4984Driver_t *this_drv = (A4984Driver_t *)driver_ins;
    if (this_drv == this_drv->self)
    {
        ret = this_drv->step_mode;
    }

    return ret;
}
