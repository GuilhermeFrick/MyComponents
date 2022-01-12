/*!*********************************************************************
 * $Id$     A4984_driver.h         09 de nov de 2020
 *//*!
 * \file    A4984_driver.h 
 * \brief   Allegro A4984 Stepper Motor Driver component header file
 * \version	1.0
 * \date    09 de nov de 2020
 * \author  Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \verbatim
 *   ** Making this component functional **
 *   =======================================
 *   1- Create one or more A4984Instance_t variable to represent your driver(s) instance(s)
 *   2- Define your own write pin functions in a higher layer
 *   3- Use the \ref A4984Init function for each instance with its pin functions in a A4984PinFunc_t variable
 *   4- Configure the mode according to your design
 *
 *   ** Notes about the \ref A4984PinFunc_t **
 *   The pin functions are expected to write the \ref A4984PinLevel_e into each pin
 *   so the user doesn't need to check the datasheet to verify which logic its expected 
 *   from them. 
 *
 * \endverbatim
 *************************************************************************/
/*! @addtogroup A4984Driver A4984 Driver
 * @{
 */
#ifndef _A4984_DRIVER_H_
#define _A4984_DRIVER_H_
#include <stdbool.h>
#include <stdint.h>

/*! A4984 return values */
typedef enum A4984RetDefinition
{
    A4984_RET_OK            = 0, /*!<The function executed successfully*/
    A4984_RET_NOT_INIT_ERR  = 1, /*!<The driver was not initialized*/
    A4984_RET_MEM_ERR       = 3, /*!<Memory allocation error*/
    A4984_RET_CFG_ERR       = 4, /*!<Configuration error occurred*/
    A4984_RET_INVALID_PARAM = 6  /*!<Invalid parameter error occurred*/
} A4984Ret_e;

/*! A4984 supported step resolutions*/
typedef enum A4984ModeDefinition
{
    A4984_FULL_STEP  = 0,
    A4984_HALF_STEP  = 1,
    A4984_1_4_STEP   = 2,
    A4984_1_8_STEP   = 3,
    A4984_MODE_INDEF = 4
} A4984Mode_e;

/*!
 * \brief       Pin level definition
 */
typedef enum A4984PinDefinition
{
    A4984_CLEAR_PIN = 0,
    A4984_SET_PIN   = !A4984_CLEAR_PIN
} A4984PinLevel_e;

/*!
 * \brief       Set of pin toggle functions required by the A4984 driver
 */
typedef struct A4984PinFuncDefinition
{
    void (*ms1_func)(A4984PinLevel_e lvl);     /*!<Function pointer to toggle the MS1 pin*/
    void (*ms2_func)(A4984PinLevel_e lvl);     /*!<Function pointer to toggle the MS2 pin*/
    void (*en_func)(A4984PinLevel_e en);       /*!<Function pointer to toggle the Enable pin*/
    void (*rst_func)(A4984PinLevel_e rst);     /*!<Function pointer to toggle the Reset pin*/
    void (*sleep_func)(A4984PinLevel_e sleep); /*!<Function pointer to toggle the Sleep pin*/
} A4984PinFunc_t;

/*! A4984 instance variable*/
typedef void *A4984Instance_t;

A4984Ret_e  A4984Init(A4984Instance_t *p_driver_ins, A4984PinFunc_t pin_funcs);
A4984Ret_e  A4984EnableMotor(A4984Instance_t driver_ins);
A4984Ret_e  A4984DisableMotor(A4984Instance_t driver_ins);
A4984Ret_e  A4984SleepMotor(A4984Instance_t driver_ins, bool sleep);
A4984Ret_e  A4984ResetMotor(A4984Instance_t driver_ins, bool reset);
A4984Ret_e  A4984SetMode(A4984Instance_t driver_ins, A4984Mode_e motor_mode);
bool        A4984CheckEnabled(A4984Instance_t driver_ins);
A4984Mode_e A4984GetMode(A4984Instance_t driver_ins);

#endif /* _A4984_DRIVER_H_ */
/*! @}*/
