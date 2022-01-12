/**********************************************************************************************************************************************
 * $Id$		queue.c				2020-12-16
 *//**
* \file		queue.c
* \version	1.0
* \date		16. Dez. 2020
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
* \brief	Source of component for managing a ring buffer based on the interfaces of FreeRTOS
***********************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

/** \weakgroup   QueueWeak Queue Weak
 *  \ingroup Queue
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak)) /**<Ensures that weak attribute is defined*/
#endif
__weak void *QueueMalloc(size_t size);
__weak void  QueueFree(void *ptr);
/** @}*/ // End of QueueWeak

/** \weakgroup   QueuePrivate Queue Private
 *  \ingroup Queue
 * @{
 */
#ifndef configASSERT
#define configASSERT(x) /*<Define to previne errors*/
#endif

/*!
 *  \brief Queue control Structure
 */
typedef struct QueueDefinition
{
    uint8_t *pcHead;     /**<Points to the beginning of the queue storage area.*/
    uint8_t *pcWriteTo;  /**<Points to the free next place in the storage area.*/
    uint8_t *pcTail;     /**<Points to the byte at the end of the queue storage area.  Once more byte is allocated than necessary to store the queue
                            items, this is used as a marker.*/
    uint8_t *pcReadFrom; /**<Points to the last place that a queued item was read from when the structure is used as a queue.*/
    uint32_t uxLength;   /**<The length of the queue defined as the number of items it will hold, not the number of bytes.*/
    uint32_t uxItemSize; /**<The size of each items that the queue will hold. */
    uint32_t ucStaticallyAllocated; /**<Set to true if the memory used by the queue was statically allocated to ensure no attempt is made to free the
                                       memory. */
} Queue_t;

static bool xQueueIsQueueFull(QueueHandle_t xQueue);

/** @}*/ // End of QueuePrivate

/*!
 *  \brief      Creates a new queue instance, and returns a handle by which the new queue
 *              can be referenced.
 *  \param[in]  uxQueueLength: The maximum number of items that the queue can contain
 *  \param[in]  uxItemSize:  The number of bytes each item in the queue will require
 *  \return     If the queue is successfully create then a handle to the newly
 *              created queue is returned.  If the queue cannot be created then 0 is
 *              returned.
 */
QueueHandle_t xQueueCreate(uint32_t uxQueueLength, uint32_t uxItemSize)
{
    Queue_t *pxNewQueue;
    size_t   xQueueSizeInBytes;

    xQueueSizeInBytes = (size_t)(uxQueueLength * uxItemSize);
    pxNewQueue        = (Queue_t *)QueueMalloc(sizeof(Queue_t) + xQueueSizeInBytes);

    if (pxNewQueue != NULL)
    {
        uint8_t *pucQueueStorage;
        pucQueueStorage = (uint8_t *)pxNewQueue;
        pucQueueStorage += sizeof(Queue_t);
        pxNewQueue->ucStaticallyAllocated = 0;
        /* Set the head to the start of the queue storage area. */
        pxNewQueue->pcHead = (uint8_t *)pucQueueStorage;

        /* Initialise the queue members as described where the queue type is
        defined. */
        pxNewQueue->uxLength   = uxQueueLength;
        pxNewQueue->uxItemSize = uxItemSize;
        xQueueReset(pxNewQueue);
    }

    return pxNewQueue;
}
/*!
 *  \brief      Creates a new queue instance, and returns a handle by which the new queue
 *              can be referenced.
 *  \param[in]  uxQueueLength: The number of bytes each item in the queue will require.
 *  \param[in]  uxItemSize:  The number of bytes each item in the queue will require
 *  \param[in]  pucQueueStorageBuffer: If uxItemSize is not zero then
 *              pucQueueStorageBuffer must point to a uint8_t array that is at least large
 *              enough to hold the maximum number of items that can be in the queue at any
 *              one time - which is ( uxQueueLength * uxItemsSize ) bytes.
 *  \param[in]  pxQueueBuffer: Must point to a variable of type StaticQueue_t, which
 *              will be used to hold the queue's data structure.
 *  \return     If the queue is created then a handle to the created queue is
 *              returned.  If pxQueueBuffer is NULL then NULL is returned.
 */
QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t *pucQueueStorageBuffer, StaticQueue_t *pxQueueBuffer)
{
    Queue_t *pxNewQueue;
    configASSERT(uxQueueLength > 0);
    configASSERT(pxQueueBuffer);
    configASSERT(pucQueueStorageBuffer);
    configASSERT(uxItemSize);
    configASSERT(sizeof(StaticQueue_t) == sizeof(Queue_t));

    pxNewQueue                        = (Queue_t *)pxQueueBuffer;
    pxNewQueue->ucStaticallyAllocated = 1;
    pxNewQueue->pcHead                = pucQueueStorageBuffer;
    pxNewQueue->uxLength              = uxQueueLength;
    pxNewQueue->uxItemSize            = uxItemSize;
    xQueueReset(pxNewQueue);

    return pxNewQueue;
}
/*!
 *  \brief      Delete a queue - freeing all the memory allocated for storing of items placed on the queue.
 *  \param[in]  xQueue: A handle to the queue to be deleted.
 */
void vQueueDelete(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue = xQueue;

    if (pxQueue->ucStaticallyAllocated == 0)
    {
        QueueFree(pxQueue);
    }
}
/*!
 *  \brief      Post an item on a queue.  The item is queued by copy, not by reference.
 *  \param[in]  xQueue: The handle to the queue on which the item is to be posted.
 *  \param[in]  pvItemToQueue: A pointer to the item that is to be placed on the
 *              queue.  The size of the items the queue will hold was defined when the
 *              queue was created, so this many bytes will be copied from pvItemToQueue
 *              into the queue storage area.
 *  \param[in]  xTicksToWait: xTicksToWait The maximum amount of time the task should block
 *              waiting for space to become available on the queue, should it already
 *              be full.  The call will return immediately if this is set to 0 and the
 *              queue is full.
 *  \return     true if the item was successfully posted otherwise false
 */
bool xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, uint32_t xTicksToWait)
{
    Queue_t *const pxQueue = xQueue;

    configASSERT(xQueue);
    configASSERT(!((pvItemToQueue == NULL) && (pxQueue->uxItemSize != 0U)));

    if (!xQueueIsQueueFull(pxQueue))
    {
        (void)memcpy(
            (void *)pxQueue->pcWriteTo, pvItemToQueue,
            (size_t)pxQueue->uxItemSize); /*lint !e961 !e418 !e9087 MISRA exception as the casts are only redundant for some ports, plus previous
                                             logic ensures a null pointer can only be passed to memcpy() if the copy size is 0.  Cast to void required
                                             by function signature and safe as no alignment requirement and copy length specified in bytes. */
        pxQueue->pcWriteTo += pxQueue->uxItemSize;
        if (pxQueue->pcWriteTo >= pxQueue->pcTail)
        {
            pxQueue->pcWriteTo = pxQueue->pcHead;
        }
    }
    else
    {
        // TODO timeout here , use xTicksToWait
    }
    return true;
}
// xQueueSendFromISR()
// xQueueSendToBack()
// xQueueSendToBackFromISR()
// xQueueSendToFront()
// xQueueSendToFrontFromISR()
/*!
 *  \brief      Receive an item from a queue. The item is received by copy so a buffer of
 *              adequate size must be provided. The number of bytes copied into the buffer
 *              was defined when the queue was created.
 *  \param[in]  xQueue: The handle to the queue from which the item is to be received.
 *  \param[in]  pvBuffer: Pointer to the buffer into which the received item will be copied.
 *  \param[in]  xTicksToWait: The maximum amount of time to wait an item.
 *              xQueueReceive() will return immediately if xTicksToWait is zero and the queue is empty.
 *  \return     true if an item was successfully received from the queue otherwise false
 */
bool xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait)
{
    Queue_t *const pxQueue = xQueue;
    configASSERT(xQueue);
    configASSERT(pvBuffer);

    if (pxQueue->pcReadFrom != pxQueue->pcWriteTo) // not empty
    {
        (void)memcpy((void *)pvBuffer, (void *)pxQueue->pcReadFrom, (size_t)pxQueue->uxItemSize);
        pxQueue->pcReadFrom += pxQueue->uxItemSize;
        if (pxQueue->pcReadFrom >= pxQueue->pcTail)
        {
            pxQueue->pcReadFrom = pxQueue->pcHead;
        }
        return 1;
    }
    else
    {
        // TODO timeout here , use xTicksToWait
    }

    return 0;
}

/*!
 *  \brief      Receive an item from a queue without deleting it. The item is received by copy so a buffer of
 *              adequate size must be provided. The number of bytes copied into the buffer
 *              was defined when the queue was created.
 *  \param[in]  xQueue: The handle to the queue from which the item is to be received.
 *  \param[in]  pvBuffer: Pointer to the buffer into which the received item will be copied.
 *  \param[in]  xTicksToWait: The maximum amount of time to wait an item.
 *              xQueuePeek() will return immediately if xTicksToWait is zero and the queue is empty.
 *  \return     true if an item was successfully received from the queue otherwise false
 */
bool xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, uint32_t xTicksToWait)
{
    Queue_t *const pxQueue = xQueue;
    configASSERT(xQueue);
    configASSERT(pvBuffer);

    if (pxQueue->pcReadFrom != pxQueue->pcWriteTo) // not empty
    {
        (void)memcpy((void *)pvBuffer, (void *)pxQueue->pcReadFrom, (size_t)pxQueue->uxItemSize);
        return 1;
    }
    else
    {
        // TODO timeout here , use xTicksToWait
    }

    return 0;
}
//	xQueueReceiveFromISR()
/*!
 *  \brief      Return the number of messages stored in a queue
 *  \param[in]  xQueue: A handle to the queue being queried
 *  \return     The number of messages available in the queue
 */
uint32_t uxQueueMessagesWaiting(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue = xQueue;
    int32_t        ReadFrom;
    int32_t        WriteTo;
    int32_t        MessagesWaiting;

    ReadFrom        = (int32_t)pxQueue->pcReadFrom - (uint32_t)pxQueue->pcHead;
    WriteTo         = (int32_t)pxQueue->pcWriteTo - (uint32_t)pxQueue->pcHead;
    MessagesWaiting = WriteTo - ReadFrom;

    if (MessagesWaiting < 0)
        MessagesWaiting += (pxQueue->uxItemSize * pxQueue->uxLength);

    return MessagesWaiting;
}
// uxQueueMessagesWaitingFromISR
/*!
 *  \brief      Return the number of free spaces in a queue.
 *  \param[in]  xQueue: A handle to the queue being queried.
 *  \return     The number of free spaces available in the queue.
 */
uint32_t uxQueueSpacesAvailable(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue = xQueue;
    int32_t        ReadFrom;
    int32_t        WriteTo;
    int32_t        SpacesAvailable;

    ReadFrom        = (int32_t)pxQueue->pcReadFrom - (uint32_t)pxQueue->pcHead;
    WriteTo         = (int32_t)pxQueue->pcWriteTo - (uint32_t)pxQueue->pcHead;
    SpacesAvailable = ReadFrom - WriteTo;

    if (SpacesAvailable <= 0)
        SpacesAvailable += (pxQueue->uxItemSize * pxQueue->uxLength);

    return SpacesAvailable;
}
// vQueueDelete
/*!
 *  \brief      Resets a queue to its original empty state
 *  \param[in]  xQueue: The handle of the queue being reset
 *  \return     always returns true
 */
bool xQueueReset(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue = xQueue;
    configASSERT(xQueue);

    pxQueue->pcTail     = pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize);
    pxQueue->pcWriteTo  = pxQueue->pcHead;
    pxQueue->pcReadFrom = pxQueue->pcHead;

    return true;
}
// xQueueIsQueueEmptyFromISR
// xQueueIsQueueFullFromISR
/*!
 *  \brief      Queries a queue to determine if the queue is full
 *  \param[in]  xQueue: A handle to the queue being queried.
 *  \return     true if queue is full otherwise false
 */
static bool xQueueIsQueueFull(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue  = xQueue;
    uint8_t *      WriteTo  = pxQueue->pcWriteTo;
    uint8_t *      ReadFrom = pxQueue->pcReadFrom;

    if ((WriteTo == pxQueue->pcTail) && (ReadFrom == pxQueue->pcHead))
        return true;
    if ((ReadFrom - WriteTo) == 1)
        return true;

    return false;
}
/*!
 *  \brief      Allocates a block of size bytes of memory
 *  \param[in]  size: Size of the memory block, in bytes
 *  \return     On success, a pointer to the memory block allocated by the function. \n
 *              If the function failed to allocate the requested block of memory, a null pointer is returned.
 */
__weak void *QueueMalloc(size_t size)
{
    return malloc(size);
}
/*!
 *  \brief      Free memory allocated on the buffer pointer
 *  \param[in]  ptr: pointer to a memory block previously allocated
 */
__weak void QueueFree(void *ptr)
{
    free(ptr);
}
