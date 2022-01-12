/***********************************************************************
* $Id$		HDC1080.c         2021-08-25
*//**
* \file		HDC1080.c
* \version	1.0
* \date		25. Aug. 2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
* \brief	Source file to HDC1080 sensor
* \verbatim
*   ** Making this component functional**
*   ======================================= 
*   1- Implement write function on the i2c bus with the correct error returns (HDC1080Write) \n
*   2- Implement read function off the i2c bus with the correct error returns (HDC1080Read) \n
*   3- Implement function that returns the state of the i2c bus (HDC1080_IsI2CBusy) \n
*   4- Implement function that set the state of the i2c bus (HDC1080_MakeI2CBusy) \n
*   5- Implement a delay function in ms (HDC1080_delay_ms) \n
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

#include <string.h>
#include "HDC1080.h"

/** @weakgroup   HDC1080Weak HDC1080 Weak
 * @{
 */
__weak bool HDC1080_IsI2CBusy (void);
__weak void HDC1080_MakeI2CBusy (bool i2cState);
__weak HDC1080_ret_e HDC1080Write(uint16_t DevAddress, uint8_t *pData, uint16_t Size);
__weak HDC1080_ret_e HDC1080Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size);
__weak void  HDC1080_delay_ms (uint32_t delay);
/** @}*/ //End of HDC1080Weak

/** @addtogroup  HDC1080Private HDC1080 Private
 * @{
 */
#define HDC1080_WRITE_ADDRESS           0x80        /**<7 bit address (0b1000000) with write bit at the end*/
#define HDC1080_READ_ADDRESS            0x81        /**<7 bit address (0b1000000) with read bit at the end*/
#define HDC1080_NODELAY                 0           /**<No delay between write and read operation on i2c*/

/*!
 *  \brief Sensors conversion time
 */
typedef enum{
    RHCT8bits = 3,      /**<Relative humidity sensor conversion time (8 bit resolution) = 2.50 ms*/
    RHCT11bits = 4,     /**<Relative humidity sensor conversion time (11 bit resolution) = 3.85 ms*/
    RHCT14bits = 7,     /**<Relative humidity sensor conversion time (14 bit resolution) = 6.50 ms*/
    TEMPCT11bits = 4,   /**<Temperature sensor conversion time (11 bit resolution) = 3.65 ms*/
    TEMPCT14bits = 7    /**<Temperature sensor conversion time (14 bit resolution) = 6.35 ms*/
}HDC1080ConversionTime_e;

/*!
 *  \brief Register Map Structure
 */
typedef enum{
    TEMPERATURE_REG = 0x00,     /**<Temperature measurement outpu*/
    HUMIDITY_REG    = 0x01,     /**<Relative Humidity measurement output*/
    CONFIG_REG      = 0x02,     /**<HDC1080 configuration and status*/
    SERIAL_ID_REG_1 = 0xFB,     /**<First 2 bytes of the serial ID of the part*/
    SERIAL_ID_REG_2 = 0xFC,     /**<Mid 2 bytes of the serial ID of the part*/
    SERIAL_ID_REG_3 = 0xFD,     /**<Last byte bit of the serial ID of the part*/
    MAN_ID_REG      = 0xFE,     /**<ID of Texas Instruments*/
    DEV_ID_REG      = 0xFF      /**<ID of the device*/
}HDC1080_register_e;

/*!
 *  \brief      Function that writes a data to the selected registers
 *  \param[in]  reg: first register to receive the data 
 *  \param[in]  pData: pointer with data to be written
 *  \param[in]  size: pData vector size
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 */
static HDC1080_ret_e HDC1080WriteRegister(HDC1080_register_e reg, uint16_t *pData, uint8_t size)
{
    HDC1080_ret_e ret = HDC1080_RET_OK;
    bool i2c_blocked = false;
    
    if((reg + size) > (CONFIG_REG + 1))
    {
        ret = HDC1080_INV_PARAM;
    }
    
    if(ret == HDC1080_RET_OK)
    {
        if(HDC1080_IsI2CBusy())
        {
            ret = HDC1080_ERROR;
        }
        else                        
        {
            HDC1080_MakeI2CBusy(true);
            i2c_blocked = true;
        }
    }
    
    if(ret == HDC1080_RET_OK)
    {
        uint8_t aux_data[11];
        uint8_t index = 0;
		
        aux_data[index++] = reg;
        while(index <= size)
        {
            aux_data[index++] = (uint8_t)(*pData >> 8);
            aux_data[index++] = (uint8_t)(*pData & 0x0F);
            pData++;
        }
        ret = HDC1080Write(HDC1080_WRITE_ADDRESS,aux_data,index);
    }
    
    if(i2c_blocked)
    {
        HDC1080_MakeI2CBusy(false);
    }
    
    return ret;
}
/*!
 *  \brief      Function that reads data from selected registers
 *  \param[in]  reg: first register to read 
 *  \param[out] pData: pointer to receive register values
 *  \param[in]  size: number of registers to be read
 *  \param[in]  delay: time between (SLA + W) and (SLA + R) in ms, time nedded to complete conversion
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 */
static HDC1080_ret_e HDC1080ReadRegister(HDC1080_register_e reg, uint16_t *pData, uint8_t size, uint32_t delay)
{
    HDC1080_ret_e ret = HDC1080_RET_OK;
    uint8_t aux_data[11];
    bool i2c_blocked = false;
    
    if( (reg <= CONFIG_REG) && ((reg + size) > (CONFIG_REG + 1)) )
    {
        ret = HDC1080_INV_PARAM;
    }
    if( (reg >= SERIAL_ID_REG_1) && ((uint16_t)(reg + size) > (uint16_t)(DEV_ID_REG + 1)) )
    {
        ret = HDC1080_INV_PARAM;
    }
    
    if(ret == HDC1080_RET_OK)
    {
        if(HDC1080_IsI2CBusy())
        {
            ret = HDC1080_ERROR;
        }
        else                        
        {
            HDC1080_MakeI2CBusy(true);
            i2c_blocked = true;
        }
    }
    
    if(ret == HDC1080_RET_OK)
    {
        ret = HDC1080Write(HDC1080_WRITE_ADDRESS,(uint8_t*)&reg,1);
    }
    if(ret == HDC1080_RET_OK)
    {
        if(delay > 0)   HDC1080_delay_ms(delay);
        ret = HDC1080Read(HDC1080_READ_ADDRESS,aux_data,2*size);
    }
    if(ret == HDC1080_RET_OK)
    {
        uint8_t index = 0;
        while(index < size)
        {
            pData[index] = (aux_data[2*index]<<8) + aux_data[2*index + 1];
            index++;
        }
    }
    
    if(i2c_blocked)
    {
        HDC1080_MakeI2CBusy(false);
    }

    return ret;
}
/** @}*/ //End of HDC1080Private

/** @addtogroup  HDC1080Public HDC1080 Public
 * @{
 */
/*!
 *  \brief      Function to get temperature and humidity from HDC1080 sensor
 *  \param[out] Temperature: pointer to receive temperature value (degree Celsius/10)
 *  \param[out] Humidity: pointer to receive humidity value (%)
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \warning    bit "mode" from configuration register must be on TEMP_AND_HUM value
 *  \note       This function expects the maximum resolution conversion time (TEMPCT14bits + RHCT14bits), 
 *              it is possible to implement a variable wait according to the resolution 
 *              using the structure HDC1080ConversionTime_e
 */
HDC1080_ret_e HDC1080ReadTempAndHumidity(int32_t *Temperature,int32_t *Humidity)
{
    HDC1080_ret_e ret;
    uint16_t data[4];
    
    ret = HDC1080WriteRegister(TEMPERATURE_REG,NULL,HDC1080_NODELAY);
    if(ret == HDC1080_RET_OK)
    {
        ret = HDC1080ReadRegister(TEMPERATURE_REG,data,2,TEMPCT14bits + RHCT14bits);
    }
    
    if(ret == HDC1080_RET_OK)
    {
        *Temperature = ((data[0] * 1650) >> 16) - 400;
        *Humidity =  (data[1] * 100) >> 16;
    }

    return ret;
}
/*!
 *  \brief      Function to get temperature from HDC1080 sensor
 *  \param[out] Temperature: pointer to receive temperature value (degree Celsius/10)
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \warning    bit "mode" from configuration register must be on TEMP_OR_HUM value
 *  \note       This function expects the maximum resolution conversion time (TEMPCT14bits), 
 *              it is possible to implement a variable wait according to the resolution 
 *              using the structure HDC1080ConversionTime_e
 */
HDC1080_ret_e HDC1080ReadTemperature(int32_t* Temperature)
{
    HDC1080_ret_e ret;
    uint16_t reg_value;
    
    ret = HDC1080ReadRegister(TEMPERATURE_REG,&reg_value,1,TEMPCT14bits);
    if(ret == HDC1080_RET_OK)
    {
        *Temperature = ((reg_value * 1650) >> 16) - 400;
    }
    return ret;
}
/*!
 *  \brief      Function to get humidity from HDC1080 sensor
 *  \param[out] Humidity: pointer to receive humidity value (%)
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \warning    bit "mode" from configuration register must be on TEMP_OR_HUM value
 *  \note       This function expects the maximum resolution conversion time (RHCT14bits), 
 *              it is possible to implement a variable wait according to the resolution 
 *              using the structure HDC1080ConversionTime_e
 */
HDC1080_ret_e HDC1080ReadHumidity(int32_t* Humidity)
{
    HDC1080_ret_e ret;
    uint16_t reg_value;
    
    ret = HDC1080ReadRegister(TEMPERATURE_REG,&reg_value,1,RHCT14bits);
    if(ret == HDC1080_RET_OK)
    {
        *Humidity =  ((int32_t)reg_value * 100) >> 16;
    }
    return ret;
}
/*!
 *  \brief      Function to get configuration values from HDC1080 sensor
 *  \param[out] ConfigValue: pointer to receive configuration values (HDC1080_config_t structure)
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 */
HDC1080_ret_e HDC1080GetConfig(HDC1080_config_t* ConfigValue)
{
    HDC1080_ret_e ret;
    uint16_t config;
    ret = HDC1080ReadRegister(CONFIG_REG,&config,1,HDC1080_NODELAY);
    if(ret == HDC1080_RET_OK)
    {
        memcpy(ConfigValue,&config,sizeof(uint16_t));
    }
    return ret;
}
/*!
 *  \brief      Function to set configuration values from HDC1080 sensor
 *  \param[in]  ConfigValue: pointer to configuration values (HDC1080_config_t structure)
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 */
HDC1080_ret_e HDC1080SetConfig(HDC1080_config_t* ConfigValue)
{
    HDC1080_ret_e ret;
    HDC1080_config_t reg;
    uint16_t config;
    
    HDC1080GetConfig(&reg);
    reg.rst = ConfigValue->rst;
    reg.heat = ConfigValue->heat;
    reg.mode = ConfigValue->mode;
    reg.tres = ConfigValue->tres;
    reg.hres = ConfigValue->hres;
    
    memcpy(&config,ConfigValue,sizeof(uint16_t));
    ret = HDC1080WriteRegister(CONFIG_REG,&config,1);

    return ret;
}
/*!
 *  \brief      Function to get Manufacturer ID
 *  \param[out] ManufacturerID: pointer to receive Manufacturer ID value
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred 
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \note       ID of Texas Instruments: 0x5449
 */
HDC1080_ret_e HDC1080ReadManufacturerID(uint16_t* ManufacturerID)
{
    HDC1080_ret_e ret;
    uint16_t id;
    
    ret = HDC1080ReadRegister(MAN_ID_REG,&id,1,HDC1080_NODELAY);
    
    if(ret == HDC1080_RET_OK)
    {
        *ManufacturerID = id;
    }
    
    return ret;
}
/*!
 *  \brief      Function to get Device ID
 *  \param[out] DeviceID: pointer to receive Device ID value
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \note       ID of the device: 0x1050
 */
HDC1080_ret_e HDC1080ReadDeviceID(uint16_t* DeviceID)
{
    HDC1080_ret_e ret;
    uint16_t id;
    
    ret = HDC1080ReadRegister(DEV_ID_REG,&id,1,HDC1080_NODELAY);
    if(ret == HDC1080_RET_OK)
    {
        *DeviceID = id;
    }
    
    return ret;
}
/*!
 *  \brief Access structure of the HDC1080 Driver
 */
HDC1080_DRIVER_t HDC1080_DRIVER = {
    HDC1080ReadTempAndHumidity,
    HDC1080ReadTemperature,
    HDC1080ReadHumidity,
    HDC1080SetConfig,
    HDC1080GetConfig,
    HDC1080ReadManufacturerID,
    HDC1080ReadDeviceID
};
 /** @}*/ //End of HDC1080Public

/** @weakgroup   HDC1080Weak HDC1080 Weak
 *  @ingroup HDC1080 HDC1080
 * @{
 */
/*!
 *  \brief      Function that writes the i2c bus
 *  \param[in]  DevAddress: Address of the device to be accessed (always HDC1080_WRITE_ADDRESS)
 *  \param[in]  pData: pointer with the data to be written
 *  \param[in]  Size: number of bytes to be written
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \warning    Stronger function must be declared externally
 */
__weak HDC1080_ret_e HDC1080Write(uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{    
    ((void)(DevAddress));
    ((void)(pData));
    ((void)(Size));
    return HDC1080_ERROR;
}
/*!
 *  \brief      Function that reads from i2c bus
 *  \param[in]  DevAddress: Address of the device to be accessed (always HDC1080_READ_ADDRESS)
 *  \param[in]  pData: pointer where the read data will be stored
 *  \param[in]  Size: number of bytes to be read
 *  \return     Result of operation
 *  \retval     HDC1080_RET_OK: Successfully executed function
 *  \retval     HDC1080_ERROR: Error on i2c communication
 *  \retval     HDC1080_TIMEOUT: Timeout occurred
 *  \retval     HDC1080_INV_PARAM: Parameter entered is invalid
 *  \warning    Stronger function must be declared externally
 */
__weak HDC1080_ret_e HDC1080Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{
    ((void)(DevAddress));
    ((void)(pData));
    ((void)(Size));
    return HDC1080_ERROR;
}
/*!
 *  \brief      Function that returns the state of the i2c bus
 *  \return     State of the i2c bus
 *  \retval     0: i2c bus is not busy
 *  \retval     1: i2c bus is busy
 *  \warning    Stronger function must be declared externally
 */
__weak bool HDC1080_IsI2CBusy (void)
{
    return true;
}
/*!
 *  \brief      Function that set the state of the i2c bus
 *  \param[in]  i2cState: state of the i2c bus [false: i2c not busy | true: i2c is busy]
 *  \warning    Stronger function must be declared externally
 */
__weak void HDC1080_MakeI2CBusy (bool i2cState)
{
    ((void)(i2cState));
}
/*!
 *  \brief      Function to wait a specified time in milliseconds
 *  \param[in]  delay: wait time in milliseconds
 *  \warning    Stronger function must be declared externally
 */
__weak void  HDC1080_delay_ms (uint32_t delay)
{
    ((void)(delay));
}
 /** @}*/ //End of HDC1080Weak
