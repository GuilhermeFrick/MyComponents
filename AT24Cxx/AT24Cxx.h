/*!
 * \file      AT24Cxx.h
 * \author    Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \brief     EEPROM AT24Cxx write and read driver header file
 * \version   2.1
 * \date      2020-12-14
 * \copyright Copyright (c) 2020
 * \verbatim
 *  ** Making this component functional**
 *  =======================================
 *  1- Implement a variable uint32_t that is incremented every 1ms
 *  2- Override AT24CxxGetTick function to get the value of the variable that increments every 1 ms
 *  3- Override AT24CxxI2cWrite write function on the i2c bus with the correct error returns
 *  4- Override AT24CxxI2cRead read function off the i2c bus with the correct error returns
 *  5- Override AT24CxxI2cTake function off the take mutex with the correct error returns
 *  6- Include "AT24Cxx.h" in source file (include "AT24Cxx.h")
 *  7- export symbol AT24Cxx_DRIVER (extern AT24Cxx_DRIVER_t AT24Cxx_DRIVER;)
 * \n
 *   ** If using FreeRTOS **
 *  =======================================
 *  8- Override AT24CxxI2cTake to take the semaphore that locks the use of the i2c communication interface
 *  9- Override AT24CxxI2cGive to give back the semaphore that frees the use of the i2c communication interface
 *  10- Override AT24CxxOsDelay, delay function in ms to free up processing for other tasks while waiting
 * \endverbatim
 */

/** \addtogroup  AT24Cxx AT24Cxx
 * @{
 */
#ifndef __AT24Cxx_DRIVER
#define __AT24Cxx_DRIVER
#include <stdint.h>
#include <stdbool.h>

/*!
 *  \brief Return codes of the functions
 */
typedef enum AT24Cxx_RETURNDefinition
{
    AT24Cxx_RET_OK          = 0,  /**<The function executed successfully 	*/
    AT24Cxx_ADDR_INV        = -1, /**<Invalid input address */
    AT24Cxx_SIZE_INV        = -2, /**<Invalid input size */
    AT24Cxx_WR_IN_PROGRESS  = -3, /**<Write cycle in progress */
    AT24Cxx_I2C_ERROR       = -4, /**<I2c handler error */
    AT24Cxx_PARAM_INV       = -5, /**<Invalid parameter */
    AT24Cxx_NOT_INITIALIZED = -6, /**<Driver not initialized, run the Initialize function*/
    AT24Cxx_I2C_TAKE_ERROR  = -7, /**<Error in obtaining the mutex*/
    AT24Cxx_I2C_GIVE_ERROR  = -8, /**<Mutex release error*/
} AT24Cxx_RETURN;
/*!
 *  \brief List of supported models
 */
typedef enum AT24CxxModelDefinition
{
    AT24C01, /**<1K memory (128 x 8)*/
    AT24C02, /**<2K memory (256 x 8)*/
    AT24C04, /**<4K memory (512 x 8)*/
    AT24C08, /**<8K memory (1024 x 8)*/
    AT24C16  /**<16K memory (2048 x 8)*/
} AT24CxxModel_e;
/*!
 *  \brief List of possible addressing pins connections
 */
typedef enum AT24CxxAdressInputsDefinition
{
    AT24Cxx_HIG_IMP = 0,  /**<High impedance, Address input not connected*/
    AT24Cxx_GND     = -1, /**<Address input connected to VCC*/
    AT24Cxx_VCC     = 1,  /**<Address input connected to GND*/
} AT24CxxAdressInputs_e;
/*!
 *  \brief Memory settings structure to initialize driver
 */
typedef struct AT24CxxResourcesDefinition
{
    AT24CxxModel_e        Model : 8;  /**<Memory model used*/
    AT24CxxAdressInputs_e A0 : 2;     /**<A0 pin connection*/
    AT24CxxAdressInputs_e A1 : 2;     /**<A1 pin connection*/
    AT24CxxAdressInputs_e A2 : 2;     /**<A2 pin connection*/
} AT24CxxResources_t;
/*!
 *  \brief AT24Cxx Driver control block
 */
typedef struct AT24Cxx_DRIVERDefinition
{
    AT24Cxx_RETURN (*Initialize)(AT24CxxResources_t *resources);              /**< \ref AT24CxxInitialize : initialize driver*/
    AT24Cxx_RETURN (*Unitialize)(void);                                       /**< \ref AT24CxxUnitialize : unitialize driver*/
    AT24Cxx_RETURN (*WriteByte)(uint16_t addr, uint8_t data);                 /**< \ref AT24CxxWriteByte : write a byte*/
    AT24Cxx_RETURN (*ReadByte)(uint16_t addr, uint8_t *data);                 /**< \ref AT24CxxReadByte : read a byte*/
    AT24Cxx_RETURN (*Write)(uint16_t addr, uint8_t *data, uint16_t size);     /**< \ref AT24CxxWrite : write a buffer*/
    AT24Cxx_RETURN (*WritePage)(uint16_t addr, uint8_t *data, uint16_t size); /**< \ref AT24CxxWritePage : write a buffer limited to 1 page*/
    AT24Cxx_RETURN (*Read)(uint16_t addr, uint8_t *data, uint16_t size);      /**< \ref AT24CxxRead : read a buffer*/
    AT24Cxx_RETURN (*EraseAll)(void);                                         /**< \ref AT24CxxEraseAll : erase all bytes (write 0xFF)*/
    bool (*IsBusy)(void);                                                     /**< \ref AT24CxxBusy : checks if memory is busy*/
} const AT24Cxx_DRIVER_t;

#endif //__AT24Cxx_DRIVER
/** @}*/
