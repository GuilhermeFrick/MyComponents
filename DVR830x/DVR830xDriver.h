/***********************************************************************
* $Id$		DVR830xDriver.h         2021-05-23
*//**
* \file		DVR830xDriver.h
* \brief	Header file of DVR8307 brushless motor control driver
* \version	1.0
* \date		23. May. 2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************************************/
/** @addtogroup  DVR830xDriver DVR830x Driver
 * @{
 */ 
 
#ifndef DVR830XDRIVER_H
#define DVR830XDRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*!
 *  \brief Return codes of the functions
 */
typedef enum{
    DVR830x_RET_OK,             /**<Successfully executed function*/
    DVR830x_END,                /**<Motor stopped due to the end of the acceleration curve*/
    DVR830x_NOT_ENABLED,        /**<Driver was not enabled, execute DVR830xStart function*/
    DVR830x_OVER_LIMIT_ANGLE,   /**<Motor has stopped since it exceeded the angular limit of excursion */
    DVR830x_FAULT,              /**<Driver in fault condition*/
    DVR830x_TIMEOUT,            /**<Timeout occurred */
    DVR830x_MUTEX_TAKE_ERROR,   /**<Error taking the mutex (not available)*/
    DVR830x_MUTEX_GIVE_ERROR,   /**<Error giving the mutex*/
    DVR830x_MUTEX_NULL_ERROR,   /**<Mutex was not created*/
    DVR830x_INV_PARAM,          /**<Parameter entered is invalid*/
}DVR830x_return_e;
/*!
 *  \brief Motor rotation directions
 */
typedef enum{
    Clockwise = 0,      /**<Rotates motor in clockwise direction*/
    Anticlockwise = 1,  /**<Rotates motor in anticlockwise direction*/
}DVR830_direction_e;
/*!
 *  \brief Driver enable/disable
 */
typedef enum{
    DVR830x_ENABLE,      /**<Driver state machine is enabled*/
    DVR830x_DISABLE      /**<Driver state machine is disabled*/
}DVR830x_ENABLE_e;

/*!
 *  \brief This structure controls the engine acceleration deceleration ramp.\n
 * The ramp is linear and is controlled as in the figure below:
 * <PRE>
 *           cruise_time
 *               ||
 *               \/
 * duty_max....______   valley_time
 *            /      \     ||
 *           /        \    \/
 *          /          \ _________ .......duty_min
 *         /                      \
 *    ____/                        \ ____
 *          /\       /\           /\
 *          ||       ||           ||
 *    accel_time  decel_time   last_decel_time
 * </PRE>
 */
typedef struct{
    DVR830x_ENABLE_e enable;        /**<Driver state machine enable*/
    DVR830_direction_e rot_dir;     /**<Motor rotation direction*/
    uint32_t accel_time;            /**<Time to increase 1 step of speed (ms)*/
    uint32_t decel_time;            /**<Time to decrease 1 step of speed (ms)*/
    uint32_t last_decel_time;       /**<Time to decrease 1 step of speed (ms) after reach microswitch*/
    uint32_t cruise_time;           /**<Timeout to stay at cruising speed (ms)*/
    uint32_t angle_to_decel;        /**<Angle to start deceleration, in degrees*/
    uint32_t valley_time;           /**<Timeout to stay at minimum speed (ms)*/
    uint8_t duty_min;               /**<Higher duty cycle on the acceleration ramp (%)*/
    uint8_t duty_max;               /**<Lower duty cycle on the deceleration ramp (%)*/
}DVR830x_config_t;

void DVR830xInitialize (void);
DVR830x_return_e DVR830xSetConfig (DVR830x_config_t config);
DVR830x_return_e DVR830xGetConfig (DVR830x_config_t *config);
DVR830x_return_e DVR830xStart (DVR830_direction_e direction);
void DVR830xSlowDown (void);
void DVR830xSoftStop (bool brake);
void DVR830xStop(void);
DVR830x_return_e DVR830xManager (void);
DVR830x_return_e DVR8030xPulse (DVR830_direction_e direction, uint32_t pulse_time, uint8_t in_pwm);

void HALLOUT_EXTI_Callback(void);
void DVR830xResetCounters(void);
uint32_t DVR830xGetAngle(void);
uint32_t DVR830xGetMaxSpeed(void);
uint32_t DVR830xGetMeanSpeed(void);
uint32_t DVR830xSpeed(void);
uint32_t DVR830xFullOperationTime(void);
void DVR830xStartEndOperationTime (void);
void DVR830xStopEndOperationTime (void);
#endif //DVR830XDRIVER_H
/** @}*/

