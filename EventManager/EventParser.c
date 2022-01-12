/**
 * @file       EventParser.c
 * @brief      Source file of Event parsing and high level access
 * @date       2021-08-21
 * @version    1.0
 * @author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * @copyright  Copyright (c) 2021
 */
#include <string.h>
#include <stdio.h>
#include "EventParser.h"
#include "FreeRTOS.h"
#include "EventTask.h"
#include "linked_list.h"
/**
 * @addtogroup EventParserWeak Event Parser Weak
 * @ingroup EventParser
 * @{
 */
#ifndef __weak
#define __weak __attribute__((weak))
#endif

__weak void EventTaskCreate(void);

/** @} */ // End of EventParserWeak

/** @addtogroup EventParserPrivate Event Parser Private
 *  @ingroup EventParser
 * @{
 */

/**
 * @brief       Event parser control structure
 */
static struct EventCtrl_s
{
    size_t                   event_size;  ///< Size of the EventBase_st + common_data_size + spec_data_size to be recorded on the memory
    const EventParserCfg_st *cfg;         ///< Pointer to a configuration structure @ref EventParserCfg_st
    LinkedListInstance_t     spec_list;   ///< List of specific code structure pointers @ref EventSpecificCfg_st
    bool                     initialized; ///< Boolean flag indicating the module was initialized
} EventCtrl;

static LinkedListNode_t *EventSearchSpecList(uint8_t trigger);
static int32_t           EventBasePrint(EventBase_st *const base, uint32_t max_buff_size, uint8_t *buff);
static int32_t           EventGenericPrint(uint8_t *in, uint32_t in_size, uint32_t max_buff_size, uint8_t *buff);

/** @} */ // End of EventParserPrivate

/**
 * @brief       Initializes the Event Parser module
 * @param[in]   parser_cfg: Configuration of this module
 * @param[in]   manager_cfg: Configuration used by the EventManager
 * @return      Result of the operation @ref EventReturn_e
 */
EventReturn_e EventInit(const EventParserCfg_st *const parser_cfg, const EventManagerConfig_t *const manager_cfg)
{
    EventReturn_e ret = EVENT_INVALID_PARAM;
    do
    {
        if (EventCtrl.initialized == true)
        {
            ret = EVENT_RET_OK;
            break;
        }
        if (parser_cfg == NULL)
        {
            break;
        }
        if (parser_cfg->BaseFill == NULL)
        {
            break;
        }
        if ((parser_cfg->CommonFill == NULL) && (parser_cfg->common_data_size > 0))
        {
            break;
        }
        if ((parser_cfg->CommonFill != NULL) && (parser_cfg->common_data_size == 0))
        {
            break;
        }
        EventCtrl.event_size = sizeof(EventBase_st) + parser_cfg->common_data_size + parser_cfg->spec_data_size;
        if (manager_cfg->event_size != EventCtrl.event_size)
        {
            break;
        }

        EventCtrl.cfg = parser_cfg;
        EventTaskCreate();
        EventCtrl.initialized = true;
        ret                   = EVENT_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Creates a raw event with the trigger and code informed by the caller
 * @param[in]   trigger: Trigger of the event
 * @param[in]   code: Code of the trigger
 * @return      Result of the operation @ref EventReturn_e
 */
EventReturn_e EventCreateRaw(uint8_t trigger, uint16_t code)
{
    EventReturn_e ret   = EVENT_RET_OK;
    uint8_t      *event = NULL;

    do
    {
        bool success = false;
        if (EventCtrl.initialized == false)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        event = (uint8_t *)pvPortMalloc(EventCtrl.event_size);
        if (event == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        memset(event, EventCtrl.cfg->padding_byte, EventCtrl.event_size);
        success = EventCtrl.cfg->BaseFill(trigger, code, (EventBase_st *)event);
        if (success == false)
        {
            ret = EVENT_CALLBACK_ERROR;
            break;
        }
        uint8_t *next_data = event + sizeof(EventBase_st);

        if (EventCtrl.cfg->CommonFill != NULL)
        {
            success = EventCtrl.cfg->CommonFill(EventCtrl.cfg->common_data_size, next_data);
            if (success == false)
            {
                ret = EVENT_CALLBACK_ERROR;
                break;
            }
            next_data += EventCtrl.cfg->common_data_size;
        }

        LinkedListNode_t *this_cfg_node = EventSearchSpecList(trigger);

        if (this_cfg_node != NULL)
        {
            EventSpecificCfg_st *specs = this_cfg_node->item;
            if (specs->SpecFill != NULL)
            {
                success = specs->SpecFill(code, EventCtrl.cfg->spec_data_size, next_data);
                if (success == false)
                {
                    ret = EVENT_CALLBACK_ERROR;
                    break;
                }
            }
        }

        ret = EventManagerWriteBack((uint8_t *)event, EventCtrl.event_size);

    } while (0);

    if (event != NULL)
    {
        vPortFree(event);
    }

    return ret;
}
/**
 * @brief       Inserts the specific trigger code handling functions
 * @param[in]   spec_cfg: Specific configuration @ref EventSpecificCfg_st
 * @return      Result of the operation @ref EventReturn_e
 */
EventReturn_e EventInsertSpecsCfg(const EventSpecificCfg_st *const spec_cfg)
{
    EventReturn_e ret = EVENT_INVALID_PARAM;
    do
    {
        if (EventCtrl.initialized == false)
        {
            ret = EVENT_NOT_INIT;
            break;
        }
        if (EventCtrl.spec_list == NULL)
        {
            if (LinkedListInit(&EventCtrl.spec_list, sizeof(EventParserCfg_st *), 256 * sizeof(EventParserCfg_st *)) != LINKED_LIST_RET_OK)
            {
                ret = EVENT_RET_ERR_MEM;
                break;
            }
        }

        LinkedListNode_t *this_cfg_node = EventSearchSpecList(spec_cfg->trigger);

        if (this_cfg_node == NULL)
        {
            this_cfg_node = LinkedListApend(EventCtrl.spec_list, NULL);
        }
        if (this_cfg_node == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        this_cfg_node->item = (void *)spec_cfg;

        ret = EVENT_RET_OK;
    } while (0);

    return ret;
}

/**
 * @brief       Read the event in simple format
 * @param[in]   log_number: Number of the log to be read
 * @param[in]   max_buff_size: Maximum size of the output buffer in bytes
 * @param[out]  buff: output buffer
 * @return      Number of bytes written in buff
 */
int32_t EventReadFormat(uint32_t log_number, uint32_t max_buff_size, uint8_t *buff)
{
    int32_t  ret   = EVENT_NOT_INIT;
    uint8_t *event = NULL;

    do
    {
        if (EventCtrl.initialized == false)
        {
            break;
        }
        event = (uint8_t *)pvPortMalloc(EventCtrl.event_size);
        if (event == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        ret = EventManagerRead(log_number, (uint8_t *)event, EventCtrl.event_size);

        if (ret != EVENT_RET_OK)
        {
            break;
        }
        EventBase_st *base_event  = (EventBase_st *)event;
        uint8_t      *common_data = event + sizeof(EventBase_st);
        uint8_t      *spec_data   = common_data + EventCtrl.cfg->common_data_size;

        ret = EventBasePrint(base_event, max_buff_size - ret, buff);

        if (EventCtrl.cfg->CommonPrint != NULL)
        {
            ret += EventCtrl.cfg->CommonPrint(common_data, max_buff_size - ret, &buff[ret]);
        }
        else
        {
            ret += EventGenericPrint(common_data, EventCtrl.cfg->common_data_size, max_buff_size - ret, &buff[ret]);
        }

        ret += EventGenericPrint(spec_data, EventCtrl.cfg->spec_data_size, max_buff_size - ret, &buff[ret]);

    } while (0);

    if (event != NULL)
    {
        vPortFree(event);
    }
    return ret;
}
/**
 * @brief       Read the next event in simple format
 * @param[in]   max_buff_size: Maximum size of the output buffer in bytes
 * @param[out]  buff: output buffer
 * @return      Number of bytes written in buff
 */
int32_t EventReadFormatNext(uint32_t max_buff_size, uint8_t *buff)
{
    int32_t  ret   = EVENT_NOT_INIT;
    uint8_t *event = NULL;

    do
    {
        if (EventCtrl.initialized == false)
        {
            break;
        }
        event = (uint8_t *)pvPortMalloc(EventCtrl.event_size);
        if (event == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        ret = EventManagerReadNext(EventCtrl.event_size, (uint8_t *)event);

        if (ret != EVENT_RET_OK)
        {
            break;
        }
        EventBase_st *base_event  = (EventBase_st *)event;
        uint8_t      *common_data = event + sizeof(EventBase_st);
        uint8_t      *spec_data   = common_data + EventCtrl.cfg->common_data_size;

        ret = EventBasePrint(base_event, max_buff_size - ret, buff);

        if (EventCtrl.cfg->CommonPrint != NULL)
        {
            ret += EventCtrl.cfg->CommonPrint(common_data, max_buff_size - ret, &buff[ret]);
        }
        else
        {
            ret += EventGenericPrint(common_data, EventCtrl.cfg->common_data_size, max_buff_size - ret, &buff[ret]);
        }

        ret += EventGenericPrint(spec_data, EventCtrl.cfg->spec_data_size, max_buff_size - ret, &buff[ret]);

    } while (0);

    if (event != NULL)
    {
        vPortFree(event);
    }
    return ret;
}

/**
 * @brief       Read the event in verbose format
 * @param[in]   log_number: Number of the log to be read
 * @param[in]   max_buff_size: Maximum size of the output buffer in bytes
 * @param[out]  buff: output buffer
 * @return      Number of bytes written in buff
 */
int32_t EventReadVerbose(uint32_t log_number, uint32_t max_buff_size, uint8_t *buff)
{
    int32_t  ret   = EVENT_NOT_INIT;
    uint8_t *event = NULL;

    do
    {
        if (EventCtrl.initialized == false)
        {
            break;
        }
        event = (uint8_t *)pvPortMalloc(EventCtrl.event_size);
        if (event == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        ret = EventManagerRead(log_number, (uint8_t *)event, EventCtrl.event_size);

        if (ret != EVENT_RET_OK)
        {
            break;
        }
        EventBase_st *base_event  = (EventBase_st *)event;
        uint8_t      *common_data = event + sizeof(EventBase_st);
        uint8_t      *spec_data   = common_data + EventCtrl.cfg->common_data_size;

        ret = snprintf((char *)buff, max_buff_size, "\r\n*********** %05u ***********\r\n", log_number);
        if (EventCtrl.cfg->BasePrintVerbose != NULL)
        {
            ret += EventCtrl.cfg->BasePrintVerbose(base_event, max_buff_size - ret, &buff[ret]);
        }

        LinkedListNode_t *this_cfg_node = EventSearchSpecList(base_event->trigger);

        if (this_cfg_node != NULL)
        {
            EventSpecificCfg_st *specs = this_cfg_node->item;

            if (specs->SpecPrintVerbose != NULL)
            {
                ret += specs->SpecPrintVerbose(base_event->code, spec_data, max_buff_size - ret, &buff[ret]);
            }
        }
        if (EventCtrl.cfg->CommonPrintVerbose != NULL)
        {
            ret += EventCtrl.cfg->CommonPrintVerbose(common_data, max_buff_size - ret, &buff[ret]);
        }

    } while (0);

    if (event != NULL)
    {
        vPortFree(event);
    }

    return ret;
}
/**
 * @brief       Read the event in verbose format
 * @param[in]   max_buff_size: Maximum size of the output buffer in bytes
 * @param[out]  buff: output buffer
 * @return      Number of bytes written in buff
 */
int32_t EventReadVerboseNext(uint32_t max_buff_size, uint8_t *buff)
{
    int32_t  ret   = EVENT_NOT_INIT;
    uint8_t *event = NULL;

    do
    {
        if (EventCtrl.initialized == false)
        {
            break;
        }
        event = (uint8_t *)pvPortMalloc(EventCtrl.event_size);
        if (event == NULL)
        {
            ret = EVENT_RET_ERR_MEM;
            break;
        }
        uint32_t log_number = EventManagerGetAutoCount();

        ret = EventManagerReadNext(EventCtrl.event_size, (uint8_t *)event);

        if (ret != EVENT_RET_OK)
        {
            break;
        }
        EventBase_st *base_event  = (EventBase_st *)event;
        uint8_t      *common_data = event + sizeof(EventBase_st);
        uint8_t      *spec_data   = common_data + EventCtrl.cfg->common_data_size;

        ret = snprintf((char *)buff, max_buff_size, "\r\n*********** %05u ***********\r\n", log_number);
        if (EventCtrl.cfg->BasePrintVerbose != NULL)
        {
            ret += EventCtrl.cfg->BasePrintVerbose(base_event, max_buff_size - ret, &buff[ret]);
        }

        LinkedListNode_t *this_cfg_node = EventSearchSpecList(base_event->trigger);

        if (this_cfg_node != NULL)
        {
            EventSpecificCfg_st *specs = this_cfg_node->item;

            if (specs->SpecPrintVerbose != NULL)
            {
                ret += specs->SpecPrintVerbose(base_event->code, spec_data, max_buff_size - ret, &buff[ret]);
            }
        }
        if (EventCtrl.cfg->CommonPrintVerbose != NULL)
        {
            ret += EventCtrl.cfg->CommonPrintVerbose(common_data, max_buff_size - ret, &buff[ret]);
        }

    } while (0);

    if (event != NULL)
    {
        vPortFree(event);
    }

    return ret;
}

/**
 * @brief       Searches the linked list for the specific functions for the informed trigger
 * @param[in]   trigger: trigger to search the specific funcions
 * @return      Node of the informed trigger. NULL if not found
 */
static LinkedListNode_t *EventSearchSpecList(uint8_t trigger)
{
    LinkedListNode_t *desired_node = NULL;

    for (uint32_t i = 0; i < 256; i++)
    {
        desired_node = LinkedListGetNth(EventCtrl.spec_list, i);
        if (desired_node == NULL)
        {
            break;
        }
        EventSpecificCfg_st *cfg;

        cfg = (EventSpecificCfg_st *)desired_node->item;
        if (cfg->trigger != trigger)
        {
            desired_node = NULL;
            continue;
        }
        else
        {
            break;
        }
    }

    return desired_node;
}

/**
 * @brief       Prints the Base information
 * @param[in]   base: The base event @ref EventBase_st
 * @param[in]   max_buff_size: Maximum size of the output buffer
 * @param[out]  buff: Output buffer
 * @return      Number of bytes written in buff
 */
static int32_t EventBasePrint(EventBase_st *const base, uint32_t max_buff_size, uint8_t *buff)
{
    int32_t ret = 0;

    if (EventCtrl.cfg->parser_header != NULL)
    {
        ret = snprintf((char *)buff, max_buff_size, "|%s", EventCtrl.cfg->parser_header);
    }
    if ((int32_t)max_buff_size - ret >= 7)
    {
        ret += snprintf((char *)&buff[ret], max_buff_size - ret, "|%02X%04X", base->trigger, base->code);
    }
    if ((int32_t)max_buff_size - ret >= 9)
    {
        ret += snprintf((char *)&buff[ret], max_buff_size - ret, "|%02u:%02u:%02u", base->hour, base->min, base->sec);
    }
    if ((int32_t)max_buff_size - ret >= 11)
    {
        ret += snprintf((char *)&buff[ret], max_buff_size - ret, "|%02u/%02u/20%02u", base->day, base->mon, base->year);
    }

    return ret;
}

/**
 * @brief       Prints a generic field data
 * @param[in]   in: Data to be printed
 * @param[in]   in_size: Size of the in buffer
 * @param[in]   max_buff_size: Maximum size of the output buffer
 * @param[out]  buff: Output buffer
 * @return      Number of bytes written in buff
 */
static int32_t EventGenericPrint(uint8_t *in, uint32_t in_size, uint32_t max_buff_size, uint8_t *buff)
{
    int32_t ret = 0;

    if (in_size > 0)
    {
        ret = snprintf((char *)buff, max_buff_size, "|");
        for (size_t i = 0; i < in_size; i++)
        {
            ret += snprintf((char *)&buff[ret], max_buff_size - ret, "%02X", in[i]);
            if ((int32_t)max_buff_size - ret < 2)
            {
                break;
            }
        }
    }
    return ret;
}

/**
 * @implements  EventTaskCreate
 */
__weak void EventTaskCreate(void)
{
}
