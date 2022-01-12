/*!
 * \file      AT24Cxx.c
 * \author    Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \brief     EEPROM AT24Cxx write and read driver source file
 * \version   2.1
 * \date      2020-12-14
 * \copyright Copyright (c) 2020
 *
 */
#include "AT24Cxx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/** \weakgroup   AT24CxxWeak AT24Cxx Weak
 *  \ingroup AT24Cxx
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak)) /**<Ensures that weak attribute is defined*/
#endif

__weak uint32_t       AT24CxxGetTick(void);
__weak AT24Cxx_RETURN AT24CxxI2cWrite(uint16_t DevAddress, uint8_t *pData, uint16_t Size);
__weak AT24Cxx_RETURN AT24CxxI2cRead(uint16_t DevAddress, uint8_t *pData, uint16_t Size);
__weak AT24Cxx_RETURN AT24CxxI2cTake(void);
__weak AT24Cxx_RETURN AT24CxxI2cGive(void);
__weak void           AT24CxxOsDelay(uint32_t delay);
/** @}*/ // End of AT24Cxxweak

/** \addtogroup  AT24CxxPrivate AT24Cxx Private
 *  \ingroup AT24Cxx
 * @{
 */
#define SELF_TIMED_WRITE_CYCLE   15   /**<15 ms to end a write cycle*/
#define EEPROM_READ_ADDRESS_BIT  0x01 /**<Read bit*/
#define EEPROM_WRITE_ADDRESS_BIT 0x00 /**<Write bit*/
#define MAX_PAGE_SIZE            16   /**<Maximum eeprom page size*/

uint32_t AT24cxxTimestamp; /**<Timestamp to control write cycle*/

/*!
 *  \brief      Function that calculates the elapsed time from an initial time
 *    \param[in]  InitialTime: Initial time for calculation
 *  \return     Elapsed time from initial time in milliseconds
 *  \note       This function corrects the error caused by overflow
 */
uint32_t AT24CxxGetElapsedTime(uint32_t InitialTime)
{
    uint32_t actualTime;
    actualTime = AT24CxxGetTick();

    if (InitialTime <= actualTime)
        return (actualTime - InitialTime);
    else
        return (((0xFFFFFFFFUL) - InitialTime) + actualTime);
}
/*!
 *  \brief  Start the write cycle counter, must be executed
 *          after every write operation
 */
void AT24cxxStartWriteCycle()
{
    AT24cxxTimestamp = AT24CxxGetTick();
}
/*!
 *  \brief  This function check if any write operation is in progress
 *          if returns TRUE, operations cant be performed in eeprom
 *  \return Result of the operation
 *  \retval TRUE: Write operation is in progress
 *  \retval FALSE: No write operation is in progress
 */
bool AT24CxxWriteCycleInProgress()
{
    if (AT24CxxGetElapsedTime(AT24cxxTimestamp) > SELF_TIMED_WRITE_CYCLE)
        return false;
    else
        return true;
}
/*!
 *  \brief Structure with information about the type of memory selected
 */
typedef struct
{
    uint8_t AddressInputs_A2A1A0; /**<Portion of the address based on the connecting pins A1, A2 and A3*/
    uint8_t PageSize;             /**<Memory page size*/
    uint8_t NumberOfPages;        /**<Number of pages in memory*/
    uint8_t AddressMask;          /**<Mask representing the bits of the chip address that will be used in memory addressing*/
    uint8_t BaseAddress;          /**<Base address of the chip, including hardware pin connection*/
    uint8_t SizeK;                /**<Memory size in kB*/
} AT24CxxConfig_t;

AT24CxxConfig_t AT24CxxConfig; /**<Initialized memory information*/
/** @}*/                       // End of AT24CxxPrivate

/** \ingroup  AT24Cxx AT24Cxx
 * @{
 */
/*!
 *  \brief  This function check if any write operation is in progress
 *          if returns TRUE, operations cant be performed in eeprom
 *  \return Result of the operation
 *  \retval true: Write operation is in progress
 *  \retval false: No write operation is in progress
 */
bool AT24CxxBusy()
{
    return AT24CxxWriteCycleInProgress();
}
/*!
 *  \brief      This function is used to initialize the driver with the memory parameters used
 *  \param[in]  resources: memory settings, a AT24CxxResources_t data structure
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK: the function was executed succesfully
 *  \retval     AT24Cxx_PARAM_INV: invalid parameter
 */
AT24Cxx_RETURN AT24CxxInitialize(AT24CxxResources_t *resources)
{
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;

    if (resources->Model == AT24C01)
    {
        if ((resources->A2 == AT24Cxx_HIG_IMP) || (resources->A1 == AT24Cxx_HIG_IMP) || (resources->A0 == AT24Cxx_HIG_IMP))
        {
            ret = AT24Cxx_PARAM_INV;
        }

        AT24CxxConfig.SizeK                = 1;
        AT24CxxConfig.AddressInputs_A2A1A0 = ((resources->A2 == AT24Cxx_VCC) << 3);
        AT24CxxConfig.AddressInputs_A2A1A0 |= ((resources->A1 == AT24Cxx_VCC) << 2);
        AT24CxxConfig.AddressInputs_A2A1A0 |= ((resources->A0 == AT24Cxx_VCC) << 1);
        AT24CxxConfig.PageSize      = 8;
        AT24CxxConfig.NumberOfPages = 16;
        AT24CxxConfig.AddressMask   = 0x0000;
    }
    else if (resources->Model == AT24C02)
    {
        if ((resources->A2 == AT24Cxx_HIG_IMP) || (resources->A1 == AT24Cxx_HIG_IMP) || (resources->A0 == AT24Cxx_HIG_IMP))
        {
            ret = AT24Cxx_PARAM_INV;
        }
        AT24CxxConfig.SizeK                = 2;
        AT24CxxConfig.AddressInputs_A2A1A0 = ((resources->A2 == AT24Cxx_VCC) << 3);
        AT24CxxConfig.AddressInputs_A2A1A0 |= ((resources->A1 == AT24Cxx_VCC) << 2);
        AT24CxxConfig.AddressInputs_A2A1A0 |= ((resources->A0 == AT24Cxx_VCC) << 1);
        AT24CxxConfig.PageSize      = 8;
        AT24CxxConfig.NumberOfPages = 32;
        AT24CxxConfig.AddressMask   = 0x0000;
    }
    else if (resources->Model == AT24C04)
    {
        if ((resources->A2 == AT24Cxx_HIG_IMP) || (resources->A1 == AT24Cxx_HIG_IMP))
        {
            ret = AT24Cxx_PARAM_INV;
        }
        AT24CxxConfig.SizeK                = 4;
        AT24CxxConfig.AddressInputs_A2A1A0 = ((resources->A2 == AT24Cxx_VCC) << 3);
        AT24CxxConfig.AddressInputs_A2A1A0 |= ((resources->A1 == AT24Cxx_VCC) << 2);
        AT24CxxConfig.PageSize      = 16;
        AT24CxxConfig.NumberOfPages = 32;
        AT24CxxConfig.AddressMask   = 0x0002;
    }
    else if (resources->Model == AT24C08)
    {
        if (resources->A2 == AT24Cxx_HIG_IMP)
        {
            ret = AT24Cxx_PARAM_INV;
        }
        AT24CxxConfig.SizeK                = 8;
        AT24CxxConfig.AddressInputs_A2A1A0 = ((resources->A2 == AT24Cxx_VCC) << 3);
        AT24CxxConfig.PageSize             = 16;
        AT24CxxConfig.NumberOfPages        = 64;
        AT24CxxConfig.AddressMask          = 0x0006;
    }
    else if (resources->Model == AT24C16)
    {
        AT24CxxConfig.SizeK                = 16;
        AT24CxxConfig.AddressInputs_A2A1A0 = 0;
        AT24CxxConfig.PageSize             = 16;
        AT24CxxConfig.NumberOfPages        = 128;
        AT24CxxConfig.AddressMask          = 0x000E;
    }
    else
    {
        ret = AT24Cxx_PARAM_INV;
    }

    if (ret == AT24Cxx_RET_OK)
    {
        AT24CxxConfig.BaseAddress = (0xA0 | AT24CxxConfig.AddressInputs_A2A1A0); // EEPROM address based on pin connection
    }
    else
    {
        AT24cxxTimestamp          = AT24CxxGetTick();
        AT24CxxConfig.BaseAddress = 0;
    }

    return ret;
}
/*!
 *  \brief      This function is used to uninitialize the memory driver
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK: the function was executed succesfully
 */
AT24Cxx_RETURN AT24CxxUnitialize(void)
{
    memset(&AT24CxxConfig, 0, sizeof(AT24CxxConfig));
    return AT24Cxx_RET_OK;
}
/*!
 *  \brief      This function is used to write a byte on i2c EEPROM
 *  \param[in]  addr: address for writing data
 *  \param[in]  data: data to write
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK: the function was executed succesfully
 *  \retval     AT24Cxx_ADDR_INV: input address  is invalid
 *  \retval     AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval     AT24Cxx_I2C_ERROR: i2c handler error
 *  \retval     AT24Cxx_NOT_INITIALIZED: driver not initialized, run the Initialize function
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 */
AT24Cxx_RETURN AT24CxxWriteByte(uint16_t addr, uint8_t data)
{
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;

    do
    {
        uint8_t device_address;
        uint8_t i2c_buffer[2];

        if (AT24CxxConfig.BaseAddress == 0)
        {
            ret = AT24Cxx_NOT_INITIALIZED;
            break;
        }
        if (addr >= (AT24CxxConfig.NumberOfPages * AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_ADDR_INV;
            break;
        }
        if (AT24CxxWriteCycleInProgress() == true)
        {
            ret = AT24Cxx_WR_IN_PROGRESS;
            break;
        }

        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_WRITE_ADDRESS_BIT;
        i2c_buffer[0] = (uint8_t)(addr & 0x00FF);
        i2c_buffer[1] = data;

        if (AT24CxxI2cTake() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_TAKE_ERROR;
            break;
        }

        ret = AT24CxxI2cWrite(device_address, i2c_buffer, 2);

        if (AT24CxxI2cGive() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_GIVE_ERROR;
            break;
        }

        AT24cxxStartWriteCycle();

    } while (0);

    return ret;
}
/*!
 *  \brief      This function is used to read a byte from i2c EEPROM
 *  \param[in]  addr: address for reading data
 *  \param[out] data: pointer that will return data readed
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK: the function was executed succesfully
 *  \retval     AT24Cxx_ADDR_INV: input address  is invalid
 *  \retval     AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval     AT24Cxx_I2C_ERROR: i2c handler error
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 */
AT24Cxx_RETURN AT24CxxReadByte(uint16_t addr, uint8_t *data)
{
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;
    uint8_t        device_address;
    uint8_t        i2c_buffer[3];

    do
    {
        if (AT24CxxConfig.BaseAddress == 0)
        {
            ret = AT24Cxx_NOT_INITIALIZED;
            break;
        }
        if (addr >= (AT24CxxConfig.NumberOfPages * AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_ADDR_INV;
            break;
        }
        if (AT24CxxWriteCycleInProgress() == true)
        {
            ret = AT24Cxx_WR_IN_PROGRESS;
            break;
        }
        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_WRITE_ADDRESS_BIT;
        i2c_buffer[0] = (uint8_t)(addr & 0x00FF);

        if (AT24CxxI2cTake() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_TAKE_ERROR;
            break;
        }

        ret = AT24CxxI2cWrite(device_address, i2c_buffer, 1);

        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_READ_ADDRESS_BIT;

        ret = AT24CxxI2cRead(device_address, i2c_buffer, 1);

        if (AT24CxxI2cGive() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_GIVE_ERROR;
            break;
        }

        *data = i2c_buffer[0];

    } while (0);

    return ret;
}
/*!
 *  \brief      This function is used to write on i2c EEPROM
 *  \param[in]  addr: address for writing data
 *  \param[in]  data: pointer containing data to be written
 *  \param[in]  size: data size to be written
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK:     the function was executed succesfully
 *  \retval     AT24Cxx_ADDR_INV:    input address  is invalid
 *  \retval     AT24Cxx_SIZE_INV:    size is over the limit
 *  \retval     AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval     AT24Cxx_I2C_ERROR: i2c handler error
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 */
AT24Cxx_RETURN AT24CxxWrite(uint16_t addr, uint8_t *data, uint16_t size)
{
    static uint8_t i2c_buffer[MAX_PAGE_SIZE + 3];
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;
    uint8_t        device_address;
    int16_t        bytes_to_write   = size;
    uint8_t        page_index       = (uint8_t)(addr / AT24CxxConfig.PageSize);
    uint8_t        first_page_bytes = (uint16_t)(AT24CxxConfig.PageSize * (page_index + 1)) - addr;
    uint32_t       I2CWriteL;

    do
    {
        if (AT24CxxConfig.BaseAddress == 0)
        {
            ret = AT24Cxx_NOT_INITIALIZED;
            break;
        }
        // Returns error if the input address is greater than last device address
        if (addr >= (AT24CxxConfig.NumberOfPages * AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_ADDR_INV;
            break;
        }

        uint8_t last_page_bytes = 0;
        uint8_t pages_to_write  = 0;
        // Returns error if the last page number is greater than NumberOfPages
        if (size < first_page_bytes)
            first_page_bytes = size;
        last_page_bytes = (size - (uint16_t)first_page_bytes) % AT24CxxConfig.PageSize;
        pages_to_write  = (uint8_t)((size - first_page_bytes - last_page_bytes) / AT24CxxConfig.PageSize) + ((first_page_bytes > 0) ? 1 : 0) +
                         ((last_page_bytes > 0) ? 1 : 0);
        if (((pages_to_write + page_index) > AT24CxxConfig.NumberOfPages))
        {
            ret = AT24Cxx_SIZE_INV;
            break;
        }

        if (AT24CxxWriteCycleInProgress() == true)
        {
            ret = AT24Cxx_WR_IN_PROGRESS;
            break;
        }

        // Set the first write length to complete the first page
        I2CWriteL      = first_page_bytes + 1;
        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_WRITE_ADDRESS_BIT;
        i2c_buffer[0] = (uint8_t)(addr & 0x00FF);

        while ((bytes_to_write > 0) && (ret == AT24Cxx_RET_OK))
        {
            uint16_t addr_index;
            while (AT24CxxWriteCycleInProgress() == true)
            {
                AT24CxxOsDelay(1);
            }
            memcpy((void *)&i2c_buffer[1], &data[size - bytes_to_write], I2CWriteL - 1);

            if (AT24CxxI2cTake() != AT24Cxx_RET_OK)
            {
                ret = AT24Cxx_I2C_TAKE_ERROR;
                break;
            }

            ret = AT24CxxI2cWrite(device_address, i2c_buffer, I2CWriteL); // Execute critical session

            if (AT24CxxI2cGive() != AT24Cxx_RET_OK)
            {
                ret = AT24Cxx_I2C_GIVE_ERROR;
                break;
            }
            bytes_to_write = bytes_to_write - (I2CWriteL - 1);
            if (bytes_to_write > AT24CxxConfig.PageSize)
            {
                I2CWriteL = AT24CxxConfig.PageSize + 1;
            }
            else if (bytes_to_write > 0)
            {
                I2CWriteL = bytes_to_write + 1;
            }
            page_index++;
            addr_index = (uint16_t)(page_index * AT24CxxConfig.PageSize);

            device_address &= ~AT24CxxConfig.AddressMask;                                 // reset memory page address bits
            device_address |= ((uint8_t)((addr_index >> 7) & AT24CxxConfig.AddressMask)); // set new memory page address bits
            i2c_buffer[0] = (uint8_t)(addr_index & 0x00FF);

            AT24cxxStartWriteCycle();
        }
    } while (0);

    return ret;
}
/*!
 *  \brief      This function is used to write on i2c EEPROM, limited to 1 page
 *  \param[in]  addr: address for writing data
 *  \param[in]    data: pointer containing data to be written
 *    \param[in]    size: data size to be written
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK:     the function was executed succesfully
 *  \retval     AT24Cxx_ADDR_INV:    input address  is invalid
 *  \retval     AT24Cxx_SIZE_INV:    size is over the limit
 *  \retval     AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval     AT24Cxx_I2C_ERROR: i2c handler error
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 */
AT24Cxx_RETURN AT24CxxWritePage(uint16_t addr, uint8_t *data, uint16_t size)
{
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;

    do
    {
        if (AT24CxxConfig.BaseAddress == 0)
        {
            ret = AT24Cxx_NOT_INITIALIZED;
            break;
        }
        // Returns error if the input address is greater than last device address
        if (addr >= (AT24CxxConfig.NumberOfPages * AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_ADDR_INV;
            break;
        }
        // Returns error if the size takes up more than one page
        if ((addr / AT24CxxConfig.PageSize) != ((addr + size) / AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_SIZE_INV;
            break;
        }
        if (AT24CxxWriteCycleInProgress() == true)
        {
            ret = AT24Cxx_WR_IN_PROGRESS;
            break;
        }

        uint8_t        device_address;
        static uint8_t i2c_buffer[MAX_PAGE_SIZE + 3];
        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_WRITE_ADDRESS_BIT;
        i2c_buffer[0] = (uint8_t)(addr & 0x00FF);
        memcpy((void *)&i2c_buffer[1], data, size);

        if (AT24CxxI2cTake() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_TAKE_ERROR;
            break;
        }

        ret = AT24CxxI2cWrite(device_address, i2c_buffer, size + 1); // Execute critical session

        if (AT24CxxI2cGive() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_GIVE_ERROR;
            break;
        }

        AT24cxxStartWriteCycle();

    } while (0);

    return ret;
}
/*!
 *  \brief      This function is used to read i2c EEPROM
 *  \param[in]  addr: address for reading data
 *  \param[out]    data: pointer that will return data readed
 *    \param[in]    size: data size to be read (min 3)
 *  \return     Result of the operation
 *  \retval     AT24Cxx_RET_OK: the function was executed succesfully
 *  \retval     AT24Cxx_ADDR_INV: input address  is invalid
 *  \retval     AT24Cxx_SIZE_INV: size is smaller than minimum required to I2C2_Engine
 *  \retval     AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval     AT24Cxx_I2C_ERROR: i2c handler error
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 *    \warning    Size of data buffer must be at last 3
 */
AT24Cxx_RETURN AT24CxxRead(uint16_t addr, uint8_t *data, uint16_t size)
{
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;
    uint8_t        device_address;

    do
    {
        if (AT24CxxConfig.BaseAddress == 0)
        {
            ret = AT24Cxx_NOT_INITIALIZED;
            break;
        }
        // Returns error if the input address is greater than last device address
        if (addr >= (AT24CxxConfig.NumberOfPages * AT24CxxConfig.PageSize))
        {
            ret = AT24Cxx_ADDR_INV;
            break;
        }
        if (AT24CxxWriteCycleInProgress() == true)
        {
            ret = AT24Cxx_WR_IN_PROGRESS;
            break;
        }
        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_WRITE_ADDRESS_BIT;
        data[0] = (uint8_t)(addr & 0x00FF);

        if (AT24CxxI2cTake() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_TAKE_ERROR;
            break;
        }

        ret = AT24CxxI2cWrite(device_address, data, 1); // Init of the critical session

        device_address = AT24CxxConfig.BaseAddress;
        device_address |= ((uint8_t)((addr >> 7) & AT24CxxConfig.AddressMask));
        device_address |= EEPROM_READ_ADDRESS_BIT;
        ret = AT24CxxI2cRead(device_address, data, size);

        if (AT24CxxI2cGive() != AT24Cxx_RET_OK)
        {
            ret = AT24Cxx_I2C_GIVE_ERROR;
            break;
        }

    } while (0);

    return ret;
}
/*!
 *  \brief  This function is used to erase all EEPROM
 *  \return Result of the operation
 *  \retval AT24Cxx_RET_OK:     the function was executed succesfully
 *  \retval AT24Cxx_ADDR_INV:    input address  is invalid
 *  \retval AT24Cxx_SIZE_INV:    size is over the limit
 *  \retval AT24Cxx_WR_IN_PROGRESS: write operation in progress, can't write or read
 *  \retval AT24Cxx_I2C_ERROR: i2c handler error
 */
AT24Cxx_RETURN AT24CxxEraseAll(void)
{
    static uint8_t erase_buffer[MAX_PAGE_SIZE];
    uint16_t       page;
    AT24Cxx_RETURN ret = AT24Cxx_RET_OK;

    if (AT24CxxConfig.BaseAddress == 0)
    {
        ret = AT24Cxx_NOT_INITIALIZED;
    }

    for (uint16_t i = 0; i < AT24CxxConfig.PageSize; i++)
    {
        erase_buffer[i] = 0xFF;
    }

    for (page = 0; page < AT24CxxConfig.NumberOfPages; page++)
    {
        uint16_t addr;
        if (ret != AT24Cxx_RET_OK)
        {
            break;
        }
        addr = (uint16_t)(page * AT24CxxConfig.PageSize);
        while (AT24CxxWriteCycleInProgress() == true)
        {
            AT24CxxOsDelay(1);
        }
        ret = AT24CxxWrite(addr, &erase_buffer[0], AT24CxxConfig.PageSize);
    }

    return ret;
}
/*!
 *  \brief Access structure of the AT24Cxx Driver
 */
AT24Cxx_DRIVER_t AT24Cxx_DRIVER = {AT24CxxInitialize, AT24CxxUnitialize, AT24CxxWriteByte, AT24CxxReadByte, AT24CxxWrite,
                                   AT24CxxWritePage,  AT24CxxRead,       AT24CxxEraseAll,  AT24CxxBusy};
/** @}*/ // End of AT24Cxx

/*!
 *  \brief      Function to get the current value of a tick variable (uint32_t) that is incremented every 1 ms
 *  \return     Tick value
 *  \warning    Stronger function must be declared externally
 */
__weak uint32_t AT24CxxGetTick(void)
{
    return 1;
}
/*!
 *  \brief      Function that writes the i2c bus
 *  \param[in]  DevAddress: Address of the device to be accessed (always HDC1080_WRITE_ADDRESS)
 *  \param[in]  pData: pointer with the data to be written
 *  \param[in]  Size: number of bytes to be written
 *  \return     Result of operation
 *  \retval     AT24Cxx_RET_OK: Successfully executed function
 *  \retval     AT24Cxx_I2C_ERROR: Error on i2c communication
 *  \warning    Stronger function must be declared externally
 */
__weak AT24Cxx_RETURN AT24CxxI2cWrite(uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{
    return AT24Cxx_I2C_ERROR;
}
/*!
 *  \brief      Function that reads from i2c bus
 *  \param[in]  DevAddress: Address of the device to be accessed (always HDC1080_READ_ADDRESS)
 *  \param[in]  pData: pointer where the read data will be stored
 *  \param[in]  Size: number of bytes to be read
 *  \return     Result of operation
 *  \retval     AT24Cxx_RET_OK: Successfully executed function
 *  \retval     AT24Cxx_I2C_ERROR: Error on i2c communication
 *  \warning    Stronger function must be declared externally
 */
__weak AT24Cxx_RETURN AT24CxxI2cRead(uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{
    return AT24Cxx_I2C_ERROR;
}
/*!
 *  \brief      Delay a task for a given number of ticks
 *  \param[in]  delay: the amount of time, in tick periods, that the calling task should block
 *  \warning    Stronger function must be declared externally
 */
__weak void AT24CxxOsDelay(uint32_t delay)
{
}

/*!
 *  \brief      Obtain a mutex
 *  \return     Result of operation
 *  \retval     AT24Cxx_RET_OK: mutex successfully obtained
 *  \retval     AT24Cxx_I2C_TAKE_ERROR: error in obtaining the mutex
 *  \warning    Stronger function must be declared externally
 */
__weak AT24Cxx_RETURN AT24CxxI2cTake(void)
{
    return AT24Cxx_RET_OK;
}

/*!
 *  \brief      Release a mutex
 *  \return     Result of operation
 *  \retval     AT24Cxx_RET_OK: mutex successfully released
 *  \retval     AT24Cxx_I2C_GIVE_ERROR: mutex release error
 *  \warning    Stronger function must be declared externally
 */
__weak AT24Cxx_RETURN AT24CxxI2cGive(void)
{
    return AT24Cxx_RET_OK;
}
