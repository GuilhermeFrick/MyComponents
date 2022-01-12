/*!
 * \file       EventManager.h
 * \brief
 * \date       2021-07-05
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 * \verbatim
 *   ** Component Dependencies **
 *   =======================================
 *   1- SST2xVF memory driver, available in http://svn-gd/svn/components/embedded-library/Trunk/SST2xVF \n
 * \n
 *   ** Making this component functional **
 *   =======================================
 *  1- Override EventManagerStoreCallback to store pointer and counter on non-volatile memory \n
 *  2- Add EventManager.c in your project
 *  3- Call EventManagerInitialize with event size, initial pointer and counter \n
 * \n
 *   ** If using FreeRTOS **
 *   ====================================================
 *  4- Use FreeRTOS component, available in http://svn-gd/svn/components/FreeRTOS/Trunk \n
 *  5- Add EventTask.c in your project to override weak functions
 *  6- Call EventTaskCreate to create a Task to manage events \n
 * \n
 *  ** If not using FreeRTOS - not tested**
 *   ====================================================
 *  4- Use queue of FakeFreeRTOS component, available in http://svn-gd/svn/components/embedded-library/Trunk/FakeFreeRTOS
 *  5- Call EventManagerRun in your main loop
 * \endverbatim
 */

/** \addtogroup  EventManager Event Manager
 * @{
 */
#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define EventManagerSave(...) EventManagerWriteBack(__VA_ARGS__)
/*!
 *  \brief Return codes of the functions
 */
typedef enum
{
    EVENT_RET_OK           = 0,   /**<The function executed successfully*/
    EVENT_RET_ERR_MEM      = -1,  /**<Insufficient memory occurred*/
    EVENT_RET_ERR_FLASH    = -2,  /**<Flash memory error occurred*/
    EVENT_RET_ERR_STORE    = -3,  /**<Error on \ref EventManagerStoreCallback*/
    EVENT_INVALID_PARAM    = -4,  /**<Invalid function parameter*/
    EVENT_NOT_INIT         = -5,  /**<Module not initialized*/
    EVENT_NOT_EXIST        = -6,  /**<Selected event does not exist*/
    EVENT_MUTEX_NULL_ERROR = -7,  /**<Mutex not created*/
    EVENT_MUTEX_TAKE_ERROR = -8,  /**<Error getting the mutex */
    EVENT_MUTEX_GIVE_ERROR = -9,  /**<Error giving the mutex*/
    EVENT_MALLOC_ERROR     = -10, /**<Insufficient memory for malloc*/
    EVENT_CALLBACK_ERROR   = -11, /**<Error on \ref EventManagerCallback*/
} EventReturn_e;
/*!
 *  \brief Event information structure
 */
typedef struct
{
    uint8_t  ManID;         /**<Manufacturer ID*/
    uint8_t  DevID;         /**<Device ID*/
    uint8_t  EventSize;     /**<Size of each event in bytes*/
    uint32_t LogsPerSector; /**<Number of events stored by sector*/
    uint32_t MaxLogsNumber; /**<Maximum number of stored events*/
    uint32_t FirstPointer;  /**<First address value of the event memory*/
    uint32_t MaxPointer;    /**<Last address value of the event memory*/
    uint32_t SectorSize;    /**<Sector size of event memory*/
    uint32_t Pointer;       /**<Current value of the event memory address*/
    uint32_t Counter;       /**<Current amount of events stored in event memory*/
} EventInfo_t;
/*!
 * \brief      Structure with configuration to initialize EventManager
 */
typedef struct
{
    uint32_t event_size;       /**<Size of event*/
    uint32_t pointer_init;     /**<Initial memory pointer*/
    uint32_t counter_init;     /**<Initial memory counter*/
    uint32_t queue_size;       /**<Size of queue to temporary store events*/
    uint32_t mutex_wait_tick;  /**<The time in ticks to wait for the EventManager semaphore to become available*/
    uint32_t first_valid_addr; /**<First valid address to be used in the memory*/
    size_t   size_used;        /**<Size to be used for event storage*/
} EventManagerConfig_t;
/*!
 *  \brief List of notifications to EventTask
 */
typedef enum
{
    NEW_EVENT    = 1UL << 0, /**<New event to store queued*/
    READ_EVENT   = 1UL << 1, /**<Reading of events in progress*/
    EVENT_STORED = 1UL << 2, /**<Event stored on \ref EventManagerRun*/
} EventManagerCallback_e;

/*!
 * \brief       Memory Interface access structure
 */
typedef struct EventMemoryInterfaceDef
{
    bool (*InitFunc)(void);                                         /*!<Pointer to an optional initialization function*/
    bool (*ConfigInfoFunc)(EventInfo_t *info);                      /*!<Pointer to a configuration function*/
    bool (*EraseAllFunc)(void);                                     /*!<Pointer to an erase all function*/
    bool (*EraseSectorFunc)(uint32_t addr);                         /*!<Pointer to an erase sector function*/
    bool (*ReadFunc)(uint32_t addr, uint8_t *data, uint32_t size);  /*!<Pointer to a read data function*/
    bool (*WriteFunc)(uint32_t addr, uint8_t *data, uint32_t size); /*!<Pointer to a write data function*/
} EventMemoryInterface_t;

EventReturn_e EventManagerInitialize(const EventManagerConfig_t *const config, const EventMemoryInterface_t *const mem_interface);
EventReturn_e EventManagerUninitialize(void);
EventReturn_e EventManagerRun(uint32_t max_events);
EventReturn_e EventManagerClear(void);
EventReturn_e EventManagerRead(uint32_t log_number, uint8_t *event, uint32_t event_size);
EventReturn_e EventManagerReadNext(uint32_t event_size, uint8_t *event);
EventReturn_e EventManagerResetAutoCount(void);
uint32_t      EventManagerGetAutoCount(void);
EventReturn_e EventManagerWriteBack(uint8_t *event, uint32_t event_size);
EventReturn_e EventManagerWriteThrough(uint8_t *event, uint32_t event_size);
EventInfo_t * EventManagerGetInfo(void);

#endif   /*EVENT_MANAGER_H*/
/** @}*/ // End of EventManager
