/*!
 * \file       EventSST2xVF.c
 * \brief      Event Manager using the STT2xVF memory
 * \date       2021-09-14
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 */
#include "EventSST2xVF.h"
#include "SST2xVF.h"

/*! \addtogroup EventSST2xVFPrivate Event SST2xVF Private
 *  \ingroup EventSST2xVF
 * @{
 */

extern SST2xVF_DRIVER_t SST2xVF_DRIVER; /*!<SST2xVF FLASH driver*/

/*!
 * \brief       Structure to check if the user wants to use part of the memory
 */
static struct UserDefinedArea
{
    bool     user_defined;
    uint32_t first_addr;
    size_t   total_size;
} UserConfig = {0};

static bool EventSST2xVFInit(void);
static bool EventSST2xVFConfig(EventInfo_t *info);
static bool EventSST2xVFEraseAll(void);
static bool EventSST2xVFEraseSector(uint32_t addr);
static bool EventSST2xVFRead(uint32_t addr, uint8_t *data, uint32_t size);
static bool EventSST2xVFWrite(uint32_t addr, uint8_t *data, uint32_t size);
/*!
 * \brief       Instance of the SST2xVF access functions
 */
const EventMemoryInterface_t SST2xVF_EventFuncs = {
    .InitFunc        = EventSST2xVFInit,        //
    .ConfigInfoFunc  = EventSST2xVFConfig,      //
    .EraseAllFunc    = EventSST2xVFEraseAll,    //
    .EraseSectorFunc = EventSST2xVFEraseSector, //
    .ReadFunc        = EventSST2xVFRead,        //
    .WriteFunc       = EventSST2xVFWrite,       //
};

/*! @}*/ // End of EventSST2xVFPrivate

/*!
 * \brief       Returns the SST2xVF interface functions
 * \return      \ref EventMemoryInterface_t
 */
const EventMemoryInterface_t *EventSST2xVFGetInterface(void)
{
    return &SST2xVF_EventFuncs;
}

/*!
 * \brief       Sets the memory dependant information in the Event Info structure
 * \param[out]  info: The Event information to be used in the Event Manager
 * \retval      true: If the information is correctly configured
 * \retval      false: If some information is not correct
 */
static bool EventSST2xVFConfig(EventInfo_t *info)
{
    bool ret = true;

    do
    {
        if ((info->MaxPointer + 1) == 0)
        {
            info->MaxPointer        = SST2xVF_DRIVER.GetInfo()->size - 1;
            UserConfig.user_defined = false;
        }
        else
        {
            UserConfig.user_defined = true;
        }

        if (info->MaxPointer > SST2xVF_DRIVER.GetInfo()->size)
        {
            ret = false;
            break;
        }
        if (info->FirstPointer % SST2xVF_DRIVER.GetInfo()->sector_size)
        {
            ret = false;
            break;
        }
        if ((info->MaxPointer + 1) % SST2xVF_DRIVER.GetInfo()->sector_size)
        {
            ret = false;
            break;
        }
        UserConfig.first_addr = info->FirstPointer;
        UserConfig.total_size = (info->MaxPointer - info->FirstPointer + 1);
        info->LogsPerSector   = SST2xVF_DRIVER.GetInfo()->sector_size / info->EventSize;
        info->MaxLogsNumber   = UserConfig.total_size / info->EventSize;
        info->SectorSize      = SST2xVF_DRIVER.GetInfo()->sector_size;
        info->ManID           = SST2xVF_DRIVER.GetInfo()->man_id;
        info->DevID           = SST2xVF_DRIVER.GetInfo()->dev_id;

    } while (0);

    return ret;
}

/*!
 * \brief       Initializes the SST2xVF memory
 * \retval      true: The memory was initialized correctly
 * \retval      false: An error occurred
 */
static bool EventSST2xVFInit(void)
{
    bool ret = true;

    if (SST2xVF_DRIVER.Initialize() != SST2xVF_RET_OK)
    {
        ret = false;
    }

    return ret;
}

/*!
 * \brief       Erases all the the SST2xVF sectors used by the Event Manager
 * \retval      true: The target memory area was erased correctly
 * \retval      false: An error occurred
 */
static bool EventSST2xVFEraseAll(void)
{
    bool ret = true;

    if (UserConfig.user_defined == true)
    {
        // TODO Verificar se os setores foram apagados corretamente
        for (uint32_t i = UserConfig.first_addr; i <= (UserConfig.first_addr + UserConfig.total_size); i += SST2xVF_DRIVER.GetInfo()->sector_size)
        {
            if (SST2xVF_DRIVER.EraseSector(i) != SST2xVF_RET_OK)
            {
                ret = false;
                break;
            }
        }
    }
    else if (SST2xVF_DRIVER.EraseChip() != SST2xVF_RET_OK)
    {
        ret = false;
    }

    return ret;
}

/*!
 * \brief       Erase the sector that contains the address
 * \param[in]   addr: Desired address to be erased
 * \retval      true: The target address was erased correctly
 * \retval      false: An error occurred
 */
static bool EventSST2xVFEraseSector(uint32_t addr)
{
    bool ret = true;

    if (SST2xVF_DRIVER.EraseSector(addr) != SST2xVF_RET_OK)
    {
        ret = false;
    }

    return ret;
}

/*!
 * \brief       Reads an amount of data from a desired address in the memory
 * \param[in]   addr: Position to be read from
 * \param[out]  data: Data read
 * \param[in]   size: Size of the data read
 * \retval      true: The data was read successfully
 * \retval      false: An error occurred
 */
static bool EventSST2xVFRead(uint32_t addr, uint8_t *data, uint32_t size)
{
    bool ret = true;

    if (SST2xVF_DRIVER.ReadData(addr, data, size) != SST2xVF_RET_OK)
    {
        ret = false;
    }

    return ret;
}

/*!
 * \brief       Writes an amount of data in a desired address in the memory
 * \param[in]   addr: Position to be written into
 * \param[out]  data: Data to be written
 * \param[in]   size: Size of the data to be written
 * \retval      true: The data was written successfully
 * \retval      false: An error occurred
 */
static bool EventSST2xVFWrite(uint32_t addr, uint8_t *data, uint32_t size)
{
    bool ret = true;

    if (SST2xVF_DRIVER.ProgramData(addr, data, size) != SST2xVF_RET_OK)
    {
        ret = false;
    }
    return ret;
}
