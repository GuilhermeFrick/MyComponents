/***********************************************************************
* $Id$		DVR830xConfig_template.c         2021-05-23
*//**
* \file		DVR830xConfig_template.c
* \brief	Template to strong definitions to DVR83xx brushless motor control driver 
* \version	1.0
* \date		23. May. 2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************************************/
/** @addtogroup  DVR830xConfig DVR830x Driver Config
 * @{
 */ 
#include "DVR830xDriver.h"
#include "OC.h"
#include "IO.h"
#include "WrapperRTOS.h"
#include "gpio.h"

/**Creation of output/compare*/
DEFINE_OC(CLOCK, TIM1, TIM_CHANNEL_1, E, 9, ACTIVE_LOW);

/*!
 *  \brief  Initializes the driver control pins
*/
void DVR830xHalInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_SET);
    
    /*Configure GPIO pins : PE0 PE1 PE2 PE3*/
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    /*Configure GPIO pins on GPIOE - Sensors pins*/
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    /*Configure GPIO pin : PE14 -> Hallout Interrupt*/
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    /* Priority of external interrupt pin for HALLOUT measurement */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); 
    
    pwm_config(CLOCK, 32000, 0);
    pwm_start(CLOCK);
}
void DVR830xBrake(DVR830x_ENABLE_e enable)
{
    if(enable == DVR830x_ENABLE)
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);//BRAKE
    }
    else
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);//NOT BRAKE
    }
}
/*!
 *  \brief      Enable | Disable driver 
 *  \param[in]  enable: DVR830x_ENABLE to enable brake | DVR830x_DISABLE to disable brake \ref DVR830x_ENABLE_e
 */
void DVR830xEnable(DVR830x_ENABLE_e enable)
{
    if(enable == DVR830x_ENABLE)
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET);//Enable ON
    }
    else
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);//Enable OFF   
    }
}
void DVR830xDir(DVR830_direction_e dir)
{
    if(dir == Clockwise)
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);//DIR0
    }
    else
    {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);//DIR1
    }
}
/*!
 *  \brief  Reads the current state of the driver's fault pin
 *  \return Fault status
 *  \retval true: Driver is in Fault
 *  \retval false: Driver is not in Fault
*/
bool DVR830xInFault(void)
{
    return false;
//    if(HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_13) == GPIO_PIN_SET)
//        return false;
//    else
//        return true;
}
/*!
 *  \brief      Returns the current value of the tick timer in ms
 *  \return     Tick value
 */
uint32_t DVR830xGetTick (void)
{
    return RTOSGetTick();
}
/*!
 *  \brief      Sets the motor duty cycle to the value passed as a parameter
 *  \param[in]  duty: new duty value in %
 */
void DVR830xSetDuty (uint32_t duty)
{
    pwm_set_duty(CLOCK,duty);
}
/*!
 *  \brief      Creates a mutex to handle critical sessions
 *  \return  	If the mutex was created successfully then a handle to the created mutex is returned. 
 *              If the mutex was not created then NULL is returned.
 */
void * DVR830xMutexCreate(void)
{
    return RTOSMutexCreate();
}
/*!
 *  \brief      Obtain a mutex
 *  \param[in]  Mutex: a handle to the semaphore being taken - obtained when the mutex was created.
 *  \return  	Result of operation
 *  \retval     true: mutex successfully obtained
 *  \retval     false: error in obtaining the mutex
 *  \warning    Stronger function must be declared externally
 */
bool DVR830xMutexTake(void * Mutex)
{
    return RTOSMutexTake(Mutex);
}
/*!
 *  \brief      Release a mutex
 *  \param[in]  Mutex: a handle to the semaphore being released - obtained when the semaphore was created.
 *  \return  	Result of operation
 *  \retval     true: mutex successfully released
 *  \retval     false: mutex release error
 *  \warning    Stronger function must be declared externally
 */
bool DVR830xMutexGive(void * Mutex)
{
    return RTOSMutexGive(Mutex);
}
/*!
 *  \brief  Enter a critical code session
 */
void DVR830xEnterCritical(void)
{
    RTOSEnterCritical();
}
/*!
 *  \brief  Exit from critical code session
 */
void DVR830xExitCritical(void)
{
    RTOSExitCritical();
}
/*!
 *  \brief      Wait for Timeout (Time Delay)
 *  \param[in]  millisec: time delay value in ms
 */
void DVR830xDelay(uint32_t millisec)
{
    RTOSDelay(millisec);
}
/** @}*/ //End of DVR830xConfig
