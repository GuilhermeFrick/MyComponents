/*!
 * \file       EventTask.c
 * \brief      Sourece file of event manager task
 * \date       2021-09-15
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 */
#include <string.h>
#include <stdbool.h>
#include "EventManager.h"
#include "EventTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*! \addtogroup  EventTaskPivate Event Task Pivate
 * \ingroup EventTask
 * @{
 */
/*!
 *  \brief  List of task notifications
 */
typedef enum
{
    NOTIFY_NEW_EVENT  = 1UL << 0, /**<New event to store queued*/
    NOTIFY_READ_EVENT = 1UL << 1, /**<Reading of events in progress*/
    NOTIFY_TERMINATE  = 1UL << 2  /**<Save all stored events and delete the task*/
} EventTaskNotify_e;

#define EventTask_PRIO  (tskIDLE_PRIORITY + 1)     /**<EventTask priority*/
#define EventTask_STACK (configMINIMAL_STACK_SIZE) /**<EventTask stack size*/
TaskHandle_t EventTask_Handle = NULL;              /**<Handle to EventTask function*/

static void   EventTask(void *pvParameters);
EventReturn_e EventManagerCallback(EventManagerCallback_e notify);
void *        EventManagerMalloc(size_t WantedSize);
void          EventManagerFree(void *buffer);
/*! @}*/ // End of EventTaskPivate

/*!
 * \brief  Create the task responsible for storing events
 */
void EventTaskCreate(void)
{
    if (EventTask_Handle == NULL)
    {
        xTaskCreate(EventTask, (char const *)"Event", EventTask_STACK, NULL, EventTask_PRIO, &EventTask_Handle);
    }
    else
    {
        vTaskResume(EventTask_Handle);
    }
}
/*!
 * \brief  Suspend the Event Task
 */
void EventTaskSuspend(void)
{
    if (EventTask_Handle != NULL)
    {
        vTaskSuspend(EventTask_Handle);
    }
}
/*!
 * \brief       Terminate the Event task
 * \param[in]   blocking: boolean indicating if the function will wait until task terminate to return
 */
void EventTaskTerminate(bool blocking)
{
    if (EventTask_Handle != NULL)
    {
        xTaskNotify(EventTask_Handle, NOTIFY_TERMINATE, eSetBits);
        if (blocking)
        {
            while (EventTask_Handle != NULL)
            {
                vTaskDelay(1);
            }
        }
    }
}
/*!
 * \brief  Event Task
 * \param  pvParameters: not used
 */
static void EventTask(void *pvParameters)
{
    while (1)
    {
        static uint32_t         EventTaskNotifiedValue = 0;
        static const TickType_t READ_EVENT_TIMEOUT     = 1500;

        if (EventManagerRun(0xFFFFFFFFUL) == EVENT_RET_OK)
        {
            xTaskNotifyWait(0xFFFFFFFFUL, 0xFFFFFFFFUL, &EventTaskNotifiedValue, portMAX_DELAY);
        }
        else
        {
            xTaskNotifyWait(0xFFFFFFFFUL, 0xFFFFFFFFUL, &EventTaskNotifiedValue, 100);
        }

        if (EventTaskNotifiedValue & NOTIFY_READ_EVENT)
        {
            do
            {
                // TODO change the timeout to parameter
                xTaskNotifyWait(0xFFFFFFFFUL, 0xFFFFFFFFUL, &EventTaskNotifiedValue, READ_EVENT_TIMEOUT);
            } while (EventTaskNotifiedValue & NOTIFY_READ_EVENT);
            EventManagerResetAutoCount();
        }

        if (EventTaskNotifiedValue & NOTIFY_TERMINATE)
        {
            EventManagerRun(0xFFFFFFFFUL);
            EventTask_Handle = NULL;
            vTaskDelete(NULL);
            while (1)
            {
                vTaskDelay(1);
            }
        }
    }
}
/*!
 *  \implements EventManagerCallback
 */
EventReturn_e EventManagerCallback(EventManagerCallback_e notify)
{
    EventReturn_e ret = EVENT_RET_OK;

    switch (notify)
    {
        case NEW_EVENT:
            if (EventTask_Handle != NULL)
            {
                if (xTaskNotify(EventTask_Handle, NOTIFY_NEW_EVENT, eSetBits) != pdTRUE)
                {
                    ret = EVENT_CALLBACK_ERROR;
                }
            }
            break;
        case READ_EVENT:
            if (EventTask_Handle != NULL)
            {
                if (xTaskNotify(EventTask_Handle, NOTIFY_READ_EVENT, eSetBits) != pdTRUE)
                {
                    ret = EVENT_CALLBACK_ERROR;
                }
            }
            break;
        case EVENT_STORED:
            vTaskDelay(10);
            break;
        default:
            ret = EVENT_CALLBACK_ERROR;
            break;
    }

    return ret;
}
/*!
 *  \implements EventManagerMalloc
 */
void *EventManagerMalloc(size_t WantedSize)
{
    return pvPortMalloc(WantedSize);
}
/*!
 *  \implements EventManagerFree
 */
void EventManagerFree(void *buffer)
{
    vPortFree(buffer);
}
