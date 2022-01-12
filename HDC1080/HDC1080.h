/***********************************************************************
* $Id$		HDC1080.h         2021-08-25
*//**
* \file		HDC1080.h
* \version	1.0
* \date		25. Aug. 2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
* \brief	Header file to HDC1080 sensor
* \verbatim
*   ** Making this component functional**
*   ======================================= 
*   1- Override write function on the i2c bus with the correct error returns (HDC1080Write) \n
*   2- Override read function off the i2c bus with the correct error returns (HDC1080Read) \n
*   3- Override function that returns the state of the i2c bus (HDC1080_IsI2CBusy) \n
*   4- Override function that set the state of the i2c bus (HDC1080_MakeI2CBusy) \n
*   5- Override a delay function in ms (HDC1080_delay_ms) \n
*   6- Include "HDC1080.h" in source file (include "HDC1080.h") \n
*   7- export symbol HDC1080_DRIVER (extern HDC1080_DRIVER_t HDC1080_DRIVER;) \n
* \n
*   ** Acquire temperature and humidity ** 
*   ========================================= 
*   1- Run the HDC1080_DRIVER.SetConfig(ConfigValue) function with the parameter ConfigValue.mode = TEMP_AND_HUM \n
*   2- Run ret = HDC1080_DRIVER.ReadTempAndHumidity(&Temperature,&Humidity) \n
*   3- Function is blocking for at least 16 ms  \n
*   4- if(ret == HDC1080_RET_OK) The values of temperature and humidity will be in the input variables  \n
*  \n
*   ** Acquire temperature only** 
*   ===============================  
*   1- Run the HDC1080_DRIVER.SetConfig(ConfigValue) function with the parameter ConfigValue.mode = TEMP_OR_HUM \n
*   2- Run ret = HDC1080_DRIVER.ReadTemperature(&Temperature) \n
*   3- Function is blocking for at least 8 ms \n
*   4- if(ret == HDC1080_RET_OK) The temperature value will be in the input variable \n
* \n
*   ** Acquire humidity only**
*   ============================
*   1- Run the HDC1080_DRIVER.SetConfig(ConfigValue) function with the parameter ConfigValue.mode = TEMP_OR_HUM \n
*   2- Run ret = HDC1080_DRIVER.ReadHumidity(&Humidity) \n
*   3- Function is blocking for at least 8 ms \n
*   4- if(ret == HDC1080_RET_OK) The humidity value will be in the input variable \n
* \endverbatim
*************************************************************************/
/** @addtogroup  HDC1080 HDC1080 
 * @{
 */
#ifndef __HDC1080_DRIVER_
#define __HDC1080_DRIVER_
#include <stdint.h>
#include <stdbool.h>

/*!
 *  \brief Return codes of the functions
 */
typedef enum{
    HDC1080_RET_OK,             /**<Successfully executed function*/
    HDC1080_ERROR,              /**<Error on i2c communication*/
    HDC1080_TIMEOUT,            /**<Timeout occurred */
    HDC1080_INV_PARAM,          /**<Parameter entered is invalid*/
}HDC1080_ret_e;
/*!
 *  \brief Software reset bit
 */
typedef enum{
    NORMAL = 0,         /**<Normal Operation, this bit self clears*/
    SOFT_RESET = 1      /**<Software Reset*/
}HDC1080_RST_e;
/*!
 *  \brief Heater
 */
typedef enum{
    HEATER_DISABLED = 0,/**<Heater Disabled*/
    HEATER_ENABLED = 1  /**<Heater Enabled*/
}HDC1080_HEAT_e;
/*!
 *  \brief Mode of acquisition
 */
typedef enum{
    TEMP_OR_HUM = 0,    /**<Temperature or Humidity is acquired.*/
    TEMP_AND_HUM = 1,   /**<Temperature and Humidity are acquired in sequence, Temperature first.*/
}HDC1080_MODE_e;
/*!
 *  \brief Battery Status
 */
typedef enum{
    BAT_OK = 0,         /**<Battery voltage > 2.8V (read only)*/
    BAT_LOW = 1,        /**<Battery voltage < 2.8V (read only)*/
}HDC1080_BTST_e;
/*!
 *  \brief Temperature Measurement Resolution
 */
typedef enum{
    TRES_14_BIT = 0,    /**<14 bit resolution*/
    TRES_11_BIT = 1     /**<11 bit resolution*/
}HDC1080_TRES_e;
/*!
 *  \brief Humidity Measurement Resolution
 */
typedef enum{
    HRES_14_BIT = 0,    /**<14 bit resolution*/
    HRES_11_BIT = 1,    /**<11 bit resolution*/
    HRES_8_BIT = 2      /**<8 bit resolution*/
}HDC1080_HRES_e;
/*!
 *  \brief This register configures device functionality and returns status.
 */
typedef struct{
    uint8_t         reserved_1:8;   /**<bit0 - bit7: Reserved, must be 0*/
    HDC1080_HRES_e  hres:2;         /**<[8:9]:Humidity Measurement Resolution \n
                                        00: 14 bit \n
                                        01: 11 bit \n
                                        10: 8 bit*/
    HDC1080_TRES_e  tres:1;         /**<[10]:Temperature Measurement Resolution \n
                                        0: 14 bit \n
                                        1: 11 bit*/
    HDC1080_BTST_e  btst:1;         /**<[11]:Battery Status \n
                                        0: Battery voltage > 2.8V (read only) \n
                                        1: Battery voltage < 2.8V (read only)*/
    HDC1080_MODE_e  mode:1;         /**<[12]:Mode of acquisition \n
                                        0: Temperature or Humidity is acquired. \n
                                        1: Temperature and Humidity are acquired in sequence, Temperature first.*/
    HDC1080_HEAT_e  heat:1;         /**<[13]:Heater \n
                                        0 Heater Disabled \n
                                        1 Heater Enabled*/
    uint8_t         reserved_2:1;   /**<[14]: Reserved, must be 0*/
    HDC1080_RST_e   rst:1;          /**<[15]:Software reset bit \n
                                        0: Normal Operation, this bit self clears \n
                                        1: Software Reset*/
}HDC1080_config_t;

/*!
 *  \brief HDC1080 Driver control block
 */
typedef struct {
    HDC1080_ret_e     (*ReadTempAndHumidity)    (int32_t* Temperature,int32_t* Humidity);   /**<Pointer to \ref HDC1080ReadTempAndHumidity : temperature and humidity values*/
    HDC1080_ret_e     (*ReadTemperature)        (int32_t* Temperature);                     /**<Pointer to \ref HDC1080ReadTemperature : get temperature value*/
    HDC1080_ret_e     (*ReadHumidity)           (int32_t* Humidity);                        /**<Pointer to \ref HDC1080ReadHumidity : get humidity value*/
    HDC1080_ret_e     (*SetConfig)              (HDC1080_config_t* ConfigValue);            /**<Pointer to \ref HDC1080SetConfig : set configuration values*/
    HDC1080_ret_e     (*GetConfig)              (HDC1080_config_t* ConfigValue);            /**<Pointer to \ref HDC1080GetConfig : get configuration values*/
    HDC1080_ret_e     (*ReadManufacturerID)     (uint16_t* ManufacturerID);                 /**<Pointer to \ref HDC1080ReadManufacturerID : get Manufacturer ID (0x5449)*/
    HDC1080_ret_e     (*ReadDeviceID)           (uint16_t* DeviceID);                       /**<Pointer to \ref HDC1080ReadDeviceID : get Device ID (0x1050)*/
} const HDC1080_DRIVER_t;

/** @}*/ //End of HDC1080
#endif //__HDC1080_DRIVER_
