/***********************************************************************
* $Id$		DVR830xDriver.c         2021-05-23
*//**
* \file		DVR830xDriver.c
* \brief	Source file of DVR8307 brushless motor control driver 
* \version	1.0
* \date		23. May. 2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************************************/
#include "DVR830xDriver.h"

/** @addtogroup  DVR830xDriverWeak DVR830x Driver Weak
 * @{
 */ 
__weak void DVR830xHalInit(void);
__weak void DVR830xBrake(DVR830x_ENABLE_e enable);
__weak void DVR830xEnable(DVR830x_ENABLE_e enable);
__weak void DVR830xDir(DVR830_direction_e dir);
__weak bool DVR830xInFault(void);
__weak uint32_t DVR830xGetTick (void);
__weak void DVR830xSetDuty (uint32_t duty);

__weak void * DVR830xMutexCreate(void);
__weak bool DVR830xMutexTake(void * Mutex);
__weak bool DVR830xMutexGive(void * Mutex);
__weak void DVR830xEnterCritical(void);
__weak void DVR830xExitCritical(void);
__weak void DVR830xDelay(uint32_t millisec);
/** @}*/ //End of DVR830xDriverPrivate

/** @addtogroup  DVR830xDriverPrivate DVR830x Driver Private
 * @{
 */ 
/** Outside Diameter 72 DC Brush Motor -  Ratio 81*/
#define  Z82BLDPN24200_45   1
/** Outside Diameter 82 DC Brush Motor -  Ratio 45*/
#define  Z72BLDPN24120_81   2
 
#if !defined DC_MOTOR 
    #error "DC_MOTOR is not defined"
#elif (DC_MOTOR == Z82BLDPN24200_45)
    #define REDUCTION                   45      /**<Motor reduction*/
    #define MAX_SPEED                   3500    /**<Maximum possible motor speed, in RPM*/
    #define POLES                       3       /**<Number of poles per phase*/
#elif (DC_MOTOR == Z72BLDPN24120_81)
    #define REDUCTION                   81      /**<Motor reduction*/
    #define MAX_SPEED                   3500    /**<Maximum possible motor speed, in RPM*/
    #define POLES                       3       /**<Number of poles per phase*/
#else
    #error "DC_MOTOR is with an invalid value"
#endif

#define TOUR_ANGLE                  250     /**<Total angle between the open position and the closed position of the gate*/
#define ANGLE_TOLERANCE             50      /**<Motor axis aperture tolerance, in degrees*/

#define STEPS_PER_ROUND     (REDUCTION * POLES)                     /**<Steps required for a complete motor revolution*/
#define LIMIT_ANGLE         TOUR_ANGLE + ANGLE_TOLERANCE            /**<Angle from which the motor stops completely*/
#define MINIMUM_PERIOD      60000/ (MAX_SPEED * STEPS_PER_ROUND)    /**<The shortest possible time for a pulse on the hallout pin (ms)*/

/*!
 *  \brief States of DVR830xManager StateMachine
 */
typedef enum{
    DVR830x_STOPPED,            /**<Motor is stopped*/
    DVR830x_START,              /**<Initializing state machine*/
    DVR830x_ACCELERATION,       /**<Motor speeding up*/
    DVR830x_CRUISE_SPEED,       /**<Motor at cruise speed*/
    DVR830x_DESACCELERATION,    /**<Motor speeding slowing down*/
    DVR830x_VALLEY_SPEED,       /**<Motor at valley speed*/
}DVR830x_state_e;
/*!
 *  \brief Structure for storing the variables for calculating engine speed and position
 */
typedef struct{
    uint32_t  FullOperationTime;       /**<Last operation execution time*/
    uint32_t  FullOperationTimestamp;  /**<Time stamp for calculation of operating time*/
    uint32_t  MinPulseTime;            /**<Shorter pulse time detected in an motor operation*/ 
    uint32_t  PulseTime;               /**<Current pulse time*/
    uint32_t  NumberOfPulses;          /**<Number of pulses detected in an motor operation*/
}DVR830x_Hallout_t;

DVR830x_state_e    DVR830_state;        /**<Global state of DVR830xManager StateMachine.*/
DVR830x_config_t   g_config;            /**<Global values for acceleration ramp control.*/
DVR830x_Hallout_t  DVR830x_Hallout;     /**<Global values for position and velocity acquisition*/
void *             DVR830xMutex = NULL; /**<Mutex to handle global configuration values.*/
volatile uint8_t   pwm;                 /**<Current Pwm applied to the motor driver*/

static uint32_t DVR830xGetElapsedTime (uint32_t InitialTime);

/** @}*/ //End of DVR830xDriverPrivate
/*!
 *  \brief  Initializes the driver control pins
*/
__weak void DVR830xHalInit(void)
{
}
/*!
 *  \brief      Turns on of off the brake pin 
 *  \param[in]  enable: DVR830x_ENABLE to enable brake | DVR830x_DISABLE to disable brake \ref DVR830x_ENABLE_e
 */
__weak void DVR830xBrake(DVR830x_ENABLE_e enable)
{
}
/*!
 *  \brief      Enable | Disable driver 
 *  \param[in]  enable: DVR830x_ENABLE to enable brake | DVR830x_DISABLE to disable brake \ref DVR830x_ENABLE_e
 */
__weak void DVR830xEnable(DVR830x_ENABLE_e enable)
{
}
/*!
 *  \brief      Set the direction pin to the clockwise or anticlockwise direction
 *  \param[in]  dir: motor direction \ref DVR830_direction_e
 */
__weak void DVR830xDir(DVR830_direction_e dir)
{
}
/*!
 *  \brief  Reads the current state of the driver's fault pin
 *  \return Fault status
 *  \retval true: Driver is in Fault
 *  \retval false: Driver is not in Fault
*/
__weak bool DVR830xInFault(void)
{
    return false;
}
/*!
 *  \brief      Returns the current value of the tick timer in ms
 *  \return     Tick value
 */
__weak uint32_t DVR830xGetTick (void)
{
    return 0;
}
/*!
 *  \brief      Sets the motor duty cycle to the value passed as a parameter
 *  \param[in]  duty: new duty value in %
 */
__weak void DVR830xSetDuty (uint32_t duty)
{
}
/*!
 *  \brief      Creates a mutex to handle critical sessions
 *  \return  	If the mutex was created successfully then a handle to the created mutex is returned. 
 *              If the mutex was not created then NULL is returned.
 */
__weak void * DVR830xMutexCreate(void)
{
    void *data;
    return data;
}
/*!
 *  \brief      Obtain a mutex
 *  \param[in]  Mutex: a handle to the semaphore being taken - obtained when the mutex was created.
 *  \return  	Result of operation
 *  \retval     true: mutex successfully obtained
 *  \retval     false: error in obtaining the mutex
 *  \warning    Stronger function must be declared externally
 */
__weak bool DVR830xMutexTake(void * Mutex)
{
    return true;
}
/*!
 *  \brief      Release a mutex
 *  \param[in]  Mutex: a handle to the semaphore being released - obtained when the semaphore was created.
 *  \return  	Result of operation
 *  \retval     true: mutex successfully released
 *  \retval     false: mutex release error
 *  \warning    Stronger function must be declared externally
 */
__weak bool DVR830xMutexGive(void * Mutex)
{
    return true;
}
/*!
 *  \brief  Enter a critical code session
 */
__weak void DVR830xEnterCritical(void)
{
    
}
/*!
 *  \brief  Exit from critical code session
 */
__weak void DVR830xExitCritical(void)
{
    
}
/*!
 *  \brief      Wait for Timeout (Time Delay)
 *  \param[in]  millisec: time delay value in ms
 */
__weak void DVR830xDelay(uint32_t millisec)
{
}
/*!
 *  \brief 		Function that calculates the elapsed time from an initial time
 *  \note       This function must be called at the corresponding interrupt to the pin that is connected to the HALLOUT
 */
void HALLOUT_EXTI_Callback(void)
{
    #warning "Colocar um xQueueSendFromISR aqui e um xQueueReceive na BarrierTask"
    static uint32_t last_time;
    uint32_t acctual_time;

    acctual_time = DVR830xGetTick();
    DVR830x_Hallout.NumberOfPulses++;
    
    DVR830x_Hallout.PulseTime = acctual_time - last_time;
    if((DVR830x_Hallout.PulseTime > MINIMUM_PERIOD) && \
       (DVR830x_Hallout.PulseTime < DVR830x_Hallout.MinPulseTime))
    {
        DVR830x_Hallout.MinPulseTime = DVR830x_Hallout.PulseTime;
    }
    
    last_time = acctual_time;
}
/*!
 *  \brief  Starts the counter responsible for measuring the operational time
 */
void DVR830xStartEndOperationTime (void)
{
    DVR830x_Hallout.FullOperationTimestamp = DVR830xGetTick();
}
/*!
 *  \brief  Stops the counter responsible for measuring the operational time
 */
void DVR830xStopEndOperationTime (void)
{
    DVR830x_Hallout.FullOperationTime = DVR830xGetElapsedTime(DVR830x_Hallout.FullOperationTimestamp);
}
/*!
 *  \brief  Resets all counters and starts a new position and velocity measurement
 */
void DVR830xResetCounters(void)
{
    DVR830xEnterCritical();
    //DVR830xStartEndOperationTime();
    DVR830x_Hallout.MinPulseTime = 0xFFFFFFFF;
    DVR830x_Hallout.NumberOfPulses = 0;
    DVR830xExitCritical();
}
/*!
 *  \brief 		Function to get current motor angle
 *  \return 	Angle traveled by the engine since the DVR830xResetCounters() function was executed
 */
uint32_t DVR830xGetAngle(void)
{
    uint32_t value;
    DVR830xEnterCritical();
    value =  (DVR830x_Hallout.NumberOfPulses * 360) /STEPS_PER_ROUND ;
    DVR830xExitCritical();
    return value;
}
/*!
 *  \brief 		Function to get motor maximum speed
 *  \return 	Maximum speed reached by the motor since the DVR830xResetCounters() function was executed
 */
uint32_t DVR830xGetMaxSpeed(void)
{
    uint32_t value;
    DVR830xEnterCritical();
    value = (60000/(STEPS_PER_ROUND * DVR830x_Hallout.MinPulseTime)) ;
    DVR830xExitCritical();
    return value;
}
/*!
 *  \brief 		Function to get motor mean speed
 *  \return 	Mean speed reached by the motor since the DVR830xResetCounters() function was executed
 */
uint32_t DVR830xGetMeanSpeed(void)
{
    return (60000 * DVR830xGetAngle())/(360*DVR830xFullOperationTime());
}
/*!
 *  \brief 		Function to get current motor speed
 *  \return 	Current motor speed
 */
uint32_t DVR830xSpeed(void)
{
    uint32_t value;
    DVR830xEnterCritical();
    value = (60000/(STEPS_PER_ROUND * DVR830x_Hallout.PulseTime)) ;
    DVR830xExitCritical();
    return value;
}
/*!
 *  \brief 		Function to get last operation time
 *  \return 	Operation time in ms
 */
uint32_t DVR830xFullOperationTime(void)
{
    uint32_t value;
    DVR830xEnterCritical();
    value = DVR830x_Hallout.FullOperationTime;
    DVR830xExitCritical();
    return (uint32_t)value;
}
/*!
 *  \brief  Set the direction pin to the anticlockwise direction
*/
void DVR830xInitialize (void)
{
    DVR830xHalInit();
    DVR830xMutex = DVR830xMutexCreate();
}
/*!
 *  \brief 		Function to set configuration values of DVR830x Driver
 *	\param[in]  config: pointer with configuration
 *  \return 	Result of operation
 *  \retval		DVR830x_RET_OK : Successful operation
 *  \retval		DVR830x_MUTEX_NULL_ERROR : Mutex was not created
 *  \retval		DVR830x_MUTEX_TAKE_ERROR : Error taking the mutex(not available)
 *  \retval		DVR830x_MUTEX_GIVE_ERROR : Error giving the mutex
*/
DVR830x_return_e DVR830xSetConfig (DVR830x_config_t config)
{
    DVR830x_return_e ret = DVR830x_RET_OK;

    if(DVR830xMutex != NULL)
    {
        if(DVR830xMutexTake(DVR830xMutex))
        {
            g_config.enable = config.enable;
            g_config.rot_dir = config.rot_dir;
            g_config.accel_time = config.accel_time;
            g_config.decel_time = config.decel_time;
            g_config.last_decel_time = config.last_decel_time;
            g_config.cruise_time = config.cruise_time;
            g_config.angle_to_decel = config.angle_to_decel;
            g_config.valley_time = config.valley_time;
            g_config.duty_min = config.duty_min;
            g_config.duty_max = config.duty_max;
            if(!DVR830xMutexGive( DVR830xMutex ))  ret =  DVR830x_MUTEX_GIVE_ERROR;
        } 
        else    ret =  DVR830x_MUTEX_TAKE_ERROR;
    }
    else    ret =  DVR830x_MUTEX_NULL_ERROR;
    
    return ret;
}
/*!
 *  \brief 		Function to get current configuration values of DVR830x Driver
 *	\param[out] config: pointer where configuration will be placed
 *  \return 	Result of operation
 *  \retval		DVR830x_RET_OK : Successful operation
 *  \retval		DVR830x_MUTEX_NULL_ERROR : Mutex was not created
 *  \retval		DVR830x_MUTEX_TAKE_ERROR : Error taking the mutex(not available)
 *  \retval		DVR830x_MUTEX_GIVE_ERROR : Error giving the mutex
*/
DVR830x_return_e DVR830xGetConfig (DVR830x_config_t *config)
{
    DVR830x_return_e ret = DVR830x_RET_OK;

    if(DVR830xMutex != NULL)
    {
        if(DVR830xMutexTake(DVR830xMutex))
        {
            config->enable = g_config.enable;
            config->rot_dir = g_config.rot_dir;
            config->accel_time = g_config.accel_time;
            config->decel_time = g_config.decel_time;
            config->last_decel_time = g_config.last_decel_time;
            config->cruise_time = g_config.cruise_time;
            config->angle_to_decel = g_config.angle_to_decel;
            config->valley_time = g_config.valley_time;
            config->duty_min = g_config.duty_min;
            config->duty_max = g_config.duty_max;

            if(!DVR830xMutexGive( DVR830xMutex ))  ret =  DVR830x_MUTEX_GIVE_ERROR;
        }
        else    ret =  DVR830x_MUTEX_TAKE_ERROR;
    }
    else    ret =  DVR830x_MUTEX_NULL_ERROR;
    
    return ret;
}
/*!
 *  \brief 		Function to get current configuration values of DVR830x Driver
 *	\param[in]  direction: direction to which the motor will be rotated (Clockwise | Anticlockwise)
 *  \return 	Result of operation
 *  \retval		DVR830x_RET_OK : Successful operation
 *  \retval		DVR830x_MUTEX_NULL_ERROR : Mutex was not created
 *  \retval		DVR830x_MUTEX_TAKE_ERROR : Error taking the mutex(not available)
 *  \retval		DVR830x_MUTEX_GIVE_ERROR : Error giving the mutex
 *  \warning    To activate the motor, it is necessary to run DVR830xManager() function
*/
DVR830x_return_e DVR830xStart (DVR830_direction_e direction)
{
    DVR830x_return_e ret = DVR830x_RET_OK;
    
    if(DVR830xMutex != NULL)
    {
        if(DVR830xMutexTake(DVR830xMutex))
        {
            g_config.rot_dir = direction;
            g_config.enable = DVR830x_ENABLE;
            if(!DVR830xMutexGive( DVR830xMutex ))  ret =  DVR830x_MUTEX_GIVE_ERROR;
        }
        else    ret =  DVR830x_MUTEX_TAKE_ERROR;
    }
    else    ret =  DVR830x_MUTEX_NULL_ERROR;
    
    if(ret == DVR830x_RET_OK)
    {
        DVR830xSetDuty(0);
        DVR830_state = DVR830x_START;
    }
    
    return ret;
}
/*!
 *  \brief      Reduces the motor speed to the minimum configured
 *  \warning    This function is blocking
 */
void DVR830xSlowDown (void)
{
    while(pwm > g_config.duty_min)
    {
        pwm--;
        DVR830xSetDuty(pwm);
        DVR830xDelay(g_config.last_decel_time);
    }
}
/*!
 *  \brief      Decelerate the motor until it stops
 *  \param[in]  brake: defines whether the engine will remain braked after stopping
 */
void DVR830xSoftStop (bool brake)
{
    DVR830_state = DVR830x_STOPPED;
    
    while(pwm > 0)
    {
        pwm--;
        DVR830xSetDuty(pwm);
        DVR830xDelay(g_config.last_decel_time);
    }
    
    if(brake == true)
    {
        DVR830xBrake(DVR830x_ENABLE);
        DVR830xEnable(DVR830x_ENABLE);
    }
    else
    {
        DVR830xBrake(DVR830x_DISABLE);
        DVR830xEnable(DVR830x_ENABLE);
    }
}
/*!
 *  \brief  Stops motor
*/
void DVR830xStop (void)
{
    DVR830xBrake(DVR830x_ENABLE);
    DVR830xEnable(DVR830x_ENABLE);
    pwm = 0;
    DVR830xSetDuty(pwm);
    DVR830_state = DVR830x_STOPPED;
}
/*!
 *  \brief 		Drives the motor to the direction setted in DVR830xStart() and the acceleration ramp setted in DVR830xSetConfig()
 *  \return 	Result of operation
 *  \retval		DVR830x_RET_OK : Successful operation (motor is running)
 *  \retval     DVR830x_NOT_ENABLED: Driver was not enabled, execute DVR830xStart function
 *  \retval     DVR830x_TIMEOUT: Timeout occurred 
 *  \retval     DVR830x_MUTEX_TAKE_ERROR: Error taking the mutex(not available)
 *  \retval     DVR830x_MUTEX_GIVE_ERROR: Error giving the mutex
 *  \retval     DVR830x_MUTEX_NULL_ERROR: Mutex was not created
 *  \warning    To activate the motor, it is necessary to run DVR830xManager() function
*/
DVR830x_return_e DVR830xManager (void)
{
    DVR830x_return_e ret = DVR830x_RET_OK;
    static DVR830x_config_t accel_ramp;
    static DVR830_direction_e direction;
    static uint32_t Timestamp;
    static uint32_t angle;
    
    angle = DVR830xGetAngle();

    if(angle > LIMIT_ANGLE)
    {
        DVR830_state = DVR830x_STOPPED;
        ret = DVR830x_OVER_LIMIT_ANGLE;
    }
    if((DVR830xInFault())&&(DVR830_state != DVR830x_START))
    {
        DVR830_state = DVR830x_STOPPED;
        ret = DVR830x_FAULT;
    }
    
    switch(DVR830_state)
    {
        case DVR830x_STOPPED:
        {
            DVR830xBrake(DVR830x_ENABLE);
            //DVR830xDisable(); 
            DVR830xSetDuty(0);
            if(ret == DVR830x_RET_OK)   ret = DVR830x_NOT_ENABLED;
        }
        break;
        case DVR830x_START:
        {
            ret = DVR830x_RET_OK;
            if( (DVR830xMutex != NULL) && (ret == DVR830x_RET_OK) )
            {
                if(DVR830xMutexTake(DVR830xMutex))
                {
                    accel_ramp.accel_time =  g_config.accel_time ;
                    accel_ramp.cruise_time = g_config.cruise_time;
                    accel_ramp.decel_time = g_config.decel_time;
                    accel_ramp.valley_time = g_config.valley_time;
                    accel_ramp.angle_to_decel = g_config.angle_to_decel;
                    direction = g_config.rot_dir;
                    if(g_config.enable != DVR830x_ENABLE) ret = DVR830x_NOT_ENABLED;
                    
                    if(!DVR830xMutexGive( DVR830xMutex ))   ret =  DVR830x_MUTEX_GIVE_ERROR;
                }
                else    ret =  DVR830x_MUTEX_TAKE_ERROR;
            }
            else    ret =  DVR830x_MUTEX_NULL_ERROR;
            
            if(ret == DVR830x_RET_OK)
            {
                pwm = 0;
                DVR830xSetDuty(pwm);
                Timestamp = DVR830xGetTick();
                DVR830_state = DVR830x_ACCELERATION;
            }
            if(ret == DVR830x_RET_OK)
            {
                if (direction == Clockwise)             DVR830xDir(Clockwise);
                else if (direction == Anticlockwise)    DVR830xDir(Anticlockwise);
                else                                    ret =  DVR830x_INV_PARAM;
            }
            if(ret == DVR830x_RET_OK)
            {
                DVR830xEnable(DVR830x_ENABLE);
                DVR830xBrake(DVR830x_DISABLE);
            }
        }
        break;
        case DVR830x_ACCELERATION:
        {
            if(pwm >= g_config.duty_max)
            {
                DVR830_state = DVR830x_CRUISE_SPEED;
                Timestamp = DVR830xGetTick();
            }
            else if(DVR830xGetElapsedTime(Timestamp) >= accel_ramp.accel_time)
            {
                DVR830xSetDuty(pwm++);
                Timestamp = DVR830xGetTick();
            }
        }
        break;
        case DVR830x_CRUISE_SPEED:
        {
            if((DVR830xGetElapsedTime(Timestamp) >= accel_ramp.cruise_time) ||
               (angle >= accel_ramp.angle_to_decel))
            {
                DVR830_state = DVR830x_DESACCELERATION;
                Timestamp = DVR830xGetTick();
            }
        }
        break;
        case DVR830x_DESACCELERATION:
        {
            if(pwm <= g_config.duty_min)
            {
                DVR830_state = DVR830x_VALLEY_SPEED;
                Timestamp = DVR830xGetTick();
            }
            else if(DVR830xGetElapsedTime(Timestamp) >= accel_ramp.decel_time)
            {
                DVR830xSetDuty(pwm--);
                Timestamp = DVR830xGetTick();
            }
        }
        break;
        case DVR830x_VALLEY_SPEED:
        {
            if(DVR830xGetElapsedTime(Timestamp) > accel_ramp.valley_time)
            {
                DVR830_state = DVR830x_STOPPED;
                DVR830xStopEndOperationTime();
                DVR830xBrake(DVR830x_ENABLE);
                DVR830xSetDuty(0);
                ret = DVR830x_TIMEOUT;
            }
        }
        break;
    }
    
    return ret;
}
/*!
 *  \brief  	This function will run the engine according to the parameters provided
 *  \param[in]  direction: Direction of rotation: \n
 *              Clockwise: Rotates motor in clockwise direction \n
 *              Anticlockwise: Rotates motor in anticlockwise direction
 *	\param[in]	pulse_time: time the motor will remain running
 *	\param[in]	in_pwm: PWM thar will be applied to the motor
 *  \return 	Result of operation
 *  \retval		DVR830x_RET_OK : Successful operation
 *  \retval		DVR830x_MUTEX_NULL_ERROR : Mutex was not created
 *  \retval		DVR830x_MUTEX_TAKE_ERROR : Error taking the mutex(not available)
 *  \retval		DVR830x_MUTEX_GIVE_ERROR : Error giving the mutex
 *  \warning    This function is blocking during the pulse period, use it only for test purposes 
 */
DVR830x_return_e DVR8030xPulse (DVR830_direction_e direction, uint32_t pulse_time, uint8_t in_pwm)
{
    DVR830x_return_e ret = DVR830x_RET_OK;
    
    if(in_pwm > 100)
    {
        ret =  DVR830x_INV_PARAM;
    }
    else
    {
        pwm = in_pwm;
    }
    
    if(ret == DVR830x_RET_OK)
    {
        if (direction == Clockwise)             DVR830xDir(Clockwise);
        else if (direction == Anticlockwise)    DVR830xDir(Anticlockwise);
        else                                    ret =  DVR830x_INV_PARAM;
    }
    
    if(ret == DVR830x_RET_OK)
    {
        DVR830xSetDuty(pwm);
        DVR830xEnable(DVR830x_ENABLE);
        DVR830xBrake(DVR830x_DISABLE);
        DVR830xDelay(pulse_time);
    }
    if(ret == DVR830x_RET_OK)
    {
        DVR830xSoftStop(true);
    }
    
    return ret;
}
/*!
 *  \brief 		Function that calculates the elapsed time from an initial time
 *	\param[in]  InitialTime: Initial time for calculation
 *  \return 	Elapsed time from initial time in milliseconds
 *  \note       This function corrects the error caused by overflow
 */
static uint32_t DVR830xGetElapsedTime (uint32_t InitialTime)
{
    uint32_t actualTime;
    actualTime = DVR830xGetTick();

    if ( InitialTime <= actualTime )
        return ( actualTime - InitialTime );
    else
        return ( ( (0xFFFFFFFFUL) - InitialTime ) + actualTime );
}
