/*!
 * \file       EventManager.c
 * \brief
 * \date       2021-07-05
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 */

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "EventManager.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/** \weakgroup  EventManagerWeak Event Manager Weak
 *  \ingroup EventManager
 * @{
 */

#ifndef __weak
#define __weak __attribute__((weak)) /**<weak attribute definition*/
#endif

__weak EventReturn_e EventManagerCallback(EventManagerCallback_e notify);
__weak EventReturn_e EventManagerStoreCallback(uint32_t pointer, uint32_t counter);
__weak void *        EventManagerMalloc(size_t WantedSize);
__weak void          EventManagerFree(void *buffer);
/** @}*/ // End of EventManagerWeak

/** \addtogroup  EventManagerPrivate Event Manager Private
 *  \ingroup EventManager
 * @{
 */
QueueHandle_t EventQueue = NULL; /**<Queue for temporary storage of events*/

static int32_t       EventSend2Queue(uint8_t *event);
static EventReturn_e EventStore(uint8_t *event);
static EventReturn_e EventCheckErase(void);
static uint32_t      GetFlashPointer(uint32_t event_number);

static EventInfo_t            EventInfo;             /**<Event Information*/
static bool                   Initialized   = false; /**<Boolean indicating if the driver has initialized*/
const EventMemoryInterface_t *MEM_INTERFACE = NULL;  /*!<Memory access functions*/

static SemaphoreHandle_t EventManagerMutex      = NULL; /**<Mutex to handle global values*/
static uint32_t          MutexWaitTicks         = 0;    /**<The time in ticks to wait for the EventManager semaphore to become available*/
static uint32_t          AutoIncrementLogNumber = 0;    ///< Automatically incremented log number to be used by the EventManagerReadNext function

/** @}*/ // End of EventManagerPrivate

/*!
 * \brief       This function is used to initialize the driver
 * \param[in]   event_size: size of stored events, needs to be a multiple of 8
 * \return      Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerInitialize(const EventManagerConfig_t *const config, const EventMemoryInterface_t *const mem_interface)
{
    EventReturn_e ret         = EVENT_RET_OK;
    bool          mutex_taken = false;

    do
    {
        if (EventManagerMutex == NULL)
        {
            EventManagerMutex = xSemaphoreCreateMutex();
        }
        if (EventManagerMutex == NULL)
        {
            ret = EVENT_MUTEX_NULL_ERROR;
            break;
        }

        if ((config == NULL) || (mem_interface == NULL))
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }

        MutexWaitTicks = config->mutex_wait_tick;

        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }
        mutex_taken = true;
        if (EventQueue == NULL)
        {
            EventQueue = xQueueCreate(config->queue_size, config->event_size);
        }
        if (EventQueue == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        MEM_INTERFACE = mem_interface;

        if ((MEM_INTERFACE->ConfigInfoFunc == NULL) || (MEM_INTERFACE->EraseAllFunc == NULL) || (MEM_INTERFACE->EraseSectorFunc == NULL))
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if ((MEM_INTERFACE->ReadFunc == NULL) || (MEM_INTERFACE->WriteFunc == NULL))
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if (MEM_INTERFACE->InitFunc)
        {
            if (MEM_INTERFACE->InitFunc() == false)
            {
                ret = EVENT_RET_ERR_FLASH;
                break;
            }
        }
        EventInfo.EventSize    = config->event_size;
        EventInfo.Pointer      = config->pointer_init;
        EventInfo.Counter      = config->counter_init;
        EventInfo.FirstPointer = config->first_valid_addr;
        EventInfo.MaxPointer   = config->first_valid_addr + config->size_used - 1;

        if (MEM_INTERFACE->ConfigInfoFunc(&EventInfo) == false)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if ((EventInfo.Counter > EventInfo.MaxLogsNumber) || (EventInfo.Pointer > EventInfo.MaxPointer) || (EventInfo.Pointer % EventInfo.EventSize))
        {
            EventInfo.Counter = 0;
            EventInfo.Pointer = EventInfo.FirstPointer;
        }
        if (EventInfo.Pointer < EventInfo.FirstPointer)
        {
            EventInfo.Pointer = EventInfo.FirstPointer;
        }

        if (EventInfo.Counter >= (EventInfo.MaxLogsNumber - EventInfo.LogsPerSector))
        {
            EventInfo.Counter = (EventInfo.MaxLogsNumber - EventInfo.LogsPerSector) +
                                (((EventInfo.Pointer - EventInfo.FirstPointer) % EventInfo.SectorSize) / EventInfo.EventSize);
        }
        else
        {
            EventInfo.Counter = ((EventInfo.Pointer - EventInfo.FirstPointer) / EventInfo.EventSize);
        }

        if ((EventInfo.SectorSize % EventInfo.EventSize) || (EventInfo.EventSize > EventInfo.SectorSize))
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        Initialized = true;
    } while (0);

    if (mutex_taken)
    {
        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
        }
    }

    return ret;
}
/*!
 * \brief   This function is used to uninitialize the driver
 * \return  Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerUninitialize(void)
{
    EventReturn_e ret = EVENT_RET_OK;

    if (EventQueue != NULL)
    {
        vQueueDelete(EventQueue);
        EventQueue = NULL;
    }
    if (EventManagerMutex != NULL)
    {
        vSemaphoreDelete(EventManagerMutex);
        EventManagerMutex = NULL;
    }
    memset(&EventInfo, 0, sizeof(EventInfo_t));
    MEM_INTERFACE = NULL;

    Initialized = false;

    return ret;
}
/*!
 * \brief       Function that records events stored in the queue
 * \param[in]   max_events: maximum number of events that can be processed
 * \return      Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerRun(uint32_t max_events)
{
    EventReturn_e ret   = EVENT_RET_OK;
    uint8_t *     event = NULL;

    do
    {
        bool mutex_taken = false;
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if (max_events == 0)
        {
            break;
        }

        event = EventManagerMalloc(EventInfo.EventSize);
        if (event == NULL)
        {
            ret = EVENT_MALLOC_ERROR;
            break;
        }

        while (xQueuePeek(EventQueue, event, 10) == pdTRUE)
        {
            if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
            {
                ret = EVENT_MUTEX_TAKE_ERROR;
                break;
            }

            mutex_taken = true;

            ret = EventStore(event);
            if (ret != EVENT_RET_OK)
            {
                break;
            }
            xQueueReceive(EventQueue, event, 10);

            mutex_taken = false;
            if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
            {
                ret = EVENT_MUTEX_GIVE_ERROR;
                break;
            }
            EventManagerCallback(EVENT_STORED);
            if (--max_events == 0)
            {
                break;
            }
        }

        if (mutex_taken == true)
        {
            if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
            {
                ret = EVENT_MUTEX_GIVE_ERROR;
                break;   
            }
        }

    } while (0);

    EventManagerFree(event);

    return ret;
}
/*!
 * \brief   This function is used to erase all stored events
 * \return  Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerClear(void)
{
    EventReturn_e ret         = EVENT_RET_OK;
    bool          mutex_taken = false;

    do
    {
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }

        mutex_taken = true;

        if (MEM_INTERFACE->EraseAllFunc() == false)
        {
            ret = EVENT_RET_ERR_FLASH;
            break;
        }

        EventInfo.Pointer = EventInfo.FirstPointer;
        EventInfo.Counter = 0;

        if (EventManagerStoreCallback(EventInfo.Pointer, EventInfo.Counter) != EVENT_RET_OK)
        {
            ret = EVENT_RET_ERR_MEM;
        }
    } while (0);

    if (mutex_taken)
    {
        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
        }
    }
    return ret;
}
/*!
 * \brief       This function is used to read an event
 * \param[in]   log_number: number of the event to be read. The most recent is the '0' index
 * \param[out]  event: pointer to the variable where the read event will be stored
 * \param[in]   event_size: size of the event variable, to prevent memory invasion
 * \return      Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerRead(uint32_t log_number, uint8_t *event, uint32_t event_size)
{
    EventReturn_e ret         = EVENT_RET_OK;
    bool          mutex_taken = false;

    do
    {
        uint32_t flash_pointer;

        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if (event_size < EventInfo.EventSize)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }
        mutex_taken = true;

        if (log_number >= EventInfo.Counter)
        {
            ret = EVENT_NOT_EXIST;
            break;
        }

        flash_pointer = GetFlashPointer(log_number);

        if (flash_pointer > EventInfo.MaxPointer)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }

        EventManagerCallback(READ_EVENT);

        if (MEM_INTERFACE->ReadFunc(flash_pointer, event, EventInfo.EventSize) == false)
        {
            ret = EVENT_RET_ERR_FLASH;
            break;
        }

    } while (0);

    if (mutex_taken)
    {
        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
        }
    }

    return ret;
}

/**
 * @brief       Automatically reads the next event
 * @param[in]   event_size: size of the event variable, to prevent memory invasion
 * @param[out]  event: pointer to the variable where the read event will be stored
 * @return      Result of the operation \ref EventReturn_e
 */
EventReturn_e EventManagerReadNext(uint32_t event_size, uint8_t *event)
{
    EventReturn_e ret         = EVENT_RET_OK;
    bool          mutex_taken = false;

    do
    {
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        ret = EventManagerRead(AutoIncrementLogNumber, event, event_size);
        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }
        mutex_taken = true;
        if (++AutoIncrementLogNumber >= EventInfo.Counter)
        {
            AutoIncrementLogNumber = 0;
        }

    } while (0);

    if (mutex_taken)
    {
        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
        }
    }
    return ret;
}
/**
 * @brief       Resets the @ref AutoIncrementLogNumber variable
 * @return      Result of the operation @ref EventReturn_e
 */
EventReturn_e EventManagerResetAutoCount(void)
{
    EventReturn_e ret         = EVENT_RET_OK;
    bool          mutex_taken = false;

    do
    {
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }
        mutex_taken            = true;
        AutoIncrementLogNumber = 0;

    } while (0);

    if (mutex_taken)
    {
        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
        }
    }
    return ret;
}
/**
 * @brief
 * @return      Result of the operation @ref uint32_t
 */
uint32_t EventManagerGetAutoCount(void)
{
    return AutoIncrementLogNumber;
}
/*!
 * \brief       Schedules the event to be saved later
 * \param[in]   event: pointer to event to be saved
 * \param[in]   event_size: size of event buffer
 * \return      Result of the operation
 * \retval      EVENT_RET_OK: function executed successfully, event created
 * \retval      EVENT_RET_ERROR: error occurred
 * \retval      EVENT_INVALID_PARAM: invalid event_size
 */
EventReturn_e EventManagerWriteBack(uint8_t *event, uint32_t event_size)
{
    EventReturn_e ret = EVENT_RET_OK;

    do
    {
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if (event_size != EventInfo.EventSize)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if (event == NULL)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if (EventSend2Queue(event) != 0)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
    } while (0);

    return ret;
}
/*!
 * \brief       Saves the event and only returns when confirmed
 * \param[in]   event: pointer to event to be saved
 * \param[in]   event_size: size of event buffer
 * \return      Result of the operation
 * \retval      EVENT_RET_OK: function executed successfully, event created
 * \retval      EVENT_RET_ERROR: error occurred
 * \retval      EVENT_INVALID_PARAM: invalid event_size
 */
EventReturn_e EventManagerWriteThrough(uint8_t *event, uint32_t event_size)
{
    EventReturn_e ret = EVENT_RET_OK;

    do
    {
        if (Initialized != true)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if (event_size != EventInfo.EventSize)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if (event == NULL)
        {
            ret = EVENT_INVALID_PARAM;
            break;
        }
        if ((xSemaphoreTake(EventManagerMutex, MutexWaitTicks) != pdTRUE))
        {
            ret = EVENT_MUTEX_TAKE_ERROR;
            break;
        }

        ret = EventStore(event);

        if (xSemaphoreGive(EventManagerMutex) != pdTRUE)
        {
            ret = EVENT_MUTEX_GIVE_ERROR;
            break;
        }
    } while (0);

    return ret;
}
/**
 * \brief       Returns information about events
 * \return      Pointer to EventInfo_t structure with event information
 */
EventInfo_t *EventManagerGetInfo(void)
{
    return &EventInfo;
}

/*!
 * \brief   Notify Event Task
 * \param   notify: \ref EventManagerCallback_e type containing notification
 * \return  Result of operation
 * \return  Result of the operation \ref EventReturn_e
 */
__weak EventReturn_e EventManagerCallback(EventManagerCallback_e notify)
{
    return EVENT_CALLBACK_ERROR;
}
/*!
 * \brief      Function callback to store pointer and counter on non-volatile memory
 * \param[in]  pointer: pointer value
 * \param[in]  counter: counter value
 * \return     Result of the operation \ref EventReturn_e
 */
__weak EventReturn_e EventManagerStoreCallback(uint32_t pointer, uint32_t counter)
{
    return EVENT_RET_ERR_STORE;
}
/*!
 *  \brief      Allocates a block of size bytes of memory
 *  \param[in]  WantedSize: Size of the memory block, in bytes
 *  \return     On success, a pointer to the memory block allocated by the function. \n
 *              If the function failed to allocate the requested block of memory, a null pointer is returned.
 */
__weak void *EventManagerMalloc(size_t WantedSize)
{
    return malloc(WantedSize);
}
/*!
 *  \brief      Free memory allocated on the buffer pointer
 *  \param[in]  buffer: pointer to a memory block previously allocated
 */
__weak void EventManagerFree(void *buffer)
{
    free(buffer);
}
/*!
 * \brief       Saves an event (puts on the event queue)
 * \param[in]   event: pointer to event to be saved
 * \return      Result of operation
 * \retval      0: successfully saved the event on the queue
 * \retval      -1: error saving the event on the queue
 */
static int32_t EventSend2Queue(uint8_t *event)
{
    int32_t ret = 0;

    if (EventQueue == NULL)
    {
        ret = -1;
    }
    else if (xQueueSend(EventQueue, event, (TickType_t)100) != pdTRUE)
    {
        ret = -1;
    }
    else if (EventManagerCallback(NEW_EVENT) != 0)
    {
        ret = -1;
    }

    return ret;
}
/*!
 * \brief       Store an event on flash memory
 * \param[in]   event: pointer to event to be saved
 * \return      Result of operation \ref EventReturn_e
 * \retval      EVENT_RET_OK: successfully stored the event on flash memory
 * \retval      EVENT_RET_ERROR: error storing the event on flash memory
 */
static EventReturn_e EventStore(uint8_t *event)
{
    EventReturn_e ret = EVENT_RET_OK;

    do
    {
        if (EventCheckErase() != EVENT_RET_OK)
        {
            ret = EVENT_RET_ERR_FLASH;
            break;
        }

        if (MEM_INTERFACE->WriteFunc(EventInfo.Pointer, event, EventInfo.EventSize) == false)
        {
            ret = EVENT_RET_ERR_FLASH;
            break;
        }

        EventInfo.Counter++;
        EventInfo.Pointer += EventInfo.EventSize;

        if (EventInfo.Pointer > EventInfo.MaxPointer)
            EventInfo.Pointer = EventInfo.FirstPointer;

        if (EventManagerStoreCallback(EventInfo.Pointer, EventInfo.Counter) != EVENT_RET_OK)
        {
            ret = EVENT_RET_ERR_MEM;
        }

    } while (0);

    return ret;
}
/*!
 * \brief       Checks if the current sector of flash memory needs to be erased
 * \return      Result of the operation \ref EventReturn_e
 * \retval      EVENT_RET_OK: Function executed successfully
 * \retval      EVENT_RET_ERROR: Error erasing flash
 */
static EventReturn_e EventCheckErase(void)
{
    EventReturn_e ret = EVENT_RET_OK;

    if (EventInfo.Pointer % EventInfo.SectorSize == 0)
    {
        if (MEM_INTERFACE->EraseSectorFunc(EventInfo.Pointer) == false)
        {
            ret = EVENT_RET_ERR_FLASH;
        }
        else if (EventInfo.Counter >= EventInfo.MaxLogsNumber)
        {
            EventInfo.Counter -= EventInfo.LogsPerSector;
        }
    }

    return ret;
}
/*!
 * \brief       Function to get the position that the event is recorded in flash memory.
 * \param[in]   event_number: Event number to get position
 * \return      Event position
 */
static uint32_t GetFlashPointer(uint32_t event_number)
{
    uint32_t first_log_pointer;
    uint32_t flash_pointer;
    uint32_t memory_size   = EventInfo.MaxPointer + 1;
    uint32_t sector_offset = EventInfo.Pointer % EventInfo.SectorSize;

    if (EventInfo.Counter > (EventInfo.MaxLogsNumber - EventInfo.LogsPerSector))
    {
        if (sector_offset == 0)
        {
            first_log_pointer = EventInfo.Pointer;
        }
        else
        {
            first_log_pointer = EventInfo.Pointer - sector_offset + EventInfo.SectorSize;
        }

        if (first_log_pointer == memory_size)
            first_log_pointer = EventInfo.FirstPointer;
    }
    else
    {
        first_log_pointer = EventInfo.FirstPointer;
    }

    flash_pointer = ((EventInfo.Counter - event_number - 1) * EventInfo.EventSize) + first_log_pointer;
    if (flash_pointer >= memory_size)
    {
        flash_pointer -= memory_size;
    }
    return flash_pointer;
}
