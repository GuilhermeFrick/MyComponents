/*!
 * \file       linked_list.h
 * \brief      Linked list component header file
 * \date       2020-09-04
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2020
 */
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_
#include <stdint.h>
#include <stdlib.h>
/** \addtogroup   LinkedList Linked List
 * @{
 */
/*!
 * \brief       Types of linked list return values
 */
typedef enum LinkedListRetDefinition
{
    LINKED_LIST_RET_OK = 0,           /*!<Returned OK*/
    LINKED_LIST_RET_NOT_FOUND,        /*!<The requested resource was not found*/
    LINKED_LIST_RET_ERR_INIT,         /*!<Initialization error*/
    LINKED_LIST_RET_ERR_MEM,          /*!<Insuficient memory error*/
    LINKED_LIST_RET_ERR_NOT_SUPPORTED /*!<Not supported error*/
} LinkedListRet_e;

/*!
 * \brief       List node definition
 */
typedef struct ListNode
{
    struct ListNode *next_node; /*!<Next list node*/
    void *           item;      /*!<Pointer to the item*/
} LinkedListNode_t;

/*!
 * \brief       List instance type
 */
typedef void *LinkedListInstance_t;

LinkedListRet_e LinkedListInit(LinkedListInstance_t *my_list, size_t item_size, size_t max_size);
LinkedListRet_e LinkedListDeInit(LinkedListInstance_t *my_list);
// LinkedListNode_t *LinkedListPush(LinkedListInstance_t my_list, void *new_item);
LinkedListNode_t *LinkedListApend(LinkedListInstance_t my_list, void *new_item);
// LinkedListNode_t *LinkedListInsert(LinkedListInstance_t my_list, void *new_item, LinkedListNode_t *des_node);
// LinkedListNode_t *LinkedListGetRef(LinkedListInstance_t my_list, void *item_val);
// LinkedListRet_e   LinkedListGetVal(LinkedListInstance_t my_list, void *new_item, LinkedListNode_t item_ptr);
LinkedListRet_e   LinkedListRemoveItem(LinkedListInstance_t my_list, void *item);
LinkedListNode_t *LinkedListGetNth(LinkedListInstance_t my_list, uint32_t n);
uint32_t          LinkedListGetSize(LinkedListInstance_t my_list);

#endif
/** @}*/ // End of LinkedList
