/******************************************************************************
 * $Id$     logger_manager.c            2020-10-15
 *//**
 * \file    logger_manager.c 
 * \brief   Source file of the logger_manager.c
 * \version 1.0
 * \date    15. Oct. 2020
 * \author  Guilherme Frick de Oliveira
 ******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "logger_manager.h"
#include "linked_list.h"

#ifndef __weak
#define __weak __attribute__((weak))
#endif

const uint8_t MAX_NUMBER_LOGGERS  = 10; /*!<Maximum number of loggers*/
const size_t  MAX_PRINT_ITEM_SIZE = 32; /*!<Maximum size of a printable item in chars*/

/** \weakgroup   LoggerManagerWeek  LoggerManager Week
 *  \ingroup LoggerManager
 * @{
 */
__weak void   *LoggerManagerMalloc(uint32_t size);
__weak void    LoggerManagerFree(void *buffer);
__weak int32_t LoggerManagerSendBuffer(uint8_t *buffer, uint32_t size);
__weak int32_t LoggerManagerPrint(const char *format, void *val, size_t val_size, size_t max_size);

/** \addtogroup  LoggerManagerPrivate Logger Manager Private
 *  \ingroup LoggerManager
 * @{
 */
static LoggerManagerRet_e LoggerManagerPrintLabels(LoggerHandle_t inst);
/*!
 * \brief       Logger linked list to hold a record of any logger instance created
 */
static LinkedListInstance_t LoggerList;

/** @}*/ // End of LoggerManagerPrivate
/*!
 *  \brief Control structure to handle a given buffer.
 */
typedef struct BufferHandleDefiniton
{
    void         *buffer;          /*!<Pointer to an external buffer*/
    size_t        buffer_size;     /*!<Size in bytes of the buffer*/
    void         *packet_index;    /*!<Current position within the buffer*/
    ItemInfo_t   *item_info;       /*!<Item information to tell how the buffer will be printed*/
    size_t        item_qty;        /*!<Number of items in item_info*/
    size_t        max_malloc_size; /*!<Maximum size of the buffer to be allocated. Calculated dinamically*/
    LoggerState_e state;           /*!<State of the logger instance*/
} BufferHandle_t;

/*!
 * \brief       Initialize a logger instance with its item information
 * \param[out]  inst: Instance to be created
 * \param[in]   info: Information set
 * \param[in]   qty: number of items in the info
 * \return      Result of the operation \ref LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerInit(LoggerHandle_t *inst, ItemInfo_t *info, size_t qty)
{
    LoggerManagerRet_e ret = LOGGER_RET_INVALID;
    do
    {
        if ((*inst != NULL) || (qty == 0) || (info == NULL))
        {
            break;
        }
        if (LoggerList == NULL)
        {
            if (LinkedListInit(&LoggerList, sizeof(BufferHandle_t), MAX_NUMBER_LOGGERS * sizeof(BufferHandle_t)) != LINKED_LIST_RET_OK)
            {
                ret = LOGGER_FAULT_ERR_MEM;
                break;
            }
        }

        LinkedListNode_t *new_node = LinkedListApend(LoggerList, NULL);

        if (new_node == NULL)
        {
            break;
        }

        BufferHandle_t *new_handle = (BufferHandle_t *)new_node->item;

        new_handle->item_info       = info;
        new_handle->item_qty        = qty;
        new_handle->max_malloc_size = 0;
        for (uint32_t i = 0; i < qty; i++)
        {
            new_handle->max_malloc_size += strlen(new_handle->item_info[i].format_str) + MAX_PRINT_ITEM_SIZE;
        }
        new_handle->state = LOGGER_IDLE;
        *inst             = new_handle;

        ret = LOGGER_RET_OK;
    } while (0);

    return ret;
}

/*!
 * \brief       Deinitialize a logger instance
 * \param       inst: Instance to be deleted. If NULL, deletes everything
 * \return      Result of the operation \ref LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerDeInit(LoggerHandle_t *inst)
{
    LoggerManagerRet_e ret = LOGGER_RET_INVALID;

    if (inst == NULL)
    {
        // TODO remove everything
    }
    else
    {
        BufferHandle_t *buff = (BufferHandle_t *)(*inst);

        if (LinkedListRemoveItem(LoggerList, buff) == LINKED_LIST_RET_OK)
        {
            *inst = NULL;
            ret   = LOGGER_RET_OK;
        }
    }

    return ret;
}

/*!
 * \brief       Define a buffer to be bound to a logger handle
 * \param[in]   inst: Logger instance
 * \param[in]   buff: Buffer to be logged
 * \param[in]   buffer_size: Size of the buffer in bytes
 * \return      Result of the operation \ref LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerDefineBuffer(LoggerHandle_t inst, void *buff, size_t buffer_size)
{
    LoggerManagerRet_e ret         = LOGGER_RET_INVALID;
    BufferHandle_t    *this_handle = (BufferHandle_t *)inst;

    do
    {
        if (this_handle == NULL)
        {
            break;
        }
        this_handle->buffer       = buff;
        this_handle->buffer_size  = buffer_size;
        this_handle->packet_index = this_handle->buffer;
        ret                       = LOGGER_RET_OK;
    } while (0);

    return ret;
}

/*!
 * \brief       Puts the logger in running state
 * \param[in]   inst: Logger instance
 * \return      Result of the operation \ref LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerStart(LoggerHandle_t inst)
{
    LoggerManagerRet_e ret  = LOGGER_RET_INVALID;
    BufferHandle_t    *buff = (BufferHandle_t *)inst;
    do
    {
        if (buff == NULL)
        {
            break;
        }

        if (buff->state != LOGGER_IDLE)
        {
            break;
        }
        ret = LoggerManagerPrintLabels(buff);
        if (ret != LOGGER_RET_OK)
        {
            break;
        }
        buff->state = LOGGER_RUNNING;
        ret         = LOGGER_RET_OK;

    } while (0);

    return ret;
}

/*!
 * \brief       Puts the logger in idle state
 * \param[in]   inst: Logger instance
 * \return      Result of the operation \ref LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerStop(LoggerHandle_t inst)
{
    LoggerManagerRet_e ret  = LOGGER_RET_INVALID;
    BufferHandle_t    *buff = (BufferHandle_t *)inst;
    do
    {
        if (buff == NULL)
        {
            break;
        }
        if (buff->state != LOGGER_RUNNING)
        {
            break;
        }
        buff->state = LOGGER_IDLE;
        ret         = LOGGER_RET_OK;

    } while (0);
    return ret;
}
/*!
 *  \brief      Iterates through the buffer or queue and sends it to the output interface.
 *  \param[in]  inst: Logger instance.
 *  \param[in]  max_iter: Maximum number of iterations in a single call
 *  \return     Result of operation \ref LoggerManagerRet_e
 *  \retval     LOGGER_RET_OK if the logger sent data but hasn't finished.
 *  \retval     LOGGER_RET_OUT_OF_DATA if the logger finished.
 *  \retval     LOGGER_RET_INVALID if the max_iter or the handle are null.
 *  \retval     LOGGER_RET_ERROR if data wasn't sent.
 */
LoggerManagerRet_e LoggerRun(LoggerHandle_t inst, uint32_t max_iter)
{
    LoggerManagerRet_e ret  = LOGGER_RET_INVALID;
    BufferHandle_t    *buff = (BufferHandle_t *)inst;

    do
    {
        if ((buff == NULL) || (max_iter == 0))
        {
            break;
        }
        if (buff->state != LOGGER_RUNNING)
        {
            break;
        }
        void *last_pos = (uint8_t *)buff->buffer + buff->buffer_size;
        ret            = LOGGER_RET_OK;

        while (max_iter--)
        {
            size_t num_items = buff->item_qty;
            for (uint32_t i = 0; i < num_items; i++)
            {
                int32_t ret_prt = 0;
                ret_prt = LoggerManagerPrint(buff->item_info[i].format_str, buff->packet_index, buff->item_info[i].item_size, buff->max_malloc_size);
                if (ret_prt < 0)
                {
                    ret = LOGGER_RET_ERROR;
                }

                buff->packet_index = (uint8_t *)buff->packet_index + buff->item_info[i].item_size;
                if (buff->packet_index > last_pos)
                {
                    ret = LOGGER_RET_OUT_OF_DATA;
                    break;
                }
            }
            if (ret != LOGGER_RET_OK)
            {
                buff->state        = LOGGER_IDLE;
                buff->packet_index = buff->buffer;
                break;
            }
        }
    } while (0);

    return ret;
}
/**
 * @brief Get the current state of the Logger
 *
 * @param inst : Logger instance.
 * @param logger_state : Current state.
 * @return LoggerManagerRet_e
 */
LoggerManagerRet_e LoggerGetState(LoggerHandle_t inst, LoggerState_e *logger_state)
{
    LoggerManagerRet_e ret  = LOGGER_RET_INVALID;
    BufferHandle_t    *buff = (BufferHandle_t *)inst;
    do
    {
        if (buff == NULL)
        {
            break;
        }

        *logger_state = buff->state;
        ret           = LOGGER_RET_OK;

    } while (0);

    return ret;
}
/**
 * @brief  Print labels for configured items
 *
 * @param inst Logger instance.
 * @return LoggerManagerRet_e
 */
static LoggerManagerRet_e LoggerManagerPrintLabels(LoggerHandle_t inst)
{
    LoggerManagerRet_e ret  = LOGGER_RET_OK;
    BufferHandle_t    *buff = (BufferHandle_t *)inst;
    do
    {
        if (buff == NULL)
        {
            ret = LOGGER_RET_INVALID;
            break;
        }
        size_t num_items = buff->item_qty;
        for (uint32_t i = 0; i < num_items; i++)
        {
            int32_t ret_prt = 0;
            char   *format_label;
            if (i == 0)
            {
                format_label = "\n%s,";
            }
            else if (i == (num_items - 1))
            {
                format_label = "%s,\n";
            }
            else
            {
                format_label = "%s,";
            }
            uint32_t label_size = strlen((char *)buff->item_info[i].label) + strlen(format_label);
            ret_prt             = LoggerManagerPrint(format_label, (char *)buff->item_info[i].label, label_size, buff->max_malloc_size);

            if (ret_prt < 0)
            {
                ret = LOGGER_RET_ERROR;
                break;
            }
        }

    } while (0);

    return ret;
}

/*!
 *  \brief      Sends an amount of bytes in an communication interface
 *  \param[in]  data: transmit buffer
 *  \param[in]  size: length of data
 *  \return 	The amount of data written or a negative value in case of error
 */
__weak int32_t LoggerManagerSendBuffer(uint8_t *buffer, uint32_t size)
{
    return -1;
}

/*!
 * \brief       Prints the data into a temporary buffer and sends it
 * \param[in]   format: Format string according to the item information
 * \param[in]   val: Value pointer
 * \param[in]   val_size: Value size in bytes
 * \param[in]   max_size: maximum size of the string
 * \return      The amount of data written or a negative value in case of error
 */
__weak int32_t LoggerManagerPrint(const char *format, void *val, size_t val_size, size_t max_size)
{
    int32_t  ret       = -1;
    uint8_t *send_buff = NULL;

    send_buff = LoggerManagerMalloc(max_size);
    do
    {
        if (send_buff == NULL)
        {
            break;
        }
        int32_t buff_size = 0;

        switch (GetFormatType(format))
        {
            case FORMAT_INT:
            {
                int32_t aux_data_i32 = 0;
                memcpy(&aux_data_i32, val, val_size);
                buff_size = (int32_t)snprintf((char *)send_buff, max_size, format, aux_data_i32);
            }
            break;
            case FORMAT_UINT:
            {
                uint32_t aux_data_u32 = 0;
                memcpy(&aux_data_u32, val, val_size);
                buff_size = (int32_t)snprintf((char *)send_buff, max_size, format, aux_data_u32);
            }
            break;
            case FORMAT_FLOAT:
            {
                float aux_data_float = 0;
                memcpy(&aux_data_float, val, val_size);
                buff_size = (int32_t)snprintf((char *)send_buff, max_size, format, aux_data_float);
            }
            break;
            case FORMAT_LONG_INT:
            {
                uint32_t aux_data_64 = 0;
                memcpy(&aux_data_64, val, val_size);
                buff_size = (int32_t)snprintf((char *)send_buff, max_size, format, aux_data_64);
            }
            break;
            case FORMAT_STRING:
            {
                buff_size = (int32_t)snprintf((char *)send_buff, val_size < max_size ? val_size : max_size, format, val);
            }
            break;
                /*for (uint32_t i = 0; i < val_size; i++)
                {
                    buff_size += (int32_t)snprintf((char *)&send_buff[buff_size], max_size - buff_size, "%02X", val_uint8_t[i]);
                }
                break;*/
        }
        if (buff_size > 0)
        {
            ret = LoggerManagerSendBuffer(send_buff, buff_size);
        }

    } while (0);
    LoggerManagerFree(send_buff);

    return ret;
}
/*!
 *  \brief      Allocate memory block with control
 *  \param[in]  size: size of the memory block, in bytes
 *  \return 	On success, a pointer to the memory block allocated by the function
 *              If the function failed to allocate the requested block of memory, a null pointer is returned
 *  \warning    Stronger function must be declared externally
 */
__weak void *LoggerManagerMalloc(uint32_t size)
{
    return malloc(size);
}
/*!
 *  \brief      Deallocate memory block
 *  \param[in]  buffer: pointer to a memory block previously allocated with malloc
 *  \warning    Stronger function must be declared externally
 */
__weak void LoggerManagerFree(void *buffer)
{
    free(buffer);
}

FormatType_et GetFormatType(const char *format_str)
{
    static const struct FormatMap_s
    {
        FormatType_et f;
        char          c[4];
    } FormatMap[] = {
        {FORMAT_INT, "d"},
        {FORMAT_INT, "i"},
        {FORMAT_UINT, "u"},
        {FORMAT_FLOAT, "f"},
        {FORMAT_FLOAT, "F"},
        {FORMAT_LONG_INT, "ld"},
        {FORMAT_LONG_INT, "li"},
        {FORMAT_LONG_UINT, "lu"},
        {FORMAT_LONG_LONG_INT, "lli"},
        {FORMAT_LONG_LONG_INT, "lld"},
        {FORMAT_LONG_LONG_UINT, "llu"},
        {FORMAT_STRING, "s"},
    };
    static const char SkipChar[] = {'%', '-', '+', ' ', '#', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '.'};
    FormatType_et     ret        = FORMAT_ERROR;
    char             *pChar;
    
    //TODO - Fazer pular %%
    pChar = strstr(format_str, "%");
    if (pChar != NULL)
    {
        for (; *pChar != '\0'; pChar++)
        {
            bool valid = true;
            for (uint8_t i = 0; i < sizeof(SkipChar) / sizeof(*SkipChar); i++)
            {
                if (*pChar == SkipChar[i])
                {
                    valid = false;
                    break;
                }
            }
            if (valid)
            {
                break;
            }
        }
    }
    if (pChar != NULL)
    {
        for (uint8_t i = 0; i < sizeof(FormatMap) / sizeof(*FormatMap); i++)
        {
            if (!memcmp(pChar, FormatMap[i].c, strlen(FormatMap[i].c)))
            {
                ret = FormatMap[i].f;
                break;
            }
        }
    }
    return ret;
}
