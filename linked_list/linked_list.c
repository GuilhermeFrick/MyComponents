/*!
 * \file       linked_list.c
 * \brief      Linked list component source file
 * \date       2020-09-04
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2020
 */
#include <string.h>
#include "linked_list.h"

/** \addtogroup  LinkedListWeak Linked List Weak
 *  \ingroup LinkedList
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak)) /**<Definition of weak attribute*/
#endif
__weak void *LinkedListMalloc(size_t WantedSize);
__weak void  LinkedListFree(void *buffer);
/** @}*/ // End of LinkedListWeak

/** \addtogroup  LinkedListPrivate Linked List Private
 *  \ingroup LinkedList
 * @{
 */
/*!
 * \brief       Linked list control structure
 */
typedef struct ListHead
{
    struct ListHead * self;         /*!<Pointer to self to identify if the instance was created*/
    LinkedListNode_t *next_node;    /*!<Pointer to the next node of the list (first on this case)*/
    size_t            item_size;    /*!<Size of the item to be stored*/
    size_t            used_size;    /*!<Size used so far*/
    size_t            maximum_size; /*!<Maximum size allowed by the user*/
    uint32_t          insert_count; /*!<Number of insert operations*/
    uint32_t          remove_count; /*!<Number of remove operations*/
    void *            mutex;        /*!<Mutex to be implemented later*/
} LinkedListHead_t;
/** @}*/ // End of LinkedListPrivate

/*!
 * \brief       Initialize a linked list instance
 * \param[out]  my_list: Pointer to the list to be created
 * \param[in]   item_size: size of each element of the list
 * \param[in]   max_size: maximum size of memory allowed to this list
 * \return      Result of the operation \ref LinkedListRet_e
 */
LinkedListRet_e LinkedListInit(LinkedListInstance_t *my_list, size_t item_size, size_t max_size)
{
    LinkedListRet_e ret = LINKED_LIST_RET_ERR_INIT;

    do
    {
        LinkedListHead_t *this_list_header = *my_list;

        if (this_list_header->self == this_list_header)
        {
            ret = LINKED_LIST_RET_OK;
            break;
        }

        this_list_header = LinkedListMalloc(sizeof(LinkedListHead_t));

        if (this_list_header == NULL)
        {
            ret = LINKED_LIST_RET_ERR_MEM;
            break;
        }
        this_list_header->next_node    = NULL;
        this_list_header->self         = this_list_header;
        this_list_header->item_size    = item_size;
        this_list_header->maximum_size = max_size;
        this_list_header->insert_count = 0;
        this_list_header->remove_count = 0;
        this_list_header->used_size    = 0;
        this_list_header->mutex        = NULL;
        // TODO mutex for both RTOS and not RTOS systems

        *my_list = this_list_header;
        ret      = LINKED_LIST_RET_OK;
    } while (0);

    return ret;
}

/*!
 * \brief       De-initialize a linked list instance
 * \param[out]  my_list: Pointer to the list to be created
 * \return      Result of the operation \ref LinkedListRet_e
 */
LinkedListRet_e LinkedListDeInit(LinkedListInstance_t *my_list)
{
    LinkedListRet_e   ret              = LINKED_LIST_RET_NOT_FOUND;
    LinkedListHead_t *this_list_header = (LinkedListHead_t *)*my_list;

    do
    {
        if (this_list_header != this_list_header->self)
        {
            break;
        }
    } while (0);
    return ret;
}

/*!
 * \brief       Insert an item from ISR. Not implemented yet due to lack of mutex
 * \param[in]   my_list: desired list
 * \param[in]   new_item: the new item to be inserted
 * \return      Result of the operation \ref LinkedListRet_e
 */
LinkedListRet_e LinkedListInsertFromISR(LinkedListInstance_t my_list, void *new_item)
{
    LinkedListRet_e ret = LINKED_LIST_RET_ERR_NOT_SUPPORTED;

    return ret;
}

/*!
 * \brief       Insert an item on the last position of the list
 * \param[in]   my_list: Desired list
 * \param[in]   new_item: Pointer to the new item to be inserted. If NULL just allocates space and zeroes it
 * \return      The pointer to the newly created node or NULL it not created
 */
LinkedListNode_t *LinkedListApend(LinkedListInstance_t my_list, void *new_item)
{
    LinkedListNode_t *this_node = NULL;

    do
    {
        LinkedListHead_t *this_list_header = (LinkedListHead_t *)my_list;
        if (this_list_header != this_list_header->self)
        {
            break;
        }
        this_node = LinkedListMalloc(this_list_header->item_size + sizeof(LinkedListNode_t));
        if (this_node == NULL)
        {
            break;
        }
        this_node->item = this_node + 1;

        if (new_item)
        {
            memcpy(this_node->item, new_item, this_list_header->item_size);
        }
        else
        {
            memset(this_node->item, 0, this_list_header->item_size);
        }

        this_node->next_node = NULL;

        if (this_list_header->next_node == NULL)
        {
            this_list_header->next_node = this_node;
        }
        else
        {
            LinkedListNode_t *last_node = this_list_header->next_node;

            while (last_node->next_node != NULL)
            {
                last_node = last_node->next_node;
            }
            last_node->next_node = this_node;
        }
    } while (0);

    return this_node;
}

/*!
 * \brief       Remove a node from the desired list
 * \param[in]   my_list: The desired list
 * \param[in]   node: Pointer to the node to be removed
 * \return      Result of the operation \ref LinkedListRet_e
 */
LinkedListRet_e LinkedListRemove(LinkedListInstance_t my_list, LinkedListNode_t *node)
{
    LinkedListRet_e   ret              = LINKED_LIST_RET_NOT_FOUND;
    LinkedListHead_t *this_list_header = my_list;
    LinkedListNode_t *previous_node    = NULL;
    LinkedListNode_t *current_node     = NULL;

    do
    {
        if (this_list_header->next_node == NULL)
        {
            break;
        }
        current_node  = this_list_header->next_node;
        previous_node = current_node;
        while (current_node != NULL)
        {
            if (node == current_node)
            {
                if (current_node == this_list_header->next_node)
                {
                    this_list_header->next_node = current_node->next_node;
                }
                else
                {
                    previous_node->next_node = current_node->next_node;
                }

                LinkedListFree(current_node);
                ret = LINKED_LIST_RET_OK;
                break;
            }
            previous_node = current_node;
            current_node  = current_node->next_node;
        }
    } while (0);

    return ret;
}

/*!
 * \brief       Remove the first occurance of the value of a desired item
 * \param[in]   my_list: Desired list
 * \param[in]   item: Pointer to the value to be removed
 * \return      Result of the operation \ref LinkedListRet_e
 */
LinkedListRet_e LinkedListRemoveItem(LinkedListInstance_t my_list, void *item)
{
    LinkedListRet_e   ret              = LINKED_LIST_RET_NOT_FOUND;
    LinkedListHead_t *this_list_header = my_list;
    LinkedListNode_t *previous_node    = NULL;
    LinkedListNode_t *current_node     = NULL;

    do
    {
        if (this_list_header->next_node == NULL)
        {
            break;
        }
        current_node  = this_list_header->next_node;
        previous_node = current_node;
        while (current_node != NULL)
        {
            if (memcmp(current_node->item, item, this_list_header->item_size) == 0)
            {
                if (current_node == this_list_header->next_node)
                {
                    this_list_header->next_node = current_node->next_node;
                }
                else
                {
                    previous_node->next_node = current_node->next_node;
                }

                LinkedListFree(current_node);
                ret = LINKED_LIST_RET_OK;
                break;
            }
            previous_node = current_node;
            current_node  = current_node->next_node;
        }
    } while (0);

    return ret;
}
/*!
 * \brief       Get nth node from list
 * \param[in]   my_list: linked list instance
 * \param[in]   n: desired node position
 * \return      Pointer to nth node
 */
LinkedListNode_t *LinkedListGetNth(LinkedListInstance_t my_list, uint32_t n)
{
    LinkedListNode_t *node = NULL;

    if (my_list != NULL)
    {
        node = ((LinkedListHead_t *)my_list)->next_node;

        while ((node != NULL) && (n--))
        {
            node = node->next_node;
        }
    }

    return node;
}
/*!
 * \brief       Function to obtain the number of items stored in the list
 * \param[in]   my_list: linked list instance
 * \return      Number of items in the list
 */
uint32_t LinkedListGetSize(LinkedListInstance_t my_list)
{
    LinkedListNode_t *node = ((LinkedListHead_t *)my_list)->next_node;
    uint32_t          size = 0;

    if (my_list != NULL)
    {
        node = ((LinkedListHead_t *)my_list)->next_node;

        while (node != NULL)
        {
            size++;
            node = node->next_node;
        }
    }

    return size;
}
/*!
 *  \brief      Allocates a block of size bytes of memory
 *  \param[in]  WantedSize: Size of the memory block, in bytes
 *  \return     On success, a pointer to the memory block allocated by the function. \n
 *              If the function failed to allocate the requested block of memory, a null pointer is returned.
 */
__weak void *LinkedListMalloc(size_t WantedSize)
{
    return malloc(WantedSize);
}
/*!
 *  \brief      Free memory allocated on the buffer pointer
 *  \param[in]  buffer: pointer to a memory block previously allocated
 */
__weak void LinkedListFree(void *buffer)
{
    if (buffer != NULL)
    {
        free(buffer);
    }
}
