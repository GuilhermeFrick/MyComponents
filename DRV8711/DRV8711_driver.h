/***********************************************************************
 * $Id$     DRV8711_driver.h         28 de mar de 2021
 *//**
 * \file    DRV8711_driver.h 
 * \brief   Source file of the DRV8711_driver.h
 * \version	1.1
 * \date    28 de mar de 2021
 * \author  Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \verbatim
 *   ** Making this component functional **
 *   =======================================
 *   1- Create one or more drv8711_instance_t variable to represent your driver(s) instance(s)
 *   2- Override drv8711_transfer_spi function to a strong SPI transfer definition
 *   3- Use the \ref drv8711_init function for each instance.
 *   3(Optional)- Override drv8711_low_lvl_config function to initialize both pins and SPI used by you layout.
 *   If low level dependencies are already initialized, this step can be skipped.
 *   4(Optional)- Inform the Risense to the driver to enable the current configuration function
 *
 *   ** Memory allocation notes **
 *   ========================================
 *   1(Optional)- Override drv8711_malloc and drv8711_free functions to use your own memory control functions.
 *   - If the application doesn't have a memory management module, the weak functions will use the native calls.
 *   2(Optional)- If the user wants to use static memory , uncomment the DRV8711_DINAMIC_MEM definition in the source file
 *
 * \endverbatim
 *************************************************************************/
/** @addtogroup DRV8711Driver DRV8711 Driver
 * @{
 */
#ifndef STEP_MOTOR_DRV8711_DRIVER_H_
#define STEP_MOTOR_DRV8711_DRIVER_H_
#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief       DRV8711 return values
 */
typedef enum DRV8711ReturnDefinition
{
    DRV8711_RET_OK              = 0, /*!<The function executed successfully*/
    DRV8711_RET_NOT_INIT_ERR    = 1, /*!<The driver was not initialized*/
    DRV8711_RET_SPI_ERR         = 2, /*!<A SPI transmission error occurred*/
    DRV8711_RET_MEM_ERR         = 3, /*!<Memory allocation error*/
    DRV8711_RET_CFG_ERR         = 4, /*!<Low level configuration error occurred*/
    DRV8711_RET_LOAD_ERR        = 5, /*!<Driver configuration error occurred*/
    DRV8711_RET_INVALID_PARAM   = 6, /*!<Invalid parameter error occurred*/
    DRV8711_RET_MUTEX_TAKE_ERR  = 7, /*!< Mutex take failed*/
    DRV8711_RET_MUTEX_GIVE      = 8, /**< Mutex give failed*/
    DRV8711_RET_CE_ERR          = 9, /**< Error in chip selection*/
} DRV8711Ret_e;

/*!
 * \brief       DRV8711 supported step resolution
 */
typedef enum
{
    DRV8711_FULL_STEP  = 0, /*!<Full step resolution */
    DRV8711_HALF_STEP  = 1, /*!<Half step resolution */
    DRV8711_1_4_STEP   = 2, /*!<1/4 step resolution */
    DRV8711_1_8_STEP   = 3, /*!<1/8 step resolution */
    DRV8711_1_16_STEP  = 4, /*!<1/16 step resolution */
    DRV8711_1_32_STEP  = 5, /*!<1/32 step resolution */
    DRV8711_1_64_STEP  = 6, /*!<1/64 step resolution */
    DRV8711_1_128_STEP = 7, /*!<1/128 step resolution */
    DRV8711_1_256_STEP = 8, /*!<1/256 step resolution */
    DRV8711_MODE_INDEF = 9  /*!<Undefined step resolution */
} drv8711_mode_e;

/*!
 * \brief       Type of motor connected to the DRV8711
 */
typedef enum DRV8711MotorTypeDefinition
{
    DRV8711_STEP_MOTOR    = 0, /*!<Single step motor (default state)*/
    DRV8711_DUAL_DC_MOTOR = 1  /*!<Dual DC motor*/
} DRV8711MotorType_e;

/*!
 * \brief       Possible ISGAIN values
 */
typedef enum DRV8711IsGainDefinition
{
    DRV8711_GAIN_5  = 0, /*!<ISENSE amplifier gain of 5*/
    DRV8711_GAIN_10 = 1, /*!<ISENSE amplifier gain of 10*/
    DRV8711_GAIN_20 = 2, /*!<ISENSE amplifier gain of 20*/
    DRV8711_GAIN_40 = 3  /*!<ISENSE amplifier gain of 40*/
} DRV8711IsGain_e;

typedef enum DRV8711DecayModeDefinition
{
    DRV8711_ALWAYS_SLOW            = 0, /*!<Force slow decay at all times*/
    DRV8711_SLOW_INCR_MIX_DECR     = 1, /*!<Slow decay for increasing current, mixed decay for decreasing current (indexer mode only)*/
    DRV8711_ALWAYS_FAST            = 2, /*!<Force fast decay at all times*/
    DRV8711_ALWAYS_MIXED           = 3, /*!<Use mixed decay at all times*/
    DRV8711_SLOW_INCR_AUTOMIX_DECR = 4, /*!<Slow decay for increasing current, auto mixed decay for decreasing current (indexer mode only)*/
    DRV8711_ALWAYS_AUTOMIXED       = 5  /*!<Use auto mixed decay at all times*/
} DRV8711DecayMode_e;

/*!
 * \brief       DRV8711 Configuration
 */
typedef struct DRV8711ConfigDefinition
{
    DRV8711MotorType_e type;          /*!<Type of motor used \ref DRV8711MotorType_e*/
    drv8711_mode_e     mode;          /*!<Mode in case of step motor type*/
    uint8_t            risense_m_ohm; /*!<Driver Risense in mOhm*/
    uint16_t           current_mA;    /*!<Current in mA*/
} DRV8711Config_t;
/**
 *\brief Drv8711 Chip enable state
 *
 */
typedef enum DRV8711ChipEnableDefinition
{
    DRV8711_CE_DISABLE = 0,                     /**<Chip select disable*/
    DRV8711_CE_ENABLE  = !DRV8711_CE_DISABLE    /**<Chip select enable*/
} DRV8711ChipEnable_e;

/*!
 * \brief       DRV8711 Instance handle
 */
typedef void *drv8711_instance_t;

typedef DRV8711Ret_e drv8711_ret_e;

#define drv8711_init(x) DRV8711Init(x, NULL)

DRV8711Ret_e   DRV8711Init(drv8711_instance_t *p_driver_ins, DRV8711Config_t *default_config);
DRV8711Ret_e   drv8711_en_motor(drv8711_instance_t driver_ins, bool verify);
DRV8711Ret_e   drv8711_disable_motor(drv8711_instance_t driver_ins, bool verify);
DRV8711Ret_e   drv8711_set_mode(drv8711_instance_t driver_ins, drv8711_mode_e motor_mode);
bool           drv8711_check_enabled(drv8711_instance_t driver_ins);
drv8711_mode_e drv8711_get_mode(drv8711_instance_t driver_ins);
DRV8711Ret_e   drv8711_torque_config(drv8711_instance_t driver_ins, uint8_t torque);
DRV8711Ret_e   drv8711_isgain_config(drv8711_instance_t driver_ins, uint8_t isgain);
DRV8711Ret_e   drv8711_current_config(drv8711_instance_t driver_ins, uint16_t current_mA);
DRV8711Ret_e   drv8711_inform_risense(drv8711_instance_t driver_ins, uint16_t risense_m_ohm);
DRV8711Ret_e   drv8711_load_default_config(drv8711_instance_t driver_ins);

#endif /* STEP_MOTOR_DRV8711_DRIVER_H_ */
/** @}*/
