/**********************************************************************************************************************************************
 * $Id$		queue.h				2020-12-16
 *//**
* \file		queue.h
* \version	1.0
* \date		16. Dec. 2020
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
* \brief	Component for managing a ring buffer based on the interfaces of FreeRTOS
* \verbatim
*   ** Using dynamic allocation**
*   ======================================= 
*   1- [optional] Override \ref QueueMalloc function to provide a custom malloc function
*   2- [optional] Override \ref QueueFree function to provide a custom free function
*   3- Create a local handle to queue \ref QueueHandle_t
*   4- Create a new queue using \ref xQueueCreate store the returned handler in the created handler of type \ref QueueHandle_t
*   5- Post items using \ref xQueueSend
*   6- Receive items using \ref xQueueReceive
*   \n
*   ** Using static allocation**
*   ======================================= 
*   1- Create a local \ref StaticQueue_t variable
*   2- Create a local buffer with size (in bytes) equal to queue desired (ItemSize * Length)
*   3- Create a local handle to queue \ref QueueHandle_t
*   4- Create a new queue using \ref xQueueCreateStatic store the returned handler in the created handler of type \ref QueueHandle_t
*   5- Post items using \ref xQueueSend
*   6- Receive items using \ref xQueueReceive
*   \endverbatim
***********************************************************************************************************************************************/
/** \addtogroup  Queue Queue
 * @{
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct QueueDefinition *QueueHandle_t; /**<Handle to Queue*/

/*!
 *  \brief Structure to static allocation
 */
typedef struct
{
    void *   pvDummy1[4]; /**<Hidden data*/
    uint32_t pvDummy2[3]; /**<Hidden data*/
} StaticQueue_t;

QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize);
QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t *pucQueueStorageBuffer, StaticQueue_t *pxQueueBuffer);
void          vQueueDelete(QueueHandle_t xQueue);
bool          xQueueSend(QueueHandle_t pxQueue, const void *pvItemToQueue, uint32_t xTicksToWait);
bool          xQueueReceive(QueueHandle_t pxQueue, void *pvBuffer, uint32_t xTicksToWait);
bool          xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait);
uint32_t      uxQueueMessagesWaiting(QueueHandle_t xQueue);
uint32_t      uxQueueSpacesAvailable(QueueHandle_t xQueue);
bool          xQueueReset(QueueHandle_t xQueue);

/*!
 *  \brief Macro to queue send from isr, not implemented yet
 */
#define xQueueSendFromISR(x, y, z) xQueueSend(x, y, 0)

#endif
/** @}*/ // End of Queue
