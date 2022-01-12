/**
 * @file        EventParser.h
 * @brief       Header file of Event parsing and high level access
 * @date        2021-08-21
 * @version     0.1
 * @author      Henrique Awoyama Klein (henrique.klein@perto.com.br)
 * @copyright   Copyright (c) 2021
 */
#ifndef _EVENT_PARSER_H_
#define _EVENT_PARSER_H_
#include <stdint.h>
#include "EventManager.h"
/** @addtogroup EventParser Event Parser
 *  @ingroup EventParser
 * @{
 */

/**
 * @brief       Base event information
 */
typedef struct EventBase_s
{
    uint32_t version : 8; ///<(8 bits) Event Manager Version
    uint32_t trigger : 8; ///<(8 bits) Event code
    uint32_t code : 16;   ///<(16 bits) Event subcode
    uint32_t hour : 5;    ///<(5 bits) Event hour
    uint32_t min : 6;     ///<(6 bits) Event minute
    uint32_t sec : 6;     ///<(6 bits) Event second
    uint32_t day : 5;     ///<(5 bits) Event day
    uint32_t mon : 4;     ///<(4 bits) Event month
    uint32_t year : 6;    ///<(6 bits) - 64 bitsEvent year
} EventBase_st;

/**
 * @brief       Function pointer type of the basic information filling function
 */
typedef bool (*BaseFillData_ft)(uint8_t trigger, uint16_t code, EventBase_st *base);

/**
 * @brief       Function pointer type of the basic information printing function
 */
typedef int32_t (*BasePrintData_ft)(EventBase_st *const base, uint32_t max_buff_size, uint8_t *buff);

/**
 * @brief       Function pointer type of the common information filling function
 */
typedef bool (*CommonFillData_ft)(size_t max_size, uint8_t *buff);

/**
 * @brief       Function pointer type of the common information printing function
 */
typedef int32_t (*CommonPrintData_ft)(uint8_t *const common, uint32_t max_buff_size, uint8_t *buff);

/**
 * @brief       Function pointer type of the specific information filling function
 */
typedef bool (*SpecFillFunc_ft)(uint16_t code, size_t max_buff_size, uint8_t *buff);

/**
 * @brief       Function pointer type of the specific information printing function
 */
typedef int32_t (*SpecPrintFunc_ft)(uint16_t code, uint8_t *val, uint32_t max_buff_size, uint8_t *buff);

/**
 * @brief       Configuration structure of the Event Parser
 */
typedef struct EventParserCfg_s
{
    BaseFillData_ft    BaseFill;           ///< Function to fill the base event @ref EventBase_st
    BasePrintData_ft   BasePrintVerbose;   ///< Function to print the base event @ref EventBase_st in verbose mode
    CommonFillData_ft  CommonFill;         ///< Function to fill the common event
    CommonPrintData_ft CommonPrint;        ///< Function to print the common event
    CommonPrintData_ft CommonPrintVerbose; ///< Function to print the common event in verbose mode
    size_t             common_data_size;   ///< Size of the common event in bytes
    size_t             spec_data_size;     ///< Size of the specific data in bytes
    size_t             number_of_triggers; ///< Number of triggers used in the event (not used yet)
    uint8_t            padding_byte;       ///< Byte value to be used as padding
    const char *       parser_header;
} EventParserCfg_st;

/**
 * @brief       Configuration structure of each trigger's specific data
 */
typedef struct EventSpecificCfg_s
{
    uint8_t          trigger;          ///< Trigger for the specific information
    SpecFillFunc_ft  SpecFill;         ///< Function pointer used to fill the specific data according to the trigger
    SpecPrintFunc_ft SpecPrintVerbose; ///< Function pointer used to print verbosely the specific data according to the trigger
} EventSpecificCfg_st;

EventReturn_e EventInit(const EventParserCfg_st *const parser_cfg, const EventManagerConfig_t *const manager_cfg);
EventReturn_e EventCreateRaw(uint8_t trigger, uint16_t code);
EventReturn_e EventInsertSpecsCfg(const EventSpecificCfg_st *const spec_cfg);
int32_t       EventReadFormat(uint32_t log_number, uint32_t max_buff_size, uint8_t *buff);
int32_t       EventReadFormatNext(uint32_t max_buff_size, uint8_t *buff);
int32_t       EventReadVerbose(uint32_t log_number, uint32_t max_buff_size, uint8_t *buff);
int32_t       EventReadVerboseNext(uint32_t max_buff_size, uint8_t *buff);

/*! @}*/

#endif
